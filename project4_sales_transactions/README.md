# Project 4: Sales Transaction Processing and Analysis System (C)

A menu-driven C program for recording and analysing the sales transactions of a small business. It shows how **function pointers** and **callback functions** can select and run different operations at runtime, with transactions kept in dynamically allocated memory and saved to a file.

## Files

| File | Purpose |
|---|---|
| `sales.h` | Structures, function pointer types and function declarations |
| `sales.c` | All the logic: input validation, transactions, operations, callbacks, search, sort, report, file save/load |
| `main.c` | The menu and the program start-up and shut-down |
| `transactions.txt` | Sample data (8 transactions) in the program's save format |

## Build and run

```bash
gcc -Wall -Wextra -o sales_system main.c sales.c
./sales_system
```

The program loads `transactions.txt` from the current folder at start-up and saves it when you choose **10 (Save)** or **0 (Exit)**.

## Menu

```
===== SALES TRANSACTION MANAGEMENT SYSTEM =====
 1. Add New Transaction          7. Search Transactions
 2. Display All Transactions     8. Sort Transactions
 3. Update Transaction           9. Generate Sales Report
 4. Delete Transaction          10. Save Transactions
 5. Process Sales Data          11. Load Transactions
 6. Filter Transactions (Callbacks)
 0. Exit (transactions are saved automatically)
```

Option 11 replaces the transactions in memory with the saved file. If the file is missing or damaged, the current transactions are left unchanged.

## Structures

- `Transaction`: id, product, quantity, unit_price, total (quantity x unit_price), date (`YYYY-MM-DD`).
- `TransactionList`: `items` (pointer to a dynamic array of `Transaction`), `count` (how many are stored) and `capacity` (how many fit).

## Function pointers (option 5, Process Sales Data)

`sales.h` defines the type `SalesOperation`, a pointer to a function that takes the list and prints a result. In `process_sales_data()` a table pairs each operation name with its function:

| Operation | Function |
|---|---|
| Calculate total revenue | `op_total_revenue` |
| Calculate average transaction value | `op_average_value` |
| Identify the highest-value transaction | `op_highest` |
| Identify the lowest-value transaction | `op_lowest` |
| Calculate total units sold | `op_total_units` |

The user's choice picks an entry from the table, and the program calls it through the pointer: `SalesOperation run = operations[choice - 1].run; run(list);`.

## Callback functions

A callback is a function passed as an argument to another function, which then calls it.

| Callback type | Callbacks | Function that receives and calls it |
|---|---|---|
| `FilterFunc` | `filter_above_amount`, `filter_by_product`, `filter_quantity_at_least` | `display_matching()` (shows matches), `count_matching()` (counts matches) |
| `CompareFunc` | `compare_by_id`, `compare_by_product`, `compare_by_total_ascending`, `compare_by_total_descending` | `sort_transactions()` |

- Option 6 uses the filter callbacks: transactions above an amount, transactions for a product, and a count of transactions with a quantity of at least N. The extra value (amount, product name or quantity) is passed as a `const void *param`.
- Option 8 (`sort_menu()`) chooses a compare callback at runtime and passes it to `sort_transactions()`.
- The sales report also reuses `filter_by_product` for the product revenue figure.

## Dynamic memory

| Concept | Where |
|---|---|
| `malloc()` for the starting array (4 transactions) | `init_list()` |
| `realloc()` when the array is full (capacity doubles) | `append_transaction()`, used by both adding and loading |
| Allocation failure handling | Results of `malloc`/`realloc` are checked; the transaction is not added and the existing data stays safe |
| `free()` | `free_list()`, called at exit and when a load replaces the list |
| Pointers to records | `Transaction *t` in `update_transaction()`, `const Transaction *` in the callbacks and report, `TransactionList *list` everywhere |

## Searching and sorting (written by hand, no `qsort()`)

- **Search** (`search_menu()`): linear searches by ID (exact), product name (part of the name, ignoring case) and amount (equal to the nearest cent).
- **Sort** (`sort_transactions()`): bubble sort; the callback decides the order. The sort is stable, so equal items keep their order.

## Sales calculations

Total revenue is the sum of all totals; average value is revenue divided by the number of transactions (0 for an empty list, so no division by zero); highest and lowest values come from `find_highest()` and `find_lowest()`; total quantity is the sum of quantities; product revenue is the sum of the totals for transactions whose product name matches (ignoring case).

## File handling

Text file `transactions.txt`:

```
8                                         <- number of transactions
1|Laptop|2|850|2026-11-03                 <- id|product|quantity|unit_price|date
...
```

The total is not stored; it is recalculated when the file is loaded, so it can never disagree with quantity and price.

- A missing file is not an error: the program starts empty and creates the file on the first save.
- A damaged file (bad header, duplicate ID, bad date, quantity below 1, price below 0.01, extra text, truncated, unreadable) is reported and the program stops, so it never overwrites a bad file with empty data.
- Saving checks that the file opened and that writing and closing worked.
- The `|` character is not accepted in product names because it separates the fields in the file.

