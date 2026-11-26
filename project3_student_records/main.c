/* main.c - menu for the Student Academic Records system */

#include <stdio.h>
#include "student.h"

static void print_menu(void)
{
    printf("\n===== STUDENT ACADEMIC RECORDS SYSTEM =====\n");
    printf(" 1. Register new student\n");
    printf(" 2. Display all students\n");
    printf(" 3. View student details\n");
    printf(" 4. Update student information\n");
    printf(" 5. Delete student\n");
    printf(" 6. Add course to a student\n");
    printf(" 7. Update course marks\n");
    printf(" 8. Display student performance\n");
    printf(" 9. Search students\n");
    printf("10. Sort students\n");
    printf("11. Generate academic report\n");
    printf("12. Save records now\n");
    printf(" 0. Exit (records are saved automatically)\n");
}

int main(void)
{
    StudentDB db = { NULL, 0 };

    /* load saved records; stop if the file exists but cannot be read,
       so that a bad file is never overwritten with empty data */
    int result = load_students(&db, DATA_FILE);
    if (result == 1)
        printf("Loaded %d student record(s) from %s.\n", db.count, DATA_FILE);
    else if (result == 0)
        printf("No saved file found. Starting with an empty database.\n");
    else {
        printf("Please fix or rename %s and run the program again.\n", DATA_FILE);
        return 1;
    }

    int choice;
    do {
        print_menu();
        choice = read_int("Enter your choice (0-12): ", 0, 12);
        switch (choice) {
        case 1:  register_student(&db);       break;
        case 2:  display_all_students(&db);   break;
        case 3:  view_student_details(&db);   break;
        case 4:  update_student(&db);         break;
        case 5:  delete_student(&db);         break;
        case 6:  add_course(&db);             break;
        case 7:  update_marks(&db);           break;
        case 8:  display_performance(&db);    break;
        case 9:  search_menu(&db);            break;
        case 10: sort_menu(&db);              break;
        case 11: generate_report(&db);        break;
        case 12:
            if (save_students(&db, DATA_FILE) == 0)
                printf("Records saved to %s.\n", DATA_FILE);
            break;
        }
    } while (choice != 0);

    if (save_students(&db, DATA_FILE) == 0)
        printf("Records saved to %s. Goodbye.\n", DATA_FILE);
    else
        printf("Warning: records could not be saved.\n");

    free_database(&db);
    return 0;
}
