# Technical Report: Sales Transaction Processing and Analysis System (Project 4)

## 1. Objective

Record sales transactions for a small business and analyse them, with the main goal of showing **function pointers** and **callback functions** in C. The program also uses dynamic memory, hand-written search and sort, a report, and file persistence.

## 2. Program structure

```
main.c     menu loop, load at start-up, save at exit
   │
sales.c    input validation → list handling → add/update/delete
   │       → function pointers (operations) → callbacks (filter, compare)
   │       → search → sort → report → save/load
sales.h    structures, the three function pointer types, declarations
```

## 3. Structures and types

```c
typedef struct { int id; char product[64]; int quantity;
                 double unit_price; double total; char date[11]; } Transaction;

typedef struct { Transaction *items; int count; int capacity; } TransactionList;

typedef void (*SalesOperation)(const TransactionList *list);
typedef int  (*FilterFunc)(const Transaction *t, const void *param);
typedef int  (*CompareFunc)(const Transaction *a, const Transaction *b);
```

`TransactionList` is a dynamic array that keeps both how many transactions are stored (`count`) and how many fit (`capacity`). The three `typedef` lines name the three kinds of function pointer used in the program.

## 4. Function pointers: choosing an operation at runtime

`process_sales_data()` (menu option 5) holds a table that pairs each operation's name with a pointer to its function:

```c
static const struct { const char *name; SalesOperation run; } operations[] = {
    { "Calculate total revenue",               op_total_revenue },
    { "Calculate average transaction value",   op_average_value },
    { "Identify the highest-value transaction", op_highest },
    { "Identify the lowest-value transaction", op_lowest },
    { "Calculate total units sold",            op_total_units },
};
...
SalesOperation run = operations[choice - 1].run;   /* chosen at runtime */
run(list);
```

The program does not know which function will run until the user chooses. Adding a new operation means writing one function and adding one line to the table; the menu code does not change. This is what function pointers are for.

## 5. Callbacks: passing functions as arguments

A callback is a function passed as an argument to another function, which calls it. There are two kinds.

**Filter callbacks (`FilterFunc`).** They answer "does this transaction match?" with 1 or 0. The extra value to test against (an amount, a product name, a quantity) arrives as a `const void *param`, which each filter converts back to its real type.

| Callback | Matches when |
|---|---|
| `filter_above_amount` | the transaction total is above the amount |
| `filter_by_product` | the product name equals the given name (ignoring case) |
| `filter_quantity_at_least` | the quantity is at least the given number |

Two general functions take any filter and call it for every transaction:

- `display_matching(list, filter, param)` shows the matches and returns how many (`if (filter(&list->items[i], param))`).
- `count_matching(list, filter, param)` only counts them.

**Compare callbacks (`CompareFunc`).** They return a negative number, zero or a positive number, like `strcmp`. `sort_transactions(list, compare)` is a bubble sort that calls `compare()` to decide whether two neighbours must swap, so one sort function handles every ordering:

| Callback | Order |
|---|---|
| `compare_by_id` | ID, smallest first |
| `compare_by_product` | product name A-Z, ignoring case |
| `compare_by_total_ascending` | total, lowest first |
| `compare_by_total_descending` | total, highest first (calls the ascending one with its arguments swapped) |

`sort_menu()` picks the compare function from the user's choice (`CompareFunc compare = compare_by_id; ... compare = compare_by_product;`) and passes it to `sort_transactions()`. The sales report also reuses `filter_by_product` to work out the revenue of one product.

The `void *` parameter lets one function type serve filters with different kinds of values. The price of this is that the compiler cannot check the type, so each filter must convert it correctly.

## 6. Dynamic memory

| Step | Function | Detail |
|---|---|---|
| Start | `init_list()` | `malloc` for 4 transactions (`INITIAL_CAPACITY`) |
| Grow | `append_transaction()` | When `count == capacity`, `realloc` to **double** the capacity |
| Free | `free_list()` | Called at exit and when a load replaces the list |

Doubling means that adding n transactions causes only about log2(n) resizes. As in Project 3, the `realloc` result goes into a temporary pointer first, so a failure cannot lose the existing data. Deleting a transaction shifts the later ones down and lowers `count`; the capacity stays, so a delete never needs to allocate.

## 7. Searching, sorting and calculations

- **Search** is a linear scan: by ID (exact match), by product (part of the name, ignoring case) and by amount (equal to the nearest cent, because totals are decimals).
- **Sort** is the bubble sort described above. It swaps only when strictly out of order, so equal items keep their order. `qsort()` is not used.
- **Totals** are calculated as quantity x unit price when a transaction is added or updated. Revenue is their sum, the average is revenue divided by the count (0 when the list is empty, so no division by zero), and the highest and lowest come from `find_highest()` and `find_lowest()`.

## 8. Validation and file handling

All keyboard input is validated as it is typed, repeating until valid: ID (positive whole number, not already used), product (not empty, no `|`), quantity (1 or more), unit price (0.01 or more), and date (a real calendar date as `YYYY-MM-DD`, with leap years handled by `is_valid_date()`).

The file `transactions.txt` has the count on the first line and then `id|product|quantity|unit_price|date`. The **total is not stored**; it is recalculated when the file is loaded, so it can never disagree with the quantity and price. `load_transactions()` reads into a **new list** and replaces the current one only if everything loaded correctly, so a failed load (menu option 11) leaves the current data unchanged. A missing file is normal on the first run. A damaged file at start-up (bad header, duplicate ID, bad date, bad quantity or price, extra text, truncated) is reported and the program stops, so it never overwrites a bad file with an empty one.

## 9. Required concepts and where they are

| Concept | Location (`sales.c` unless stated) |
|---|---|
| Structures | `sales.h` lines 14-29 |
| Function pointer types | `sales.h` lines 31-33 |
| Function pointers chosen at runtime | `process_sales_data()` (table at line 404, call at lines 421-422) |
| Callback invoked by another function | `display_matching()` and `count_matching()` (lines 445-460), `sort_transactions()` (line 502) |
| Callback chosen at runtime | `sort_menu()` (lines 618-626), `filter_menu()` (line 513) |
| `malloc` / `realloc` / `free` | `init_list()` (178), `append_transaction()` (202), `free_list()` (190) |
| Manual search | `search_menu()` and the three `search_by_...` functions (lines 540-591) |
| Manual sort | `sort_transactions()` (line 498) |
| Division-by-zero guard | `average_value()` (line 339) |
| Save and load | `save_transactions()` (line 677), `load_transactions()` (line 727) |

## 10. Testing

All of these were run and passed: adding 8 and then 40 transactions (so the array grew several times); update and delete; duplicate IDs; invalid IDs, quantities, prices, dates and text; all five operations through function pointers; the three filters, with and without matches; the three searches; the four sorts; save, quit, restart; load after unsaved changes; a failed load that left the data unchanged; every option on an empty list; missing file; eight kinds of damaged file; and valgrind runs with no leaks or errors. A save failure and an allocation failure are handled in the code but could not be triggered here, so they are untested.

## 11. Limitations

- Money is stored as `double`. That is fine here (amounts are compared to the nearest cent), but a real accounting system would store whole cents.
- The `void *` parameter of filters is not type-checked by the compiler.
- Bubble sort is slow for very large lists.
- Updating a transaction re-enters all its details except the ID.
- The `|` character cannot be used in a product name because it separates the fields in the file.
