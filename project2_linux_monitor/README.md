# Project 2: Linux System Health and Resource Monitoring Manager (Bash)

An interactive shell script that monitors CPU, memory, disk and processes on a Linux system, lets the user search for and safely terminate processes, saves timestamped health reports, and keeps an activity log of everything it does.

## Files

| File | Purpose |
|---|---|
| `health_monitor.sh` | The complete script |
| `evidence/` | Real captured runs, a sample health report and a sample activity log (see "Evidence" below) |
| `TECHNICAL_REPORT.md` | How the script is built and why |

## Run

```bash
chmod +x health_monitor.sh
./health_monitor.sh
```

Needs only standard Linux tools (`ps`, `top`, `free`, `df`, `uptime`, `awk` and similar). The activity log is `activity.log` next to the script. To use another location: `HEALTH_MONITOR_LOG=/path/to/file.log ./health_monitor.sh`.

## Menu

```
 1. Display system information        6. Terminate a process
 2. Monitor CPU utilization           7. Generate a system health report
 3. Monitor memory utilization        8. View activity history
 4. Monitor disk utilization          9. Clear activity log
 5. View and search running processes 10. Exit
```

## What each option does

| Option | Behaviour |
|---|---|
| 1 | Hostname, operating system (`/etc/os-release`), kernel version (`uname -r`), uptime, current date and time |
| 2 | Asks for a CPU threshold, measures overall CPU usage over one second from `/proc/stat`, shows cores and load average, lists the top 5 processes by CPU right now, and flags every process above the threshold |
| 3 | Asks for a memory threshold, shows total, used and available memory (MB) and the utilization percentage |
| 4 | Asks for a disk threshold, shows size/used/available for each mounted filesystem and warns about filesystems at or above the threshold |
| 5 | Sub-menu: list running processes (top 15 by CPU) or search by name (part of a name, any case) or by PID |
| 6 | Asks for a PID, shows the process, refuses protected processes, checks permission, asks for confirmation, then terminates it |
| 7 | Asks for a directory, then saves a report with a timestamp in the file name and the report body |
| 8 | Shows the activity log (the last 50 entries) |
| 9 | Clears the log after confirmation |
| 10 | Exits |

Thresholds are whole numbers from 1 to 100. Pressing Enter keeps the current value (default 80%). A warning appears when CPU or memory utilization is above the threshold, and when a disk is at or above it ("approaching capacity").

## Calculations

- **CPU usage**: two readings of `/proc/stat` one second apart. Usage = (total time - idle time) / total time x 100.
- **Memory**: used = total - available (from `free -m`); utilization = used / total x 100.
- **Disk**: the `Use%` column of `df -P`.
- Filesystems of type `tmpfs`, `devtmpfs` and `squashfs` are left out because they are not real disks.

## Process protection and permissions

A process is **refused** if any of these is true:

- its PID is 1 or 2
- it is the script itself or the shell that started it
- it is a kernel thread (parent PID 2)
- its name is in `PROTECTED_NAMES` (`init`, `systemd`, `sshd`, `dbus-daemon`, `cron`, `rsyslogd`, `NetworkManager`, `login`, `agetty` and a few more)

If the signal cannot be sent (the process belongs to another user), the script says the user lacks permission. After confirmation it sends a normal termination signal (`kill`, SIGTERM) and checks one second later whether the process ended. If the process ignores the signal, the script says it is still running.

## Activity log

Format: `YYYY-MM-DD HH:MM:SS [LEVEL] message`, with levels `INFO`, `WARNING` and `ERROR`. It records start and exit, every monitoring result, every warning, process searches and terminations, refused terminations, report generation and errors. After the log is cleared it contains one entry saying so.

## Input validation and error handling

