# Technical Report: Linux System Health and Resource Monitoring Manager (Project 2)

## 1. Objective

An interactive Bash script that monitors CPU, memory, disk and processes, warns when usage passes a user-set threshold, lets the user search for and safely terminate processes, saves system health reports, and records every action in an activity log. It uses only standard Linux commands.

## 2. Structure

```
main()  ── menu loop: read choice, validate, call one function
  │
  ├─ Menu functions      show_system_info  monitor_cpu  monitor_memory  monitor_disk
  │                      process_menu / search_process  terminate_process
  │                      generate_report  view_log  clear_log
  │
  ├─ Section printers    print_cpu_section  print_memory_section  print_disk_section
  │   (used by BOTH the monitors and the report, so the code is written once)
  │
  ├─ Data collectors     get_cpu_usage  get_memory_stats  list_processes
  │                      sample_cpu_processes  get_uptime
  │
  └─ Helpers             log_action  ask  confirm  ask_threshold
                         require_commands  is_above  check_threshold  protection_reason
```

The same `print_*_section` functions serve the monitoring options and the health report: option 7 simply runs them with the output redirected to a file. This avoids repeating the code, as the assessment asks.

## 3. Commands used

| Command | Used for |
|---|---|
| `hostname`, `uname -r`, `uptime -p`, `date` | System information, timestamps |
| `/etc/os-release` | Operating system name |
| `/proc/stat` | CPU time counters, for the CPU usage percentage |
| `/proc/loadavg`, `nproc` | Load average, number of cores |
| `top -b -n 2` | Current CPU use per process |
| `free -m` | Total and available memory |
| `df -hP`, `df -P` | Disk table, and disk percentages for the threshold check |
| `ps -eo ...`, `ps -p` | Process list, search and details |
| `kill -0`, `kill` | Permission test, and termination |
| `awk`, `cut`, `tr`, `head`, `tail`, `wc`, `sleep`, `mkdir` | Calculations, formatting and file handling |

## 4. Key techniques

**CPU usage from `/proc/stat`.** The first line of `/proc/stat` holds counters of the time the CPUs have spent busy and idle since boot. `get_cpu_usage()` reads it, waits one second, reads it again, and works out (change in total - change in idle) / change in total x 100. One reading alone would only give the average since boot.

**Processes using the CPU right now.** `ps` shows each process's CPU averaged over its whole life, which hides a process that has just started to misbehave. `sample_cpu_processes()` therefore uses `top -b -n 2 -d 1` and keeps only the **second** snapshot, one second after the first, which is a real current value. If `top` is not installed, it falls back to `ps` and says nothing more, which is a known limitation.

**Memory.** `get_memory_stats()` reads the "available" column of `free -m` (it finds the column from the header, so it also works when the column order differs) and sets used = total - available. "Available" is what new programs can really use, so it is more honest than the "free" column.

**Thresholds.** `ask_threshold()` accepts a whole number from 1 to 100 (the pattern `^[0-9]{1,3}$` also rejects huge numbers) and Enter keeps the current value. `is_above()` compares decimal numbers with `awk`, because Bash arithmetic only handles whole numbers. CPU and memory warn when usage is *above* the threshold; a disk warns when it is *at or above* it ("approaching capacity").

**Disks.** `df` output for `tmpfs`, `devtmpfs` and `squashfs` is excluded because those are not real disks. The `Use%` column is read with `awk`, and mount points containing spaces are handled.

**Safe process termination (`terminate_process`).** The order of the checks matters:

1. The PID must be a positive number and the process must exist.
2. `protection_reason()` refuses PID 1 and 2, the script itself (`$$`), the shell that started it (`$PPID`), kernel threads (parent PID 2) and names on the `PROTECTED_NAMES` list.
3. `kill -0` sends no signal but tests whether the user is allowed to; if it fails and the process still exists, the user lacks permission.
4. The user must confirm with y/n (`confirm()` repeats until the answer is y or n).
5. `kill` sends SIGTERM, and one second later `ps -p` checks whether the process is gone. If it is still running, the script says so instead of claiming success.

