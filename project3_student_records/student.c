/* student.c - Student Academic Records and Performance Analytics System */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "student.h"

/* ================= Input validation ================= */

/* Remove spaces at the start and end of a string. */
static void trim(char *text)
{
    int start = 0;
    int end = (int)strlen(text);

    while (end > start && isspace((unsigned char)text[end - 1]))
        end--;
    while (start < end && isspace((unsigned char)text[start]))
        start++;
    memmove(text, text + start, end - start);
    text[end - start] = '\0';
}

/* Read one line from the keyboard (exits if input is closed). */
static void read_line(const char *prompt, char *buf, int size)
{
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, size, stdin) == NULL) {
        printf("\nInput closed. Exiting.\n");
        exit(0);
    }
    int len = (int)strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        int ch;                       /* line was too long: throw the rest away */
        while ((ch = getchar()) != '\n' && ch != EOF)
            ;
    }
}

/* Read text into dest. Returns 1 if dest was set, 0 if the user pressed
 * Enter and allow_empty is true. Repeats until the input is valid. */
int read_text(const char *prompt, char *dest, int size, int allow_empty)
{
    char line[256];

    while (1) {
        read_line(prompt, line, sizeof line);
        trim(line);
        if (line[0] == '\0') {
            if (allow_empty)
                return 0;
            printf("Error: this field cannot be empty.\n");
        } else if ((int)strlen(line) >= size) {
            printf("Error: too long (maximum %d characters).\n", size - 1);
        } else if (strchr(line, '|') != NULL) {
            printf("Error: the '|' character is not allowed.\n");
        } else {
            strcpy(dest, line);
            return 1;
        }
    }
}

/* Read a whole number between min and max. */
int read_int(const char *prompt, int min, int max)
{
    char line[256];
    char *end;

    while (1) {
        read_line(prompt, line, sizeof line);
        long value = strtol(line, &end, 10);
        if (end != line && *end == '\0' && value >= min && value <= max)
            return (int)value;
        printf("Error: enter a whole number from %d to %d.\n", min, max);
    }
}

/* Read a number between min and max. */
double read_double(const char *prompt, double min, double max)
{
    char line[256];
    char *end;

    while (1) {
        read_line(prompt, line, sizeof line);
        double value = strtod(line, &end);
        if (end != line && *end == '\0' && value >= min && value <= max)
            return value;
        printf("Error: enter a number from %.2f to %.2f.\n", min, max);
    }
}

/* ================= Small helpers ================= */

