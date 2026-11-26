# Technical Report: Student Academic Records and Performance Analytics System (Project 3)

## 1. Objective

Keep academic records for an unknown number of students in dynamically allocated memory, let the user manage them from a menu (register, view, update, delete, add courses, update marks), search and sort them with hand-written algorithms, produce performance statistics, and save the records to a file so they survive between runs.

## 2. Program structure

```
main.c      menu loop, load at start-up, save at exit
   │
student.c   input validation → student/course operations → search → sort
   │        → calculations → reports → save/load
student.h   structures + function declarations (shared by both .c files)
```

`main.c` only reads the menu choice and calls one function per option. All logic is in `student.c`, in small functions with one job each.

## 3. Structures and how they relate

```
StudentDB ──records──▶ Student[ count ]
                         ├─ id, first_name, last_name, programme, year
                         └─ courses ──▶ Course[ num_courses ]
                                         └─ code, name, marks, max_marks
```

- `Course` holds one course result.
- `Student` holds the personal data and a **pointer** to a dynamic array of that student's courses, plus the number of courses.
- `StudentDB` holds a **pointer** to a dynamic array of students and how many there are. It is passed around by pointer, so every function works on the same data.

So there are two levels of dynamic memory: an array of students, and inside each student an array of courses.

## 4. Pointers and dynamic memory

**Ownership.** `StudentDB` owns the student array; each `Student` owns its course array. Whoever owns an array is responsible for freeing it, which gives a simple rule: free the courses of every student first, then the student array (`free_database()`).

**Growing and shrinking.** Both arrays change size by exactly one record at a time with `realloc()`, as the assessment describes:

| Action | Function | Memory call |
|---|---|---|
| Register a student | `register_student` | `malloc` for the first, `realloc` (+1) afterwards |
| Add a course | `add_course_to_student` | `malloc` for the first, `realloc` (+1) afterwards |
| Delete a student | `delete_student` | `free` that student's courses, shift the later students down, `realloc` (-1); the array is freed when it becomes empty |
| Load from file | `load_students`, `read_student_record` | `malloc` the student array and each course array |

**Allocation failures.** Every `malloc`/`realloc` result is stored in a temporary pointer and checked before it replaces the real pointer. If it fails, the operation is cancelled with a message and the existing data is untouched. (If the result were assigned directly to the real pointer, a failure would lose the old array.)

**Pointers to records.** `ask_for_student()` returns a `Student *` into the array, and the operations then change the record through that pointer. The pointer is used immediately and never kept across an operation that could resize the array.

## 5. Algorithms

**Searching (manual, linear).** `find_student_index()` loops through the array comparing IDs. The name and programme searches loop through all students and test whether the typed text appears inside the name, ignoring upper/lower case (`contains_ignore_case()`), so "ali" finds "Alice".

**Sorting (manual, bubble sort).** `sort_students()` repeatedly compares neighbouring students and swaps them when `should_swap()` says they are in the wrong order. `should_swap()` holds the rule for each key: ID (smallest first), name (last name then first name, A-Z), overall average (highest first), programme (A-Z). A swap only happens when strictly out of order, so students that compare equal keep their order. `qsort()` is not used.

**Average (`calculate_average`).** Total marks obtained divided by total maximum marks, times 100. This weights every mark by its maximum, as the assessment requires, instead of averaging percentages. A student with no courses returns -1, shown as "N/A", so there is never a division by zero.

**Report (`generate_report`).**

| Figure | How it is calculated |
|---|---|
| Total students, total courses | Count and sum of `num_courses` |
| Overall average | Mean of the student averages, for students who have at least one course |
| Highest / lowest average | Largest and smallest student average |
| Passing / failing students | A student passes when their overall average is at least 50% (`PASS_PERCENT`); students with no courses are reported separately |
| Course results passed / failed | A course passes when marks are at least 50% of the maximum marks |
| Average per course | For each distinct course code (found with `course_seen_before`), the mean percentage over all students who take it |
| Students per programme | For each distinct programme (found with `programme_seen_before`), a count |

## 6. File handling

Text format, one record per line, fields separated by `|`:

```
5                                          number of students
101|Alice|Uwase|Computer Science|2|3       id|first|last|programme|year|number of courses
CS101|Programming in C|85|100              code|name|marks|max marks  (one line per course)
```

A text file was chosen because it can be read and checked by eye.

- **Saving** (`save_students`) checks that the file opened and that writing and closing succeeded.
- **Loading** (`load_students` and `read_student_record`) rebuilds the dynamic structure: it allocates the student array from the header count, then for each student allocates its course array and reads its courses. Each record is checked (ID is positive and not a duplicate, year 1-6, marks between 0 and the maximum, maximum above 0).
- **A missing file** is normal on the first run: the program starts empty.
- **A damaged file** (bad header, truncated, duplicate ID, bad marks, a folder with that name) is reported, everything already allocated is freed, and the program stops. It stops on purpose so that a bad file is never overwritten by an empty save.
- The `|` character is refused in text fields because it separates the fields.

## 7. Input validation

All keyboard input goes through three helpers that repeat until the input is valid:

| Helper | Accepts |
|---|---|
| `read_int(prompt, min, max)` | A whole number in range (checked with `strtol`, so "12abc" is rejected) |
| `read_double(prompt, min, max)` | A number in range (checked with `strtod`) |
| `read_text(prompt, dest, size, allow_empty)` | Non-empty text that fits the field and contains no `|` |

Because every value is checked as it is typed, the operations themselves can assume valid data. The program also checks for duplicate student IDs, duplicate course codes within one student, non-existent students and courses, and empty databases.

## 8. Required concepts and where they are

| Concept | Location |
|---|---|
| Structures | `student.h` lines 13-35 |
| `malloc` / `realloc` for students | `register_student()` (`student.c` lines 275-277) |
| `malloc` / `realloc` for courses | `add_course_to_student()` (lines 361-363) |
| `realloc` shrink and `free` on delete | `delete_student()` (lines 323-332) |
| `free` of everything | `free_database()` (lines 135-141) |
| Manual search | `search_by_name()` (line 420), `find_student_index()` (line 147) |
| Manual sort | `sort_students()` (line 499), `should_swap()` (line 478) |
| Average and division-by-zero guard | `calculate_average()` (line 176) |
| Save and load | `save_students()` (line 652), `load_students()` (line 716) |

## 9. Testing

All of these were run and passed: registering 5 students and 8 courses; save, quit and restart; details and performance views (including a student with no courses); search by ID, part of a name and programme, with and without matches; the four sorts; update, delete (first, middle, unknown, all students); duplicate IDs and course codes; invalid menu choices, IDs, years, marks and text; empty-database behaviour for every option; missing data file; five kinds of damaged data file; and valgrind runs with no leaks or errors. A failed save (full or read-only disk) and an allocation failure are handled in the code but could not be triggered here, so they are untested.

## 10. Limitations

- Arrays grow by one record per `realloc`. This follows the assessment but copies more than doubling would; it is fine for the size of data in this assignment.
- Bubble sort takes time proportional to the square of the number of students; fine for hundreds of students, slow for many thousands.
- Names are limited to 63 characters and `|` cannot be used in text.
- A hand-edited data file with the same course code twice for one student is not rejected on loading (it is rejected when typed in).
- "Student passes" is defined in this project as an average of at least 50%; the assessment defines only the pass mark for a course. The report shows both kinds of result.
