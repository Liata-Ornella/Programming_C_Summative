# Technical Report: Multi-threaded Document Search and Analysis System (Project 5)

## 1. Objective

Search several text files for a keyword at the same time using POSIX threads. Each file is handled by its own thread, which counts lines, counts keyword occurrences (ignoring upper/lower case), records which lines match, and writes a separate report file. The main program shows the status of every thread and waits for all of them to finish.

## 2. Program flow

```
main()
 ├─ check arguments (keyword not empty, at least one file)
 ├─ calloc one ThreadArgs record per file
 ├─ for each file:  init_thread_args()  →  pthread_create(process_file)
 │                                           │
 │        runs at the same time              ▼
 │     ┌─────────────────────────── process_file (one thread per file) ──┐
 │     │ open file → read line by line (getline)                         │
 │     │   count_occurrences() per line → totals, matching line numbers  │
 │     │ write_report()  → <name>_report.txt                             │
 │     │ print "Successfully processed" or the error                     │
 │     └──────────────────────────────────────────────────────────────────┘
 ├─ for each created thread:  pthread_join()
 ├─ display_summary()
 └─ free memory, exit code 0 (all succeeded) or 1
```

## 3. Data structure

`ThreadArgs` carries everything one thread needs and everything it produces:

| Field | Direction | Meaning |
|---|---|---|
| `id` | in | Thread number shown to the user (1, 2, 3 ...) |
| `filename`, `keyword` | in | Point into `argv` (they are never changed) |
| `report_name` | in | Output file name, built by `build_report_name()` |
| `status`, `success` | out | Status text for the report and the summary; 1 or 0 |
| `total_lines`, `total_occurrences`, `matching_lines` | out | Counters |
| `tid`, `created` | main only | The `pthread_t` and whether `pthread_create` succeeded |

`main()` allocates an array of these (`calloc`) and gives each thread a pointer to its own element, so the threads never share a record.

## 4. Why no thread synchronization is needed

- Each thread reads and writes **only its own** `ThreadArgs` element, its own local variables and buffers, and its own report file.
- The keyword and file names are read-only.
- `main()` reads a thread's results only **after** `pthread_join()` on that thread, so it never reads a value while the thread is still changing it.
- The only shared resource is the terminal. Each `printf` call is thread-safe in the C library (each call locks the stream while it runs), so a line is never mixed with another thread's line. The *order* of lines from different threads varies from run to run, which is expected and is the visible proof that the threads run concurrently.

## 5. Key algorithms

**Counting occurrences (`count_occurrences`).** For each position in the line, compare the keyword with the text character by character using `tolower()` on both sides. On a match, add one to the count and skip past the keyword (matches do not overlap: "aaa" contains "aa" once). On a mismatch, move forward one character.

**Reading lines (`process_file`).** `getline()` reads lines of any length, so there is no fixed line-size limit. It also returns a final line that has no newline at the end, so that line is counted. An empty file gives 0 lines.

**Matching line numbers.** When a line has at least one match, its number is appended to a dynamic array of `long`. The array starts empty and doubles in size (16, 32, 64 ...) with `realloc()` when full.

**Report names.** `build_report_name()` removes any directory and the extension, then adds `_report.txt` (`data/document1.txt` becomes `document1_report.txt` in the current directory).

## 6. Memory management

| Memory | Allocated | Freed |
|---|---|---|
| Thread records | `calloc` in `main()` | `free` at the end of `main()` |
| Line buffer for `getline` | by `getline` | `free(buf)` after the loop |
| Matching line numbers | `realloc` in `process_file` | `free(lines)` after the report is written |

Every `realloc` result goes into a temporary pointer first, so the old array is not lost if it fails; the thread then reports "Failed: out of memory". A memory check with valgrind found no leaks or errors.

## 7. Error handling

| Situation | What happens |
|---|---|
| No arguments, or an empty keyword | Usage or error message, exit code 1 |
| Input file missing or unreadable | The thread prints the error and still writes a report with the failed status; the other threads continue |
| Output file cannot be created or written | Status "Failed: cannot write report file" |
| Keyword not found | Counted as success; the report says "Success (keyword not found)" and "none" for line numbers |
| `pthread_create` fails | Message from `main()`; that file is marked failed and is **not** joined; threads that were created are all joined |
| Out of memory | Status "Failed: out of memory" |

The exit code is 0 only if every file succeeded.

## 8. Required concepts and where they are

| Concept | Location in `document_analyzer.c` |
|---|---|
| POSIX threads, one per file | `pthread_create` in `main()` (line 241) |
| Passing data through a structure | `ThreadArgs` (line 26), passed as the thread argument |
| `pthread_join()` | `main()` (line 254) |
| Case-insensitive search | `count_occurrences()` (line 44), `tolower()` at line 53 |
| Per-thread report | `write_report()` (line 93) |
| Thread status messages | `process_file()` (line 122) and `main()` |
| Modular functions | `init_thread_args`, `process_file`, `count_occurrences`, `write_report`, `display_summary` |

## 9. Testing

All of these were run and passed: a normal run with 3 files (6, 0 and 5 occurrences, checked by hand); upper/lower case variants; keyword not found; last line without a newline; empty file; missing file; report that cannot be written; no arguments; empty keyword; 8 files at once; and a valgrind check with no leaks. Thread-creation failure is handled in the code but could not be triggered on a normal system, so it is untested.

## 10. Limitations

- One thread per file with no upper limit; a very large number of files creates a very large number of threads.
- If two input files have the same base name in different folders (`a/doc.txt` and `b/doc.txt`), both threads would write the same report name, and one report would overwrite the other. The assessment's example uses distinct names, so this was left as is.
- Case-insensitive matching covers ordinary (ASCII) letters only.
- A keyword is matched within a single line, never across a line break.
