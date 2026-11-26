/* main.c - menu for the Sales Transaction Management System */

#include <stdio.h>
#include "sales.h"

static void print_menu(void)
{
    printf("\n===== SALES TRANSACTION MANAGEMENT SYSTEM =====\n");
    printf(" 1. Add New Transaction\n");
    printf(" 2. Display All Transactions\n");
    printf(" 3. Update Transaction\n");
    printf(" 4. Delete Transaction\n");
    printf(" 5. Process Sales Data\n");
    printf(" 6. Filter Transactions (Callbacks)\n");
    printf(" 7. Search Transactions\n");
    printf(" 8. Sort Transactions\n");
    printf(" 9. Generate Sales Report\n");
    printf("10. Save Transactions\n");
    printf("11. Load Transactions\n");
    printf(" 0. Exit (transactions are saved automatically)\n");
}

int main(void)
{
    TransactionList list;

    if (init_list(&list) != 0) {
        printf("Error: out of memory.\n");
        return 1;
    }

    /* load saved transactions; stop if the file exists but cannot be read,
       so that a bad file is never overwritten with empty data */
    int result = load_transactions(&list, DATA_FILE);
    if (result == 1)
        printf("Loaded %d transaction(s) from %s.\n", list.count, DATA_FILE);
    else if (result == 0)
        printf("No saved file found. Starting with no transactions.\n");
    else {
        printf("Please fix or rename %s and run the program again.\n", DATA_FILE);
        free_list(&list);
        return 1;
    }

    int choice;
    do {
        print_menu();
        choice = read_menu_choice();
        switch (choice) {
        case 1:  add_transaction(&list);            break;
        case 2:  display_all_transactions(&list);   break;
        case 3:  update_transaction(&list);         break;
        case 4:  delete_transaction(&list);         break;
        case 5:  process_sales_data(&list);         break;
        case 6:  filter_menu(&list);                break;
        case 7:  search_menu(&list);                break;
        case 8:  sort_menu(&list);                  break;
        case 9:  generate_sales_report(&list);      break;
        case 10:
            if (save_transactions(&list, DATA_FILE) == 0)
                printf("Transactions saved to %s.\n", DATA_FILE);
            break;
        case 11: {
            int r = load_transactions(&list, DATA_FILE);
            if (r == 1)
                printf("Loaded %d transaction(s) from %s.\n", list.count, DATA_FILE);
            else if (r == 0)
                printf("No saved file found. Current transactions are unchanged.\n");
            else
                printf("Load failed. Current transactions are unchanged.\n");
            break;
        }
        }
    } while (choice != 0);

    if (save_transactions(&list, DATA_FILE) == 0)
        printf("Transactions saved to %s. Goodbye.\n", DATA_FILE);
    else
        printf("Warning: transactions could not be saved.\n");

    free_list(&list);
    return 0;
}