## Validation and error handling

| Check | Behaviour |
|---|---|
| Menu choice not a number from 0 to 11 | Message, asks again |
| Transaction ID not a positive whole number, or already used | Message / transaction not added |
| Product name empty, too long, or containing `\|` | Message, asks again |
| Quantity below 1 (negative, zero, or not a number) | Message, asks again |
| Unit price below 0.01 or not a number | Message, asks again |
| Date not a real calendar date `YYYY-MM-DD` (leap years handled) | Message, asks again |
| Update/delete/search for a transaction that does not exist | Message, nothing changes |
| Any operation, report or search on an empty list | "No transactions recorded yet." |
| Memory allocation failure, file open/read/write failure | Message; data left unchanged |
| Input closed (Ctrl+D) | Message and the program ends; changes since the last save are not saved |

## Sample output (from `transactions.txt`)

```
ID       Product                Qty      Unit price   Total          Date
1        Laptop                 2        850.00       1700.00        2026-11-03
2        Mouse                  10       15.50        155.00         2026-11-03
3        Keyboard               5        45.00        225.00         2026-11-05
4        Monitor                3        220.00       660.00         2026-11-07
5        Laptop                 1        850.00       850.00         2026-11-10
6        USB Cable              25       3.20         80.00          2026-11-12
7        Mouse                  4        15.50        62.00          2026-11-15
8        Headphones             6        59.99        359.94         2026-11-18
```

Option 5 (function pointer), choosing "Calculate total revenue", then "Identify the highest-value transaction":

```
Total revenue: 4091.94

Highest-value transaction:
ID       Product                Qty      Unit price   Total          Date
1        Laptop                 2        850.00       1700.00        2026-11-03
```

Option 6 (callback), filter "total above 200":

```
1        Laptop                 2        850.00       1700.00        2026-11-03
3        Keyboard               5        45.00        225.00         2026-11-05
4        Monitor                3        220.00       660.00         2026-11-07
5        Laptop                 1        850.00       850.00         2026-11-10
8        Headphones             6        59.99        359.94         2026-11-18
5 transaction(s) match.
```

Option 9, sales report (product "Laptop"):

```
===== SALES REPORT =====
Total number of transactions : 8
Total revenue                : 4091.94
Average transaction value    : 511.49
Highest transaction value    : 1700.00 (ID 1, Laptop)
Lowest transaction value     : 62.00 (ID 7, Mouse)
Total quantity sold          : 56
Revenue from Laptop: 2550.00 (2 transaction(s))
```

These figures were checked by hand (for example, 1700 + 155 + 225 + 660 + 850 + 80 + 62 + 359.94 = 4091.94).

## Testing

All tests below were run and passed.

| Test | Expected | Result |
|---|---|---|
| Add 8 transactions (the array grows from 4 slots) | All stored, totals correct | Pass |
| Add 40 transactions (several growth steps) | 40 stored, revenue 1230.00, 820 units, reload gives 40 | Pass |
| Update, delete (middle, first, unknown ID) | Records changed or removed; file updated on save | Pass |
| Duplicate ID | Rejected | Pass |
| Invalid menu choices (`abc`, `12`, `-1`) | Rejected | Pass |
| Invalid ID, quantity, price, date (`0`, `abc`, `-3`, `-2`, `2026-02-30`, `21-11-2026`, `2026-13-01`) | Rejected, asks again | Pass |
| Valid leap-year date `2024-02-29` | Accepted | Pass |
| Empty product name; name containing `\|` | Rejected | Pass |
| All 5 operations through function pointers; invalid operation choice | Correct results; invalid choice rejected | Pass |
| Filters by amount, product, quantity (with and without matches) | Correct matches and counts | Pass |
| Search by ID, product (part of name), amount (with and without matches) | Correct matches / "not found" | Pass |
| Sort by ID, product, total ascending, total descending | Correct order each time | Pass |
| Save, quit, restart | Same transactions loaded back | Pass |
| Option 11 (load) after unsaved changes | Unsaved changes discarded, saved data shown | Pass |
| Option 11 when the file is damaged while the program runs | "Load failed", current data unchanged | Pass |
| Empty list: every option | Clear message, no crash, no division by zero | Pass |
| Missing data file | Starts empty | Pass |
| Damaged data file (8 kinds: bad header, duplicate ID, bad date, negative quantity, zero price, extra text, truncated, a folder named `transactions.txt`) | Error message, program stops, nothing overwritten | Pass |
| Memory check with valgrind (growth, CRUD, search/sort sessions) | No leaks or errors | Pass |

Not tested: a failure while saving (for example a full or read-only disk) and a memory allocation failure. The code handles both, but they could not be triggered in the test environment.
