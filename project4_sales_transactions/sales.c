/* sales.c - Sales Transaction Processing and Analysis System */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "sales.h"

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
static int read_text(const char *prompt, char *dest, int size, int allow_empty)
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
static int read_int(const char *prompt, int min, int max)
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
static double read_double(const char *prompt, double min, double max)
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

/* Read the main menu choice (0-11). */
int read_menu_choice(void)
{
    return read_int("Enter your choice (0-11): ", 0, 11);
}

/* Return 1 if text is a real calendar date written as YYYY-MM-DD. */
static int is_valid_date(const char *text)
{
    static const int days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int year, month, day;

    if (strlen(text) != 10 || text[4] != '-' || text[7] != '-')
        return 0;
    for (int i = 0; i < 10; i++) {
        if (i != 4 && i != 7 && !isdigit((unsigned char)text[i]))
            return 0;
    }
    if (sscanf(text, "%d-%d-%d", &year, &month, &day) != 3)
        return 0;
    if (year < 1900 || year > 2100 || month < 1 || month > 12 || day < 1)
        return 0;

    int max_day = days_in_month[month - 1];
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0))
        max_day = 29;
    return day <= max_day;
}

/* Read a valid date into dest (DATE_SIZE characters). */
static void read_date(const char *prompt, char *dest)
{
    char line[256];

    while (1) {
        read_line(prompt, line, sizeof line);
        trim(line);
        if (is_valid_date(line)) {
            strcpy(dest, line);
            return;
        }
        printf("Error: enter a real date as YYYY-MM-DD (for example 2026-11-21).\n");
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

/* ================= Dynamic list ================= */

/* Allocate the starting array. Returns 0 on success, -1 if out of memory. */
int init_list(TransactionList *list)
{
    list->items = malloc(INITIAL_CAPACITY * sizeof(Transaction));
    list->count = 0;
    list->capacity = INITIAL_CAPACITY;
    if (list->items == NULL) {
        list->capacity = 0;
        return -1;
    }
    return 0;
}

void free_list(TransactionList *list)
{
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

/* Add a copy of t to the end of the list, doubling the array when it is
 * full. Returns 0 on success, -1 if out of memory. */
int append_transaction(TransactionList *list, const Transaction *t)
{
    if (list->count == list->capacity) {
        int new_capacity = list->capacity * 2;
        Transaction *tmp = realloc(list->items, new_capacity * sizeof(Transaction));
        if (tmp == NULL)
            return -1;
        list->items = tmp;
        list->capacity = new_capacity;
    }
    list->items[list->count++] = *t;
    return 0;
}

/* Linear search for a transaction ID. Returns the index or -1. */
int find_transaction_index(const TransactionList *list, int id)
{
    for (int i = 0; i < list->count; i++) {
        if (list->items[i].id == id)
            return i;
    }
    return -1;
}

/* ================= Display ================= */

static void print_header(void)
{
    printf("%-8s %-22s %-8s %-12s %-14s %s\n",
           "ID", "Product", "Qty", "Unit price", "Total", "Date");
}

static void print_row(const Transaction *t)
{
    printf("%-8d %-22.22s %-8d %-12.2f %-14.2f %s\n",
           t->id, t->product, t->quantity, t->unit_price, t->total, t->date);
}

void display_all_transactions(const TransactionList *list)
{
    if (list->count == 0) {
        printf("No transactions recorded yet.\n");
        return;
    }
    print_header();
    for (int i = 0; i < list->count; i++)
        print_row(&list->items[i]);
}

/* ================= Add, update, delete ================= */

/* Ask for product, quantity, price and date, and work out the total. */
static void read_transaction_details(Transaction *t)
{
    read_text("Product name: ", t->product, MAX_PRODUCT, 0);
    t->quantity = read_int("Quantity sold (1 or more): ", 1, MAX_QUANTITY);
    t->unit_price = read_double("Unit price (more than 0): ", 0.01, MAX_PRICE);
    read_date("Transaction date (YYYY-MM-DD): ", t->date);
    t->total = t->quantity * t->unit_price;
}

void add_transaction(TransactionList *list)
{
    Transaction t;

    t.id = read_int("Transaction ID (positive number): ", 1, MAX_ID);
    if (find_transaction_index(list, t.id) != -1) {
        printf("Error: transaction ID %d already exists.\n", t.id);
        return;
    }
    read_transaction_details(&t);

    if (append_transaction(list, &t) != 0) {
        printf("Error: out of memory. Transaction not added.\n");
        return;
    }
    printf("Transaction added (total %.2f).\n", t.total);
}

void update_transaction(TransactionList *list)
{
    if (list->count == 0) {
        printf("Error: no transactions recorded yet.\n");
        return;
    }
    int id = read_int("Enter transaction ID to update: ", 1, MAX_ID);
    int index = find_transaction_index(list, id);
    if (index < 0) {
        printf("Error: no transaction with ID %d.\n", id);
        return;
    }

    Transaction *t = &list->items[index];
    printf("Current transaction:\n");
    print_header();
    print_row(t);
    printf("Enter the new details (the ID stays the same).\n");
    read_transaction_details(t);
    printf("Transaction updated (new total %.2f).\n", t->total);
}

void delete_transaction(TransactionList *list)
{
    if (list->count == 0) {
        printf("Error: no transactions recorded yet.\n");
        return;
    }
    int id = read_int("Enter transaction ID to delete: ", 1, MAX_ID);
    int index = find_transaction_index(list, id);
    if (index < 0) {
        printf("Error: no transaction with ID %d.\n", id);
        return;
    }

    for (int i = index; i < list->count - 1; i++)   /* close the gap */
        list->items[i] = list->items[i + 1];
    list->count--;
    printf("Transaction %d deleted.\n", id);
}

/* ================= Sales figures ================= */

static double total_revenue(const TransactionList *list)
{
    double sum = 0.0;
    for (int i = 0; i < list->count; i++)
        sum += list->items[i].total;
    return sum;
}

static int total_units(const TransactionList *list)
{
    int sum = 0;
    for (int i = 0; i < list->count; i++)
        sum += list->items[i].quantity;
    return sum;
}

/* Average value; returns 0 for an empty list (avoids division by zero). */
static double average_value(const TransactionList *list)
{
    if (list->count == 0)
        return 0.0;
    return total_revenue(list) / list->count;
}

/* Index of the highest-value transaction, or -1 if the list is empty. */
static int find_highest(const TransactionList *list)
{
    int best = -1;
    for (int i = 0; i < list->count; i++) {
        if (best < 0 || list->items[i].total > list->items[best].total)
            best = i;
    }
    return best;
}

/* Index of the lowest-value transaction, or -1 if the list is empty. */
static int find_lowest(const TransactionList *list)
{
    int best = -1;
    for (int i = 0; i < list->count; i++) {
        if (best < 0 || list->items[i].total < list->items[best].total)
            best = i;
    }
    return best;
}

/* ================= Function pointers: process sales data ================= */

/* Each operation has the same type (SalesOperation) so they can be
 * stored in a table and called through a function pointer. */
static void op_total_revenue(const TransactionList *list)
{
    printf("Total revenue: %.2f\n", total_revenue(list));
}

static void op_average_value(const TransactionList *list)
{
    printf("Average transaction value: %.2f\n", average_value(list));
}

static void op_highest(const TransactionList *list)
{
    printf("Highest-value transaction:\n");
    print_header();
    print_row(&list->items[find_highest(list)]);
}

static void op_lowest(const TransactionList *list)
{
    printf("Lowest-value transaction:\n");
    print_header();
    print_row(&list->items[find_lowest(list)]);
}

static void op_total_units(const TransactionList *list)
{
    printf("Total units sold: %d\n", total_units(list));
}

void process_sales_data(const TransactionList *list)
{
    static const struct {
        const char    *name;
        SalesOperation run;
    } operations[] = {
        { "Calculate total revenue",              op_total_revenue },
        { "Calculate average transaction value",  op_average_value },
        { "Identify the highest-value transaction", op_highest },
        { "Identify the lowest-value transaction", op_lowest },
        { "Calculate total units sold",           op_total_units },
    };
    int n = (int)(sizeof operations / sizeof operations[0]);

    if (list->count == 0) {
        printf("No transactions recorded yet.\n");
        return;
    }
    for (int i = 0; i < n; i++)
        printf("%d. %s\n", i + 1, operations[i].name);
    int choice = read_int("Choose an operation: ", 1, n);

    SalesOperation run = operations[choice - 1].run;   /* chosen at runtime */
    run(list);
}

/* ================= Callbacks ================= */

/* Filter callbacks: return 1 if the transaction matches. param is the
 * value to compare with (its type depends on the filter). */
int filter_above_amount(const Transaction *t, const void *param)
{
    return t->total > *(const double *)param;
}

int filter_by_product(const Transaction *t, const void *param)
{
    return compare_ignore_case(t->product, (const char *)param) == 0;
}

int filter_quantity_at_least(const Transaction *t, const void *param)
{
    return t->quantity >= *(const int *)param;
}

/* Count the transactions for which the callback returns 1. */
int count_matching(const TransactionList *list, FilterFunc filter, const void *param)
{
    int count = 0;
    for (int i = 0; i < list->count; i++) {
        if (filter(&list->items[i], param))
            count++;
    }
    return count;
}

/* Show the transactions for which the callback returns 1. Returns how many. */
int display_matching(const TransactionList *list, FilterFunc filter, const void *param)
{
    int count = 0;
    for (int i = 0; i < list->count; i++) {
        if (filter(&list->items[i], param)) {
            if (count == 0)
                print_header();
            print_row(&list->items[i]);
            count++;
        }
    }
    if (count == 0)
        printf("No matching transactions.\n");
    else
        printf("%d transaction(s) match.\n", count);
    return count;
}

/* Compare callbacks: return negative, zero or positive like strcmp. */
int compare_by_id(const Transaction *a, const Transaction *b)
{
    return a->id - b->id;
}

int compare_by_product(const Transaction *a, const Transaction *b)
{
    return compare_ignore_case(a->product, b->product);
}

int compare_by_total_ascending(const Transaction *a, const Transaction *b)
{
    if (a->total < b->total)
        return -1;
    return a->total > b->total;
}

int compare_by_total_descending(const Transaction *a, const Transaction *b)
{
    return compare_by_total_ascending(b, a);
}

/* Bubble sort. The callback decides the order. */
void sort_transactions(TransactionList *list, CompareFunc compare)
{
    for (int i = 0; i < list->count - 1; i++) {
        for (int j = 0; j < list->count - 1 - i; j++) {
            if (compare(&list->items[j], &list->items[j + 1]) > 0) {
                Transaction temp = list->items[j];
                list->items[j] = list->items[j + 1];
                list->items[j + 1] = temp;
            }
        }
    }
}

/* ================= Menus: filter, search, sort ================= */

void filter_menu(const TransactionList *list)
{
    if (list->count == 0) {
        printf("No transactions recorded yet.\n");
        return;
    }
    printf("1. Transactions with a total above an amount\n");
    printf("2. Transactions for a product\n");
    printf("3. Count transactions with a quantity of at least N\n");
    int choice = read_int("Choose a filter: ", 1, 3);

    if (choice == 1) {
        double amount = read_double("Amount: ", 0.0, 1000000000.0);
        display_matching(list, filter_above_amount, &amount);
    } else if (choice == 2) {
        char product[MAX_PRODUCT];
        read_text("Product name: ", product, MAX_PRODUCT, 0);
        display_matching(list, filter_by_product, product);
    } else {
        int quantity = read_int("Minimum quantity: ", 1, MAX_QUANTITY);
        int n = count_matching(list, filter_quantity_at_least, &quantity);
        printf("%d transaction(s) have a quantity of at least %d.\n", n, quantity);
    }
}

/* ---- searching (manual linear searches) ---- */

static void search_by_id(const TransactionList *list)
{
    int id = read_int("Enter transaction ID: ", 1, MAX_ID);
    int index = find_transaction_index(list, id);

    if (index < 0) {
        printf("No transaction found with ID %d.\n", id);
        return;
    }
    print_header();
    print_row(&list->items[index]);
}

static void search_by_product(const TransactionList *list)
{
    char text[MAX_PRODUCT];
    int found = 0;

    read_text("Enter product name (or part of it): ", text, MAX_PRODUCT, 0);
    for (int i = 0; i < list->count; i++) {
        if (contains_ignore_case(list->items[i].product, text)) {
            if (!found)
                print_header();
            print_row(&list->items[i]);
            found++;
        }
    }
    if (!found)
        printf("No transaction found for product matching \"%s\".\n", text);
}

static void search_by_amount(const TransactionList *list)
{
    double amount = read_double("Enter transaction amount: ", 0.01, 1000000000.0);
    int found = 0;

    for (int i = 0; i < list->count; i++) {
        double diff = list->items[i].total - amount;
        if (diff < 0)
            diff = -diff;
        if (diff < 0.005) {                    /* equal to the nearest cent */
            if (!found)
                print_header();
            print_row(&list->items[i]);
            found++;
        }
    }
    if (!found)
        printf("No transaction found with amount %.2f.\n", amount);
}

void search_menu(const TransactionList *list)
{
    if (list->count == 0) {
        printf("No transactions recorded yet.\n");
        return;
    }
    printf("1. By transaction ID\n2. By product name\n3. By transaction amount\n");
    int choice = read_int("Choose search type: ", 1, 3);

    if (choice == 1)
        search_by_id(list);
    else if (choice == 2)
        search_by_product(list);
    else
        search_by_amount(list);
}

void sort_menu(TransactionList *list)
{
    if (list->count == 0) {
        printf("No transactions recorded yet.\n");
        return;
    }
    printf("1. By transaction ID\n2. By product name (A-Z)\n");
    printf("3. By total amount (lowest first)\n4. By total amount (highest first)\n");
    int choice = read_int("Choose sort type: ", 1, 4);

    CompareFunc compare = compare_by_id;       /* callback chosen at runtime */
    if (choice == 2)
        compare = compare_by_product;
    else if (choice == 3)
        compare = compare_by_total_ascending;
    else if (choice == 4)
        compare = compare_by_total_descending;

    sort_transactions(list, compare);
    printf("Sorted.\n");
    display_all_transactions(list);
}

/* ================= Report ================= */

void generate_sales_report(const TransactionList *list)
{
    if (list->count == 0) {
        printf("No transactions recorded. Nothing to report.\n");
        return;
    }

    const Transaction *highest = &list->items[find_highest(list)];
    const Transaction *lowest = &list->items[find_lowest(list)];

    printf("\n===== SALES REPORT =====\n");
    printf("Total number of transactions : %d\n", list->count);
    printf("Total revenue                : %.2f\n", total_revenue(list));
    printf("Average transaction value    : %.2f\n", average_value(list));
    printf("Highest transaction value    : %.2f (ID %d, %s)\n",
           highest->total, highest->id, highest->product);
    printf("Lowest transaction value     : %.2f (ID %d, %s)\n",
           lowest->total, lowest->id, lowest->product);
    printf("Total quantity sold          : %d\n", total_units(list));

    char product[MAX_PRODUCT];
    if (read_text("\nProduct for the revenue figure (Enter to skip): ",
                  product, MAX_PRODUCT, 1)) {
        double revenue = 0.0;
        int n = 0;
        for (int i = 0; i < list->count; i++) {
            if (filter_by_product(&list->items[i], product)) {
                revenue += list->items[i].total;
                n++;
            }
        }
        if (n == 0)
            printf("No transactions found for product \"%s\".\n", product);
        else
            printf("Revenue from %s: %.2f (%d transaction(s))\n", product, revenue, n);
    }
}

/* ================= File handling ================= */
/* File format (text):
 *   first line:  number of transactions
 *   per line:    id|product|quantity|unit_price|date
 * The total is not stored; it is recalculated when the file is loaded. */

int save_transactions(const TransactionList *list, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Error: cannot open %s for writing (%s).\n", filename, strerror(errno));
        return -1;
    }

    fprintf(fp, "%d\n", list->count);
    for (int i = 0; i < list->count; i++) {
        const Transaction *t = &list->items[i];
        fprintf(fp, "%d|%s|%d|%.10g|%s\n", t->id, t->product, t->quantity,
                t->unit_price, t->date);
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

/* Read one transaction line and add it to list. Returns 1 on success,
 * 0 if the line is invalid. */
static int read_transaction_record(FILE *fp, TransactionList *list)
{
    char line[512];
    Transaction t;
    int used = 0;

    if (fgets(line, sizeof line, fp) == NULL)
        return 0;
    if (sscanf(line, "%d|%63[^|]|%d|%lf|%10[^|\n]%n", &t.id, t.product, &t.quantity,
               &t.unit_price, t.date, &used) != 5)
        return 0;
    if (line[used] != '\n' && line[used] != '\0')
        return 0;                                  /* extra text after the date */
    if (t.id < 1 || t.quantity < 1 || !(t.unit_price >= 0.01) ||
        !is_valid_date(t.date) || find_transaction_index(list, t.id) != -1)
        return 0;

    t.total = t.quantity * t.unit_price;
    return append_transaction(list, &t) == 0;
}

/* Load transactions into a new list. On success the old contents of
 * list are replaced; on failure list is left unchanged. */
int load_transactions(TransactionList *list, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        if (errno == ENOENT)
            return 0;                              /* no file yet: not an error */
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

    TransactionList loaded;
    if (init_list(&loaded) != 0) {
        fclose(fp);
        printf("Error: out of memory while loading %s.\n", filename);
        return -1;
    }
    for (int i = 0; i < count; i++) {
        if (!read_transaction_record(fp, &loaded)) {
            fclose(fp);
            free_list(&loaded);
            printf("Error: file %s is corrupted (problem at transaction %d).\n",
                   filename, i + 1);
            return -1;
        }
    }
    fclose(fp);

    free_list(list);
    *list = loaded;
    return 1;
}
