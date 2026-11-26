/* student.h - structures and function declarations */
#ifndef STUDENT_H
#define STUDENT_H

#define MAX_TEXT      64          /* size of name/programme text fields */
#define MAX_CODE      16          /* size of a course code */
#define MAX_ID        999999999   /* largest allowed student ID */
#define MAX_YEAR      6           /* largest allowed year of study */
#define PASS_PERCENT  50.0        /* pass mark in percent */
#define DATA_FILE     "students.txt"

/* One course taken by a student */
typedef struct {
    char   code[MAX_CODE];
    char   name[MAX_TEXT];
    double marks;        /* marks obtained */
    double max_marks;    /* maximum marks */
} Course;

/* One student; courses is a dynamic array of num_courses items */
typedef struct {
    int     id;
    char    first_name[MAX_TEXT];
    char    last_name[MAX_TEXT];
    char    programme[MAX_TEXT];
    int     year;
    int     num_courses;
    Course *courses;
} Student;

/* All students; records is a dynamic array (NULL when count is 0) */
typedef struct {
    Student *records;
    int      count;
} StudentDB;

/* input validation */
int    read_text(const char *prompt, char *dest, int size, int allow_empty);
int    read_int(const char *prompt, int min, int max);
double read_double(const char *prompt, double min, double max);

/* memory */
void free_database(StudentDB *db);

/* student operations */
void register_student(StudentDB *db);
void display_all_students(const StudentDB *db);
void view_student_details(StudentDB *db);
void update_student(StudentDB *db);
void delete_student(StudentDB *db);

/* course operations */
void add_course(StudentDB *db);
void update_marks(StudentDB *db);
void display_performance(StudentDB *db);

/* searching and sorting */
int  find_student_index(const StudentDB *db, int id);
void search_menu(const StudentDB *db);
void sort_students(StudentDB *db, int key);
void sort_menu(StudentDB *db);

/* calculations and reports */
double calculate_average(const Student *s);
void   generate_report(const StudentDB *db);

/* files: save returns 0 or -1; load returns 1 (loaded), 0 (no file), -1 (error) */
int save_students(const StudentDB *db, const char *filename);
int load_students(StudentDB *db, const char *filename);

#endif
