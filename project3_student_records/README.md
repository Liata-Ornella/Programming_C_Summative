# Project 3: Student Academic Records and Performance Analytics System (C)

A menu-driven C program that keeps academic records for any number of students. Records are held in dynamically allocated memory and saved to a text file, so they are still there the next time the program runs.

## Files

| File | Purpose |
|---|---|
| `student.h` | Structure definitions and function declarations |
| `student.c` | All the logic: input validation, student and course operations, search, sort, reports, file save/load |
| `main.c` | The menu and the program start-up and shut-down |
| `students.txt` | Sample data (5 students, 8 courses) in the program's save format |

## Build and run

```bash
gcc -Wall -Wextra -o student_system main.c student.c
./student_system
```

The program loads `students.txt` from the current folder at start-up and saves it when you choose **0 (Exit)** or **12 (Save)**.

## Menu

```
 1. Register new student        7. Update course marks
 2. Display all students        8. Display student performance
 3. View student details        9. Search students
 4. Update student information 10. Sort students
 5. Delete student             11. Generate academic report
 6. Add course to a student    12. Save records now
 0. Exit (records are saved automatically)
```

## Structures and how they relate

```
StudentDB ──records──> Student[ count ]
                        └─ courses ──> Course[ num_courses ]
```

- `Course`: code, name, marks, max_marks.
- `Student`: id, first_name, last_name, programme, year, num_courses, and a pointer `courses` to a dynamic array of `Course`.
- `StudentDB`: a pointer `records` to a dynamic array of `Student`, and `count`. `records` is `NULL` when `count` is 0.

## Pointers and dynamic memory

| Concept | Where |
|---|---|
| `malloc()` for the first student / first course | `register_student()`, `add_course_to_student()` |
| `realloc()` when adding | Same two functions (array grows by one) |
| `realloc()` when removing | `delete_student()` (array shrinks by one; freed completely when empty) |
| `malloc()` when loading from file | `load_students()`, `read_student_record()` |
| Allocation failure handling | Every `malloc`/`realloc` result is checked; the operation is cancelled with a message and existing data is left unchanged |
| `free()` | `delete_student()` (the student's courses), `free_database()` (everything, called at exit) |
| Pointers to records | `ask_for_student()` returns a `Student *`; functions take `StudentDB *db`, `Student *s`, `const Student *s`, `Course *c` |

## Searching and sorting (both written by hand)

- **Search** (`search_menu()`): a linear search through the records. `find_student_index()` finds an exact ID. `search_by_name()` and `search_by_programme()` match a part of the text, ignoring upper/lower case, using `contains_ignore_case()`.
- **Sort** (`sort_students()`): bubble sort. `should_swap()` decides the order for the chosen key: ID (smallest first), name (last name, then first name, A-Z), overall average (highest first, students without courses last), programme (A-Z). No `qsort()` is used.

## Academic calculations

- **Student average** (`calculate_average()`) = total marks obtained / total maximum marks x 100. If the student has no courses it returns -1, which is shown as "N/A", so there is no division by zero.
- **Course pass**: marks are at least 50% of the maximum marks (`PASS_PERCENT`).
- **Overall average**: the mean of the student averages, for students who have at least one course.
- **Student passing / failing**: a student passes when their overall average is at least 50%. Students with no courses are counted in neither group and are reported separately. The report also shows how many individual course results passed and failed.
- **Average per course**: the mean percentage of each course code across all students who take it.
- **Students per programme**: students are grouped by programme name, ignoring upper/lower case.

## File handling

Text file `students.txt`:

```
5                                         <- number of students
101|Alice|Uwase|Computer Science|2|3      <- id|first|last|programme|year|num_courses
CS101|Programming in C|85|100             <- code|name|marks|max_marks (one line per course)
...
```

- A missing file is not an error: the program starts empty and creates the file on the first save.
- A file that exists but is damaged (bad header, truncated, duplicate ID, marks out of range, unreadable) is reported, memory is released, and the program stops. This stops a bad file from being overwritten with empty data.
- Saving checks that the file opened and that writing and closing worked.
- The `|` character is not accepted in text fields because it separates the fields in the file.

## Validation and error handling

| Check | Behaviour |
|---|---|
| Menu choice not a number from 0 to 12 | Message, asks again |
| Student ID not a positive whole number, or already used | Message / registration cancelled |
| Empty text, text too long, text containing `\|` | Message, asks again |
| Year outside 1-6 | Message, asks again |
| Course already registered for the student (code compared ignoring case) | Message, course not added |
| Maximum marks outside 1-1000; marks outside 0 to maximum | Message, asks again |
| Update/delete/view for a student or course that does not exist | Message, nothing changes |
| Operations on an empty database | "No students registered yet." |
| Memory allocation failure, file open/read/write failure | Message; data left unchanged |
| Input closed (Ctrl+D) | Message and the program ends; changes since the last save are not saved |

## Sample report (from `students.txt`)

```
===== ACADEMIC PERFORMANCE REPORT =====
Total registered students : 5
Total courses registered  : 8
Overall average           : 66.3%
Highest average           : Chantal Ingabire (ID 103) - 87.3%
Lowest average            : Brian Mugisha (ID 102) - 46.5%
Students passing (average >= 50%): 3
Students failing (average <  50%): 1
Course results            : 6 passed, 2 failed
Students with no courses  : 1

Average performance per course:
  CS101      Programming in C            71.3%  (3 student(s))
  CS102      Linux Administration        68.0%  (2 student(s))
  MTH101     Calculus                    50.0%  (2 student(s))
  SE201      Algorithms                  80.0%  (1 student(s))

Students per programme:
  Computer Science          2
  Software Engineering      2
  Networking                1
```

The numbers were checked by hand, for example Alice: (85 + 72 + 45) / 300 = 67.3%.

## Testing

All tests below were run and passed.

| Test | Expected | Result |
|---|---|---|
| Register 5 students and 8 courses | Records created, table and report correct | Pass |
| Save, quit, restart | Same records loaded back | Pass |
| View details / performance of a student | Correct courses, percentages, Pass/Fail, average | Pass |
| Performance of a student with no courses | "Average not available", no crash | Pass |
| Search by ID, name (part of a name), programme | Matching rows shown | Pass |
| Search with no match (ID 999, "zzz", "chemistry") | "No student found" message | Pass |
| Sort by ID, name, average, programme | Correct order each time | Pass |
| Update student information (Enter keeps a value) | Only the typed fields change | Pass |
| Update marks; unknown course; student with no courses | Updated / error messages | Pass |
| Delete student (first, middle, unknown ID, all students) | Records removed, file updated, empty database handled | Pass |
| Duplicate student ID; duplicate course code | Rejected | Pass |
| Invalid menu choices (`abc`, `13`, `-1`) | Rejected | Pass |
| Invalid ID, year, marks (`0`, `abc`, `9`, `150`, `-5`) | Rejected, asks again | Pass |
| Empty text and text containing `\|` | Rejected | Pass |
| Report with an empty database | "Nothing to report" | Pass |
| Missing data file | Starts empty | Pass |
| Damaged data file (5 kinds: bad header, duplicate ID, bad marks, truncated, a folder named `students.txt`) | Error message, program stops, nothing overwritten | Pass |
| Memory check with valgrind (full session, delete-all session) | No leaks or errors | Pass |

Not tested: a failure while saving (for example a full or read-only disk) and a memory allocation failure. The code handles both, but they could not be triggered in the test environment.