| Situation | Behaviour |
|---|---|
| Menu choice not a number from 1 to 10 (letters, 0, 11, negative, empty, huge numbers) | Message, menu shown again |
| Threshold not a whole number from 1 to 100 | Message, asks again |
| PID not a positive whole number, or no such process | Message, nothing happens |
| Protected process | Refused with the reason, logged as a warning |
| No permission to signal a process | Message, logged as an error |
| Termination or log clearing | Asks y/n; any other answer is asked again |
| A needed command is missing (for example `free` or `df`) | The option reports which command is missing and returns to the menu; the CPU monitor falls back to `ps` if `top` is missing |
| Report directory missing | Asks whether to create it |
| Report path is a file, not a directory | Message |
| No permission to create or write in the report directory | Message |
| The log cannot be written when the script starts | Message and exit (nothing could be recorded) |
| Input closed (Ctrl+D) or Ctrl+C | Logged and the script exits cleanly |

## Evidence

All files in `evidence/` come from real runs of the final script, typed into the script as a terminal session (the main menu is shown only the first time, because it reprints after every action).

| File | Shows |
|---|---|
| `01_full_run.txt` | System information, CPU, memory (with a warning), disk, process listing and search, terminating a process, saving a report, viewing the log |
| `02_threshold_validation_and_warnings.txt` | Invalid thresholds rejected; CPU, memory and disk warnings with a CPU-heavy process running |
| `03_process_search_and_termination.txt` | Process listing, searching by name and PID, not found, invalid input, refusing protected processes, cancelling and confirming a termination |
| `04_error_handling_and_permissions.txt` | Invalid menu input, report-directory errors, clearing the log, permission errors as an unprivileged user, missing utilities, unwritable log, Ctrl+D |
| `health_report_20261122_191330.txt` | Sample generated system health report |
| `sample_activity.log` | Sample activity log from the full run |

The samples come from an Ubuntu 24.04 test machine (hostname `vm`), so the disks, memory and processes are that machine's. **Run the script on your own Linux machine and take the terminal screenshots there**; the assessment asks for screenshots of CPU, memory and disk monitoring and of process searching and management.

## Testing

All tests below were run and passed.

| Test | Expected | Result |
|---|---|---|
| System information | Hostname, OS, kernel, uptime, date and time | Pass |
| CPU monitor at default threshold | Usage, cores, load, top 5, "OK" | Pass |
| CPU monitor with one process using 100% of a core | About 50% overall (2 cores), warning, that process flagged | Pass |
| Memory and disk monitors, normal and with a low threshold | Figures shown; warnings only when at or above the threshold | Pass |
| Invalid thresholds (`abc`, `0`, `101`, `1000`, `-5`, `5.5`) | Rejected, asks again | Pass |
| List processes; search by name, by PID, not found, empty search, invalid sub-menu choice | Correct matches or messages | Pass |
| Terminate: invalid PID (`abc`, `0`), unknown PID | Rejected | Pass |
| Terminate: PID 1, the script, its parent, a process named `sshd` | Refused with the reason | Pass |
| Terminate: answer `maybe`, then `n`, then confirm `y` | Asked again; cancelled; process ended | Pass |
| Terminate a root-owned process as an unprivileged user | "No permission" | Pass |
| Terminate a process that ignores the signal | "Still running" warning | Pass |
| Report to an existing directory, a missing directory (create `y` and `n`), a file path, `~/dir`, an unwritable directory | Saved with timestamped name, or clear error | Pass |
| View log; clear log with `maybe`, `n`, `y`; log with more than 50 entries | Correct output; "last 50" shown | Pass |
| Invalid menu choices (`abc`, `0`, `11`, `-1`, empty, `99999999999`) | Rejected | Pass |
| Commands `free`, `df`, `top` removed from the PATH | Message naming the missing command; CPU monitor still works | Pass |
| Log not writable at start-up | Message and exit code 1 | Pass |
| Ctrl+D and Ctrl+C | Clean exit, logged | Pass |

Not tested: other Linux distributions or older versions of `ps`/`top` (only Ubuntu 24.04 was available), and a static check with `shellcheck` (not installed in the test environment).

## Known limitations

- If `top` is missing, the process CPU figures come from `ps`, which averages over the whole life of each process instead of showing current use.
- A user who can run the script as root can terminate any process that is not on the protected list (after confirmation). The list is short on purpose and is the `PROTECTED_NAMES` variable at the top of the script.
- The activity log shows the last 50 entries; the whole file is `activity.log`.