/* Compare two strings ignoring upper/lower case (like strcmp). */
static int compare_ignore_case(const char *a, const char *b)
{
    while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) {
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* Return 1 if part appears inside text, ignoring case. */
static int contains_ignore_case(const char *text, const char *part)
{
    int text_len = (int)strlen(text);
    int part_len = (int)strlen(part);

    for (int i = 0; i + part_len <= text_len; i++) {
        int j = 0;
        while (j < part_len &&
               tolower((unsigned char)text[i + j]) == tolower((unsigned char)part[j]))
            j++;
        if (j == part_len)
            return 1;
    }
    return 0;
}

/* Marks of one course as a percentage. */
static double course_percentage(const Course *c)
{
    return c->marks / c->max_marks * 100.0;
}

/* Free all students, their courses, and the student array. */
void free_database(StudentDB *db)
{
    for (int i = 0; i < db->count; i++)
        free(db->records[i].courses);
    free(db->records);
    db->records = NULL;
    db->count = 0;
}

/* ================= Searching by ID ================= */

/* Linear search for a student ID. Returns the index or -1. */
int find_student_index(const StudentDB *db, int id)
{
    for (int i = 0; i < db->count; i++) {
        if (db->records[i].id == id)
            return i;
    }
    return -1;
}

/* Ask for an ID and return that student, or NULL (with a message). */
static Student *ask_for_student(StudentDB *db)
{
    if (db->count == 0) {
        printf("Error: no students registered yet.\n");
        return NULL;
    }
    int id = read_int("Enter student ID: ", 1, MAX_ID);
    int index = find_student_index(db, id);
    if (index < 0) {
        printf("Error: no student with ID %d.\n", id);
        return NULL;
    }
    return &db->records[index];
}

/* ================= Calculations ================= */

/* Overall average in percent (total marks / total maximum marks).
 * Returns -1 if the student has no courses (avoids division by zero). */
double calculate_average(const Student *s)
{
    double total = 0.0, total_max = 0.0;

    for (int i = 0; i < s->num_courses; i++) {
        total += s->courses[i].marks;
        total_max += s->courses[i].max_marks;
    }
    if (s->num_courses == 0 || total_max <= 0.0)
        return -1.0;
    return total / total_max * 100.0;
}

/* ================= Display ================= */

static void print_header(void)
{
    printf("%-8s %-15s %-15s %-22s %-5s %-8s %s\n",
           "ID", "Last name", "First name", "Programme", "Year", "Courses", "Average");
}

static void print_student_row(const Student *s)
{
    double avg = calculate_average(s);

    printf("%-8d %-15.15s %-15.15s %-22.22s %-5d %-8d ",
           s->id, s->last_name, s->first_name, s->programme, s->year, s->num_courses);
    if (avg < 0)
        printf("N/A\n");
    else
        printf("%.1f%%\n", avg);
}

void display_all_students(const StudentDB *db)
{
    if (db->count == 0) {
        printf("No students registered yet.\n");
        return;
    }
    print_header();
    for (int i = 0; i < db->count; i++)
        print_student_row(&db->records[i]);
}

void view_student_details(StudentDB *db)
{
    Student *s = ask_for_student(db);
    if (s == NULL)
        return;

    printf("\nID: %d\nName: %s %s\nProgramme: %s\nYear of study: %d\nCourses registered: %d\n",
           s->id, s->first_name, s->last_name, s->programme, s->year, s->num_courses);
    for (int i = 0; i < s->num_courses; i++)
        printf("  %-10s %-25.25s %.2f / %.2f\n", s->courses[i].code,
               s->courses[i].name, s->courses[i].marks, s->courses[i].max_marks);
}

void display_performance(StudentDB *db)
{
    Student *s = ask_for_student(db);
    if (s == NULL)
        return;

    printf("\nPerformance of %s %s (ID %d)\n", s->first_name, s->last_name, s->id);
    if (s->num_courses == 0) {
        printf("No courses registered. Average not available.\n");
        return;
    }
    printf("%-10s %-25s %-15s %-9s %s\n", "Code", "Course", "Marks", "Percent", "Result");
    for (int i = 0; i < s->num_courses; i++) {
        const Course *c = &s->courses[i];
        double pct = course_percentage(c);
        printf("%-10s %-25.25s %6.2f / %-6.2f %6.1f%%   %s\n", c->code, c->name,
               c->marks, c->max_marks, pct, pct >= PASS_PERCENT ? "Pass" : "Fail");
    }
    printf("Overall average: %.1f%%\n", calculate_average(s));
}

/* ================= Student operations ================= */

void register_student(StudentDB *db)
{
    Student s;

    s.id = read_int("Student ID (positive number): ", 1, MAX_ID);
    if (find_student_index(db, s.id) != -1) {
        printf("Error: student ID %d already exists.\n", s.id);
        return;
    }
    read_text("First name: ", s.first_name, MAX_TEXT, 0);
    read_text("Last name: ", s.last_name, MAX_TEXT, 0);
    read_text("Programme of study: ", s.programme, MAX_TEXT, 0);
    s.year = read_int("Year of study (1-6): ", 1, MAX_YEAR);
    s.num_courses = 0;
    s.courses = NULL;

    /* make room for one more student */
    Student *tmp;
    if (db->records == NULL)
        tmp = malloc(sizeof(Student));
    else
        tmp = realloc(db->records, (db->count + 1) * sizeof(Student));
    if (tmp == NULL) {
        printf("Error: out of memory. Student not registered.\n");
        return;
    }
    db->records = tmp;
    db->records[db->count++] = s;
    printf("Student registered.\n");
}

void update_student(StudentDB *db)
{
    Student *s = ask_for_student(db);
    if (s == NULL)
        return;

    char prompt[200];
    printf("Press Enter to keep the current value.\n");

    snprintf(prompt, sizeof prompt, "First name [%s]: ", s->first_name);
    read_text(prompt, s->first_name, MAX_TEXT, 1);
    snprintf(prompt, sizeof prompt, "Last name [%s]: ", s->last_name);
    read_text(prompt, s->last_name, MAX_TEXT, 1);
    snprintf(prompt, sizeof prompt, "Programme [%s]: ", s->programme);
    read_text(prompt, s->programme, MAX_TEXT, 1);

    snprintf(prompt, sizeof prompt, "Year of study [%d] (0 to keep): ", s->year);
    int year = read_int(prompt, 0, MAX_YEAR);
    if (year != 0)
        s->year = year;
    printf("Student updated.\n");
}

void delete_student(StudentDB *db)
{
    if (db->count == 0) {
        printf("Error: no students registered yet.\n");
        return;
    }
    int id = read_int("Enter student ID to delete: ", 1, MAX_ID);
    int index = find_student_index(db, id);
    if (index < 0) {
        printf("Error: no student with ID %d.\n", id);
        return;
    }

    free(db->records[index].courses);
    for (int i = index; i < db->count - 1; i++)   /* close the gap */
        db->records[i] = db->records[i + 1];
    db->count--;

    if (db->count == 0) {
        free(db->records);
        db->records = NULL;
    } else {
        Student *tmp = realloc(db->records, db->count * sizeof(Student));
        if (tmp != NULL)                           /* shrinking; keep old block if it fails */
            db->records = tmp;
    }
    printf("Student %d deleted.\n", id);
}

/* ================= Course operations ================= */

/* Add one course to a student (code must be new for this student). */
static void add_course_to_student(Student *s)
{
    Course c;
    char prompt[100];

    read_text("Course code: ", c.code, MAX_CODE, 0);
    for (int i = 0; i < s->num_courses; i++) {
        if (compare_ignore_case(s->courses[i].code, c.code) == 0) {
            printf("Error: course %s is already registered for this student.\n", c.code);
            return;
        }
    }
    read_text("Course name: ", c.name, MAX_TEXT, 0);
    c.max_marks = read_double("Maximum marks (1-1000): ", 1.0, 1000.0);
    snprintf(prompt, sizeof prompt, "Marks obtained (0-%.2f): ", c.max_marks);
    c.marks = read_double(prompt, 0.0, c.max_marks);

    Course *tmp;
    if (s->courses == NULL)
        tmp = malloc(sizeof(Course));
    else
        tmp = realloc(s->courses, (s->num_courses + 1) * sizeof(Course));
    if (tmp == NULL) {
        printf("Error: out of memory. Course not added.\n");
        return;
    }
    s->courses = tmp;
    s->courses[s->num_courses++] = c;
    printf("Course added.\n");
}

void add_course(StudentDB *db)
{
    Student *s = ask_for_student(db);
    if (s != NULL)
        add_course_to_student(s);
}

void update_marks(StudentDB *db)
{
    Student *s = ask_for_student(db);
    if (s == NULL)
        return;
    if (s->num_courses == 0) {
        printf("Error: this student has no courses.\n");
        return;
    }

    char code[MAX_CODE];
    char prompt[100];
    read_text("Course code: ", code, MAX_CODE, 0);
    for (int i = 0; i < s->num_courses; i++) {
        Course *c = &s->courses[i];
        if (compare_ignore_case(c->code, code) == 0) {
            snprintf(prompt, sizeof prompt, "New marks (0-%.2f): ", c->max_marks);
            c->marks = read_double(prompt, 0.0, c->max_marks);
            printf("Marks updated.\n");
            return;
        }
    }
    printf("Error: course %s not found for this student.\n", code);
}

/* ================= Searching ================= */

static void search_by_id(const StudentDB *db)
{
    int id = read_int("Enter student ID: ", 1, MAX_ID);
    int index = find_student_index(db, id);

    if (index < 0) {
        printf("No student found with ID %d.\n", id);
        return;
    }
    print_header();
    print_student_row(&db->records[index]);
}

static void search_by_name(const StudentDB *db)
{
    char text[MAX_TEXT];
    int found = 0;

    read_text("Enter first or last name (or part of it): ", text, MAX_TEXT, 0);
    for (int i = 0; i < db->count; i++) {
        const Student *s = &db->records[i];
        if (contains_ignore_case(s->first_name, text) ||
            contains_ignore_case(s->last_name, text)) {
            if (!found)
                print_header();
            print_student_row(s);
            found++;
        }
    }
    if (!found)
        printf("No student found with name matching \"%s\".\n", text);
}

static void search_by_programme(const StudentDB *db)
{
    char text[MAX_TEXT];
    int found = 0;

    read_text("Enter programme (or part of it): ", text, MAX_TEXT, 0);
    for (int i = 0; i < db->count; i++) {
        if (contains_ignore_case(db->records[i].programme, text)) {
            if (!found)
                print_header();
            print_student_row(&db->records[i]);
            found++;
        }
    }
    if (!found)
        printf("No student found in programme matching \"%s\".\n", text);
}

void search_menu(const StudentDB *db)
{
    if (db->count == 0) {
        printf("No students registered yet.\n");
        return;
    }
    printf("1. By student ID\n2. By first or last name\n3. By programme\n");
    int choice = read_int("Choose search type: ", 1, 3);

    if (choice == 1)
        search_by_id(db);
    else if (choice == 2)
        search_by_name(db);
    else
        search_by_programme(db);
}

/* ================= Sorting ================= */

/* Return 1 if student a must come after student b for the given key. */
static int should_swap(const Student *a, const Student *b, int key)
{
    int cmp;

    switch (key) {
    case 1:                                   /* ID, smallest first */
        return a->id > b->id;
    case 2:                                   /* name: last name, then first name */
        cmp = compare_ignore_case(a->last_name, b->last_name);
        if (cmp == 0)
            cmp = compare_ignore_case(a->first_name, b->first_name);
        return cmp > 0;
    case 3:                                   /* average, highest first */
        return calculate_average(a) < calculate_average(b);
    case 4:                                   /* programme, A to Z */
        return compare_ignore_case(a->programme, b->programme) > 0;
    }
    return 0;
}

/* Bubble sort. key: 1 = ID, 2 = name, 3 = average, 4 = programme. */
void sort_students(StudentDB *db, int key)
{
    for (int i = 0; i < db->count - 1; i++) {
        for (int j = 0; j < db->count - 1 - i; j++) {
            if (should_swap(&db->records[j], &db->records[j + 1], key)) {
                Student temp = db->records[j];
                db->records[j] = db->records[j + 1];
                db->records[j + 1] = temp;
            }
        }
    }
}

void sort_menu(StudentDB *db)
{
    if (db->count == 0) {
        printf("No students registered yet.\n");
        return;
    }
    printf("1. By student ID\n2. By name (A-Z)\n3. By overall average (highest first)\n");
    printf("4. By programme (A-Z)\n");
    int choice = read_int("Choose sort type: ", 1, 4);

    sort_students(db, choice);
    printf("Sorted.\n");
    display_all_students(db);
}

/* ================= Reports ================= */

/* 1 if the same course code appears earlier than position (i, k). */
static int course_seen_before(const StudentDB *db, int i, int k)
{
    const char *code = db->records[i].courses[k].code;

    for (int a = 0; a <= i; a++) {
        int limit = (a < i) ? db->records[a].num_courses : k;
        for (int b = 0; b < limit; b++) {
            if (compare_ignore_case(db->records[a].courses[b].code, code) == 0)
                return 1;
        }
    }
    return 0;
}

/* 1 if the same programme appears in a student before index i. */
static int programme_seen_before(const StudentDB *db, int i)
{
    for (int a = 0; a < i; a++) {
        if (compare_ignore_case(db->records[a].programme, db->records[i].programme) == 0)
            return 1;
    }
    return 0;
}

void generate_report(const StudentDB *db)
{
    if (db->count == 0) {
        printf("No students registered. Nothing to report.\n");
        return;
    }

    int total_courses = 0, with_courses = 0;
    int passing = 0, failing = 0, course_pass = 0, course_fail = 0;
    int high = -1, low = -1;
    double sum_avg = 0.0;

    for (int i = 0; i < db->count; i++) {
        const Student *s = &db->records[i];
        total_courses += s->num_courses;
        for (int k = 0; k < s->num_courses; k++) {
            if (course_percentage(&s->courses[k]) >= PASS_PERCENT)
                course_pass++;
            else
                course_fail++;
        }

        double avg = calculate_average(s);
        if (avg < 0)
            continue;                         /* no courses: left out of averages */
        with_courses++;
        sum_avg += avg;
        if (avg >= PASS_PERCENT)
            passing++;
        else
            failing++;
        if (high < 0 || avg > calculate_average(&db->records[high]))
            high = i;
        if (low < 0 || avg < calculate_average(&db->records[low]))
            low = i;
    }

    printf("\n===== ACADEMIC PERFORMANCE REPORT =====\n");
    printf("Total registered students : %d\n", db->count);
    printf("Total courses registered  : %d\n", total_courses);
    if (with_courses == 0) {
        printf("No student has any courses yet, so no averages are available.\n");
    } else {
        printf("Overall average           : %.1f%%\n", sum_avg / with_courses);
        printf("Highest average           : %s %s (ID %d) - %.1f%%\n",
               db->records[high].first_name, db->records[high].last_name,
               db->records[high].id, calculate_average(&db->records[high]));
        printf("Lowest average            : %s %s (ID %d) - %.1f%%\n",
               db->records[low].first_name, db->records[low].last_name,
               db->records[low].id, calculate_average(&db->records[low]));
        printf("Students passing (average >= %.0f%%): %d\n", PASS_PERCENT, passing);
        printf("Students failing (average <  %.0f%%): %d\n", PASS_PERCENT, failing);
        printf("Course results            : %d passed, %d failed\n", course_pass, course_fail);
    }
    printf("Students with no courses  : %d\n", db->count - with_courses);

    printf("\nAverage performance per course:\n");
    if (total_courses == 0)
        printf("  (no courses registered)\n");
    for (int i = 0; i < db->count; i++) {
        for (int k = 0; k < db->records[i].num_courses; k++) {
            if (course_seen_before(db, i, k))
                continue;
            const char *code = db->records[i].courses[k].code;
            double sum = 0.0;
            int n = 0;
            for (int a = 0; a < db->count; a++) {
                for (int b = 0; b < db->records[a].num_courses; b++) {
                    if (compare_ignore_case(db->records[a].courses[b].code, code) == 0) {
                        sum += course_percentage(&db->records[a].courses[b]);
                        n++;
                    }
                }
            }
            printf("  %-10s %-25.25s %6.1f%%  (%d student(s))\n", code,
                   db->records[i].courses[k].name, sum / n, n);
        }
    }

    printf("\nStudents per programme:\n");
    for (int i = 0; i < db->count; i++) {
        if (programme_seen_before(db, i))
            continue;
        int n = 0;
        for (int a = 0; a < db->count; a++) {
            if (compare_ignore_case(db->records[a].programme, db->records[i].programme) == 0)
                n++;
        }
        printf("  %-25.25s %d\n", db->records[i].programme, n);
    }
}

/* ================= File handling ================= */
/* File format (text):
 *   first line:  number of students
 *   per student: id|first|last|programme|year|num_courses
 *   per course:  code|name|marks|max_marks                         */

int save_students(const StudentDB *db, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Error: cannot open %s for writing (%s).\n", filename, strerror(errno));
        return -1;
    }

    fprintf(fp, "%d\n", db->count);
    for (int i = 0; i < db->count; i++) {
        const Student *s = &db->records[i];
        fprintf(fp, "%d|%s|%s|%s|%d|%d\n", s->id, s->first_name, s->last_name,
                s->programme, s->year, s->num_courses);
        for (int k = 0; k < s->num_courses; k++)
            fprintf(fp, "%s|%s|%.10g|%.10g\n", s->courses[k].code, s->courses[k].name,
                    s->courses[k].marks, s->courses[k].max_marks);
    }

    int failed = ferror(fp);
    if (fclose(fp) != 0)
        failed = 1;
    if (failed) {
        printf("Error: writing to %s failed.\n", filename);
        return -1;
    }
    return 0;
}

