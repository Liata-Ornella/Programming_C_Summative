/* sales.h - structures, function pointer types and function declarations */
#ifndef SALES_H
#define SALES_H

#define MAX_PRODUCT       64         /* size of the product name field */
#define DATE_SIZE         11         /* "YYYY-MM-DD" plus end of string */
#define MAX_ID            999999999  /* largest allowed transaction ID */
#define MAX_QUANTITY      1000000
#define MAX_PRICE         1000000.0
#define INITIAL_CAPACITY  4          /* starting size of the dynamic array */
#define DATA_FILE         "transactions.txt"

/* One sales transaction */
typedef struct {
    int    id;
    char   product[MAX_PRODUCT];
    int    quantity;
    double unit_price;
    double total;                    /* quantity * unit_price */
    char   date[DATE_SIZE];
} Transaction;

/* Dynamic array of transactions */
typedef struct {
    Transaction *items;
    int          count;              /* transactions stored */
    int          capacity;           /* transactions the array can hold */
} TransactionList;

/* Function pointer types */
typedef void (*SalesOperation)(const TransactionList *list);
typedef int  (*FilterFunc)(const Transaction *t, const void *param);   /* callback */
typedef int  (*CompareFunc)(const Transaction *a, const Transaction *b); /* callback */

/* menu input: returns a validated choice from 0 to 11 */
int read_menu_choice(void);

/* list handling */
int  init_list(TransactionList *list);
void free_list(TransactionList *list);
int  append_transaction(TransactionList *list, const Transaction *t);
int  find_transaction_index(const TransactionList *list, int id);

/* transaction management */
void add_transaction(TransactionList *list);
void display_all_transactions(const TransactionList *list);
void update_transaction(TransactionList *list);
void delete_transaction(TransactionList *list);

/* function pointers: process sales data */
void process_sales_data(const TransactionList *list);

/* callbacks: filters, counting and comparing */
int filter_above_amount(const Transaction *t, const void *param);
int filter_by_product(const Transaction *t, const void *param);
int filter_quantity_at_least(const Transaction *t, const void *param);
int count_matching(const TransactionList *list, FilterFunc filter, const void *param);
int display_matching(const TransactionList *list, FilterFunc filter, const void *param);
int compare_by_id(const Transaction *a, const Transaction *b);
int compare_by_product(const Transaction *a, const Transaction *b);
int compare_by_total_ascending(const Transaction *a, const Transaction *b);
int compare_by_total_descending(const Transaction *a, const Transaction *b);
void sort_transactions(TransactionList *list, CompareFunc compare);

/* menus for filtering, searching and sorting */
void filter_menu(const TransactionList *list);
void search_menu(const TransactionList *list);
void sort_menu(TransactionList *list);

/* report */
void generate_sales_report(const TransactionList *list);

/* files: save returns 0 or -1; load returns 1 (loaded), 0 (no file), -1 (error) */
int save_transactions(const TransactionList *list, const char *filename);
int load_transactions(TransactionList *list, const char *filename);

#endif
