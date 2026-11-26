# Project 5: Multi-threaded Document Search and Analysis System (C, pthreads)

A command-line program that searches several text files for a keyword at the same time. One POSIX thread is created per input file. Each thread counts lines, counts keyword occurrences, records the matching line numbers, and writes its own report file.

## Files

| File | Purpose |
|---|---|
| `document_analyzer.c` | Complete source code (single file) |
| `document1.txt`, `document2.txt`, `document3.txt` | Sample input documents |
| `document1_report.txt`, `document2_report.txt`, `document3_report.txt` | Sample generated reports |

## Build and run

```bash
gcc -Wall -Wextra -pthread -o document_analyzer document_analyzer.c
./document_analyzer "IoT" document1.txt document2.txt document3.txt
```

Usage: `./document_analyzer "keyword" file1 [file2 ...]`

The exit code is `0` if every file was processed, `1` if there was a usage error or any file failed.

## How it works

1. `main()` validates the arguments and allocates one `ThreadArgs` record per file with `calloc()`.
2. For each file, `init_thread_args()` fills the record and `pthread_create()` starts a thread running `process_file()`.
3. Each thread reads its file line by line, counts occurrences of the keyword (case-insensitive), stores the matching line numbers in a dynamic array, and writes `<name>_report.txt`.
4. `main()` calls `pthread_join()` on every thread that was created, then `display_summary()` prints the results.

No mutex is needed. Each thread only reads and writes its own `ThreadArgs` record and its own report file, so no data is shared between threads.

## Code structure

| Function | Responsibility |
|---|---|
| `init_thread_args()` | Initialize one thread's argument record and build its report file name |
| `process_file()` | Thread function: read the file, search, write the report, set the status |
| `count_occurrences()` | Count case-insensitive, non-overlapping matches of the keyword in one line |
| `write_report()` | Write the per-file report |
| `display_summary()` | Print the final processing summary on the terminal |
| `build_report_name()` | Turn `dir/name.txt` into `name_report.txt` |

The `ThreadArgs` structure holds the input filename, keyword, report name, status text, the three counters, and the `pthread_t`.

## Requirement coverage

| Requirement | Where |
|---|---|
| One thread per file | `main()`, `pthread_create()` loop |
| Filename and keyword passed through a structure | `ThreadArgs`, passed as the thread argument |
| `pthread_join()` before exit | `main()`, second loop |
| Total lines, occurrences, matching lines, line numbers | `process_file()` |
| Case-insensitive search | `count_occurrences()` using `tolower()` |
| Separate report per thread | `write_report()` |
| Report contents (file, keyword, lines, occurrences, matching lines, line numbers, status) | `write_report()` |
| Thread creation, processing, success and error messages | `main()` and `process_file()` |
| Modular functions | See the table above |

## Error handling

| Situation | Behaviour |
|---|---|
| Missing or unreadable input file | Thread prints the error; a report with the failed status is still written; other threads are unaffected |
| Report file cannot be written | Status becomes "Failed: cannot write report file" |
| Keyword not found | Treated as success; the report says "Success (keyword not found)" and lists "none" |
| Thread creation fails | Error printed; that file is marked failed and is not joined; the other threads still run and are joined |
| Out of memory | Status "Failed: out of memory"; memory freed |
| No arguments or empty keyword | Usage or error message, exit code 1 |

## Sample output

```
===== MULTI-THREADED DOCUMENT ANALYZER =====

Search Keyword: IoT
Number of Input Files: 3

[Main] Created thread 1 for document1.txt
[Main] Created thread 2 for document2.txt
[Main] Created thread 3 for document3.txt
[Thread 3] Processing document3.txt ...
[Thread 1] Processing document1.txt ...
[Thread 3] Successfully processed document3.txt
[Thread 1] Successfully processed document1.txt
[Thread 2] Processing document2.txt ...
[Thread 2] Successfully processed document2.txt

===== PROCESSING SUMMARY =====

document1.txt: Completed (6 occurrences in 4 lines, report: document1_report.txt)
document2.txt: Completed (0 occurrences in 0 lines, report: document2_report.txt)
document3.txt: Completed (5 occurrences in 4 lines, report: document3_report.txt)

All processing tasks completed.
Reports generated successfully.
```

The order of the thread messages differs from run to run because the threads run concurrently.

Sample report (`document1_report.txt`):

```
===== DOCUMENT ANALYSIS REPORT =====
Input file          : document1.txt
Search keyword      : IoT
Total lines         : 6
Total occurrences   : 6
Matching lines      : 4
Matching line numbers: 1, 3, 4, 6
Status              : Success
```

## Testing

| Test | Expected | Result |
|---|---|---|
| Normal run, 3 files | 6, 0 and 5 occurrences; 3 reports written | Pass |
| Case variants (IoT, iot, IOT, iOt) | All counted | Pass |
| Keyword not found | Success, 0 matches, "none" | Pass |
| Last line without trailing newline | Counted | Pass |
| Empty file | 0 lines, success | Pass |
| Missing file | Error shown; failed report written; others succeed | Pass |
| Report cannot be written (a directory has the report's name) | "cannot write report file" | Pass |
| No arguments / empty keyword | Usage or error message, exit code 1 | Pass |
| 8 files at once | All complete correctly | Pass |
| Memory leak check (valgrind) | No leaks or errors | Pass |

Not tested: thread creation failure. The code handles it, but it cannot easily be triggered on a normal system.