**Logging.** `log_action LEVEL "message"` appends `YYYY-MM-DD HH:MM:SS [LEVEL] message` to the log. `check_threshold()` logs the measured value and, if it is over the limit, a second line at level `WARNING`.

**Input handling.** `ask()` is the only place that reads from the keyboard. It trims spaces and, if input is closed (Ctrl+D), logs it and exits cleanly instead of looping forever. `confirm()` and `ask_threshold()` are built on it.

**Other design decisions.**

- `export LC_ALL=C` at the top makes `free`, `df` and `top` print numbers and headings in the same format on every system, so the `awk` parsing works everywhere.
- Redirections are written as `2>/dev/null > "$file"` (error output first) so that Bash's own message about a file it cannot open is hidden and only the script's friendly message is shown.
- `set -e` is not used: many commands in a monitoring script are *expected* to fail sometimes (a missing process, a missing file), and each failure is handled explicitly.

## 5. Error handling

| Mechanism | Covers |
|---|---|
| `require_commands` | A missing utility: names it, logs an error, returns to the menu instead of crashing |
| Pattern checks (`[[ ... =~ ... ]]`) | Menu choices, thresholds and PIDs |
| `ps -p` result checks | Invalid PID or a process that ended |
| `protection_reason` and `kill -0` | Protected processes and missing permission |
| `confirm` | Termination and log clearing |
| Directory checks in `generate_report` | Missing directory (offers to create it), path that is a file, no permission to create or write |
| Start-up log check | A log that cannot be written: message and exit |
| `trap` on Ctrl+C, EOF check in `ask` | Clean exit, logged |

## 6. Required features and where they are (`health_monitor.sh`)

| Requirement | Location |
|---|---|
| System information | `show_system_info()` (line 231) |
| CPU monitoring, threshold, heavy processes | `monitor_cpu()` (249), `get_cpu_usage()` (101), `sample_cpu_processes()` (139) |
| Memory monitoring and percentage | `monitor_memory()` (279), `get_memory_stats()` (120) |
| Disk monitoring and threshold | `monitor_disk()` (287), `print_disk_section()` (196) |
| Process list and search by name or PID | `process_menu()` (322), `search_process()` (295) |
| Safe termination with confirmation | `terminate_process()` (372), `protection_reason()` (347) |
| Health report with timestamp, chosen directory | `generate_report()` (441), `build_report()` (427) |
| Activity log, view and clear | `log_action()` (24), `view_log()` (483), `clear_log()` (501) |
| Menu and validation | `print_menu()` (520), `main()` (535) |
| Reusable functions, variables, loops, conditionals, command substitution | Throughout, for example `for p in $PROTECTED_NAMES` (line 362) and `line=$(free -m | awk ...)` (line 122) |

## 7. Testing

All of these were run and passed: every menu option; threshold warnings for CPU (with a process using 100% of a core), memory and disk; invalid thresholds; process list and searches; invalid, unknown, protected and permission-denied terminations; cancelling and confirming a termination; a process that ignores the termination signal; report saving to an existing, missing, invalid, `~`-based and unwritable directory; log viewing, clearing and the 50-entry limit; invalid menu choices; missing `free`, `df` and `top`; an unwritable log; and Ctrl+D and Ctrl+C. Permission tests were run as the unprivileged user `nobody`. Not tested: other Linux distributions (only Ubuntu 24.04 was available) and a static check with `shellcheck` (not installed). The evidence files in `evidence/` contain the real runs.

## 8. Limitations

- Without `top`, per-process CPU figures are lifetime averages from `ps`.
- Run as root, the script can terminate any process that is not on the protected list, after confirmation.
- The protected list is short and fixed (the `PROTECTED_NAMES` variable); it is not a complete list of system services.
- The script targets Linux with GNU tools; other systems (macOS, BSD) are not supported.