/* Read one student (and its courses) from the file and add it to db.
 * Returns 1 on success, 0 if the data is invalid. */
static int read_student_record(FILE *fp, StudentDB *db)
{
    char line[512];
    Student s;

    s.courses = NULL;
    if (fgets(line, sizeof line, fp) == NULL)
        return 0;
    if (sscanf(line, "%d|%63[^|]|%63[^|]|%63[^|]|%d|%d", &s.id, s.first_name,
               s.last_name, s.programme, &s.year, &s.num_courses) != 6)
        return 0;
    if (s.id < 1 || s.year < 1 || s.year > MAX_YEAR || s.num_courses < 0 ||
        find_student_index(db, s.id) != -1)
        return 0;

    if (s.num_courses > 0) {
        s.courses = malloc(s.num_courses * sizeof(Course));
        if (s.courses == NULL)
            return 0;
    }
    for (int k = 0; k < s.num_courses; k++) {
        Course *c = &s.courses[k];
        if (fgets(line, sizeof line, fp) == NULL ||
            sscanf(line, "%15[^|]|%63[^|]|%lf|%lf", c->code, c->name,
                   &c->marks, &c->max_marks) != 4 ||
            !(c->max_marks > 0) || !(c->marks >= 0 && c->marks <= c->max_marks)) {
            free(s.courses);
            return 0;
        }
    }
    db->records[db->count++] = s;
    return 1;
}

int load_students(StudentDB *db, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        if (errno == ENOENT)
            return 0;                         /* no file yet: not an error */
        printf("Error: cannot open %s (%s).\n", filename, strerror(errno));
        return -1;
    }

    char line[512];
    int count;
    if (fgets(line, sizeof line, fp) == NULL || sscanf(line, "%d", &count) != 1 ||
        count < 0) {
        fclose(fp);
        printf("Error: file %s is corrupted (bad header).\n", filename);
        return -1;
    }

    db->records = NULL;
    db->count = 0;
    if (count > 0) {
        db->records = malloc(count * sizeof(Student));
        if (db->records == NULL) {
            fclose(fp);
            printf("Error: out of memory while loading %s.\n", filename);
            return -1;
        }
    }
    for (int i = 0; i < count; i++) {
        if (!read_student_record(fp, db)) {
            fclose(fp);
            free_database(db);
            printf("Error: file %s is corrupted (problem at student record %d).\n",
                   filename, i + 1);
            return -1;
        }
    }
    fclose(fp);
    return 1;
}
