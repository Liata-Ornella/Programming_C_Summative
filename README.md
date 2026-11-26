# C Summative Assessment: Programming Projects

One folder per project. Projects 2-5 include source code, a `README.md` (how to build, run and test), and a `TECHNICAL_REPORT.md` (design and implementation). Project 1 includes its Arduino source.

| Folder | Project | Language | Main concepts |
|---|---|---|---|
| `project2_linux_monitor/` | Linux System Health and Resource Monitoring Manager | Bash | Menu, functions, `/proc`, `ps`, `top`, `free`, `df`, process management, logging, input validation |
| `project3_student_records/` | Student Academic Records and Performance Analytics | C | Structures, `malloc`/`realloc`/`free`, manual search and sort, file persistence |
| `project4_sales_transactions/` | Sales Transaction Processing and Analysis | C | Function pointers, callbacks, dynamic array, manual search and sort, file persistence |
| `project5_document_analyzer/` | Multi-threaded Document Search and Analysis | C (pthreads) | One thread per file, `pthread_create`/`pthread_join`, case-insensitive search, per-thread reports |

**Not included:** Project 1 (Smart Occupancy Monitoring, Arduino/Tinkercad). It needs the Tinkercad simulator.

## Build and run

```bash
# Project 2
cd project2_linux_monitor && chmod +x health_monitor.sh && ./health_monitor.sh

# Project 3
cd project3_student_records && gcc -Wall -Wextra -o student_system main.c student.c && ./student_system

# Project 4
cd project4_sales_transactions && gcc -Wall -Wextra -o sales_system main.c sales.c && ./sales_system

# Project 5
cd project5_document_analyzer && gcc -Wall -Wextra -pthread -o document_analyzer document_analyzer.c
./document_analyzer "IoT" document1.txt document2.txt document3.txt
```

## Deliverables checklist

| Deliverable | P2 | P3 | P4 | P5 |
|---|---|---|---|---|
| Source code | `health_monitor.sh` | `.c` and `.h` | `.c` and `.h` | `.c` |
| Sample data or input | (live system) | `students.txt` | `transactions.txt` | `document1-3.txt` |
| Sample output or reports | `evidence/` | in `README.md` | in `README.md` | `document*_report.txt` |
| Sample generated report and activity log | `evidence/` | n/a | n/a | n/a |
| Execution evidence | `evidence/*.txt` | tests in `README.md` | tests in `README.md` | tests in `README.md` |
| Technical documentation | `TECHNICAL_REPORT.md` | `TECHNICAL_REPORT.md` | `TECHNICAL_REPORT.md` | `TECHNICAL_REPORT.md` |

**Still for you to do:**

- Project 2 asks for terminal **screenshots** (CPU, memory, disk, process searching and management). Run the script on your own Linux machine and take them there; the files in `evidence/` are text captures from a test machine.
- The rubric section of the assessment document was empty, so the work is mapped to the requirements, not to marks.
- Keep the GitHub repository public until grades are published.

## Testing status

Every project was compiled with `-Wall -Wextra` (C) or syntax-checked (Bash) and tested with normal, boundary and invalid input. The C programs were also run under valgrind with no leaks or errors. Things that could not be triggered in the test environment (a full disk while saving, a memory allocation failure, a thread creation failure) are handled in the code but untested; each project's `README.md` lists these. All tests ran on Ubuntu 24.04.
