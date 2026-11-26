#!/bin/bash
# health_monitor.sh - Linux System Health and Resource Monitoring Manager
# Run with: ./health_monitor.sh

export LC_ALL=C   # same number and text format on every system

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_FILE="${HEALTH_MONITOR_LOG:-$SCRIPT_DIR/activity.log}"

# Default thresholds in percent (the user can change them while running)
CPU_THRESHOLD=80
MEM_THRESHOLD=80
DISK_THRESHOLD=80

# Processes that must never be terminated
PROTECTED_NAMES="init systemd systemd-journal systemd-logind systemd-udevd sshd dbus-daemon cron rsyslogd NetworkManager kthreadd login agetty"

# Filesystems that are not real disks
DF_EXCLUDE="-x tmpfs -x devtmpfs -x squashfs"

# ================= Logging =================

# Add one line to the activity log. Usage: log_action LEVEL "message"
log_action() {
    printf '%s [%s] %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$1" "$2" 2>/dev/null >> "$LOG_FILE"
}

# ================= Input helpers =================

# Show a prompt and read one line into a variable. Usage: ask "prompt" variable
ask() {
    local reply
    printf '%s' "$1"
    if ! read -r reply; then
        echo
        echo "Input closed. Exiting."
        log_action INFO "Application exited (input closed)"
        exit 0
    fi
    reply="${reply#"${reply%%[![:space:]]*}"}"   # remove spaces at the start
    reply="${reply%"${reply##*[![:space:]]}"}"   # remove spaces at the end
    printf -v "$2" '%s' "$reply"
}

# Ask a yes/no question. Returns 0 for yes and 1 for no.
confirm() {
    local answer
    while true; do
        ask "$1 (y/n): " answer
        case "${answer,,}" in
            y|yes) return 0 ;;
            n|no)  return 1 ;;
            *)     echo "Please answer y or n." ;;
        esac
    done
}

# Ask for a threshold from 1 to 100. Enter keeps the current value.
# Usage: ask_threshold "CPU" VARIABLE_NAME
ask_threshold() {
    local current="${!2}" input
    while true; do
        ask "Enter $1 threshold in % (1-100) [current $current, Enter to keep]: " input
        if [ -z "$input" ]; then
            return
        fi
        if [[ "$input" =~ ^[0-9]{1,3}$ ]] && [ "$input" -ge 1 ] && [ "$input" -le 100 ]; then
            printf -v "$2" '%s' "$((10#$input))"
            return
        fi
        echo "Error: enter a whole number from 1 to 100."
    done
}

# Check that commands exist. Returns 1 (with a message) if any is missing.
require_commands() {
    local missing="" cmd
    for cmd in "$@"; do
        command -v "$cmd" >/dev/null 2>&1 || missing="$missing $cmd"
    done
    if [ -n "$missing" ]; then
        echo "Error: required command(s) not available:$missing"
        log_action ERROR "Missing command(s):$missing"
        return 1
    fi
    return 0
}

# Return 0 if the first number is greater than the second (decimals allowed).
is_above() {
    awk -v a="$1" -v b="$2" 'BEGIN { exit !(a > b) }'
}

# ================= Data collection =================

get_uptime() {
    uptime -p 2>/dev/null || uptime
}

# Print overall CPU usage in %, measured over one second from /proc/stat.
get_cpu_usage() {
    local label u1 n1 s1 i1 w1 q1 sq1 st1 u2 n2 s2 i2 w2 q2 sq2 st2
    read -r label u1 n1 s1 i1 w1 q1 sq1 st1 _ < /proc/stat || return 1
    sleep 1
    read -r label u2 n2 s2 i2 w2 q2 sq2 st2 _ < /proc/stat || return 1

    local total1=$((u1 + n1 + s1 + i1 + w1 + q1 + sq1 + st1))
    local total2=$((u2 + n2 + s2 + i2 + w2 + q2 + sq2 + st2))
    local dtotal=$((total2 - total1))
    local didle=$(((i2 + w2) - (i1 + w1)))

    if [ "$dtotal" -le 0 ]; then
        echo "0.0"
        return 0
    fi
    awk -v t="$dtotal" -v i="$didle" 'BEGIN { printf "%.1f", (t - i) * 100 / t }'
}

# Set MEM_TOTAL, MEM_AVAILABLE, MEM_USED (in MB) and MEM_PERCENT.
get_memory_stats() {
    local line
    line=$(free -m | awk 'NR == 1 { for (i = 1; i <= NF; i++) if ($i == "available") col = i + 1 }
                          /^Mem:/ { if (!col) col = 4; print $2, $col }')
    read -r MEM_TOTAL MEM_AVAILABLE <<< "$line"
    if ! [[ "$MEM_TOTAL" =~ ^[0-9]+$ && "$MEM_AVAILABLE" =~ ^[0-9]+$ ]] || [ "$MEM_TOTAL" -eq 0 ]; then
        return 1
    fi
    MEM_USED=$((MEM_TOTAL - MEM_AVAILABLE))
    MEM_PERCENT=$(awk -v u="$MEM_USED" -v t="$MEM_TOTAL" 'BEGIN { printf "%.1f", u * 100 / t }')
}

# Print every process as: PID USER %CPU %MEM COMMAND (highest CPU first).
list_processes() {
    ps -eo pid=,user:20=,pcpu=,pmem=,comm= --sort=-pcpu
}

# Print processes with their CPU use right now (top takes two snapshots;
# the second one is the real current value).
sample_cpu_processes() {
    if command -v top >/dev/null 2>&1; then
        top -b -n 2 -d 1 -o %CPU -w 512 2>/dev/null |
            awk '/^top -/ { n++ } n == 2 && $1 ~ /^[0-9]+$/ { print $1, $2, $9, $10, $12 }'
    else
        list_processes
    fi
}

# Show lines of "PID USER %CPU %MEM COMMAND" (from stdin) as a table.
print_process_table() {
    printf '%-8s %-14s %-7s %-7s %s\n' "PID" "USER" "%CPU" "%MEM" "COMMAND"
    awk '{ printf "%-8s %-14.14s %-7s %-7s %s\n", $1, $2, $3, $4, $5 }'
}

# ================= Sections used by the monitors and the report =================

# Log the value and print OK or WARNING. Usage: check_threshold NAME VALUE LIMIT
check_threshold() {
    local name="$1" value="$2" limit="$3"
    log_action INFO "$name ${value}% (threshold ${limit}%)"
    if is_above "$value" "$limit"; then
        echo "WARNING      : $name is above the ${limit}% threshold!"
        log_action WARNING "$name ${value}% exceeds threshold ${limit}%"
    else
        echo "Status       : OK (not above the ${limit}% threshold)"
    fi
}

print_cpu_section() {
    local usage
    echo "===== CPU UTILIZATION ====="
    if [ ! -r /proc/stat ] || ! usage=$(get_cpu_usage); then
        echo "Error: CPU usage could not be read."
        log_action ERROR "CPU usage could not be read"
        return 1
    fi
    echo "CPU cores    : $(nproc)"
    echo "Load average : $(cut -d' ' -f1-3 /proc/loadavg 2>/dev/null)  (1, 5, 15 minutes)"
    echo "CPU usage    : ${usage}%"
    check_threshold "CPU usage" "$usage" "$CPU_THRESHOLD"
}

print_memory_section() {
    echo "===== MEMORY UTILIZATION ====="
    if ! get_memory_stats; then
        echo "Error: memory information could not be read."
        log_action ERROR "Memory information could not be read"
        return 1
    fi
    echo "Total memory     : ${MEM_TOTAL} MB"
    echo "Used memory      : ${MEM_USED} MB"
    echo "Available memory : ${MEM_AVAILABLE} MB"
    echo "Utilization      : ${MEM_PERCENT}%"
    check_threshold "Memory utilization" "$MEM_PERCENT" "$MEM_THRESHOLD"
}

print_disk_section() {
    local table full
    echo "===== DISK UTILIZATION ====="
    # shellcheck disable=SC2086
    table=$(df -hP $DF_EXCLUDE 2>/dev/null)
    if [ -z "$table" ]; then
        echo "Error: disk information could not be read."
        log_action ERROR "Disk information could not be read"
        return 1
    fi
    echo "$table"
    echo

    # Filesystems at or above the threshold
    # shellcheck disable=SC2086
    full=$(df -P $DF_EXCLUDE 2>/dev/null | awk -v t="$DISK_THRESHOLD" '
        NR > 1 {
            use = $5; sub(/%/, "", use)
            mount = $6
            for (i = 7; i <= NF; i++) mount = mount " " $i
            if (use + 0 >= t) printf "  %s (%s%% used)\n", mount, use
        }')
    log_action INFO "Disk check done (threshold ${DISK_THRESHOLD}%)"
    if [ -n "$full" ]; then
        echo "WARNING: filesystems at or above the ${DISK_THRESHOLD}% threshold:"
        echo "$full"
        log_action WARNING "Filesystems at or above ${DISK_THRESHOLD}%: $(echo "$full" | awk '{ $1 = $1; printf "%s%s", sep, $0; sep = "; " }')"
    else
        echo "Status: OK (no filesystem is at or above the ${DISK_THRESHOLD}% threshold)"
    fi
}

# ================= Menu options =================

# 1. System information
show_system_info() {
    require_commands hostname uname uptime date || return
    local os="Unknown"
    if [ -r /etc/os-release ]; then
        os=$(. /etc/os-release && echo "$PRETTY_NAME")
    fi
    [ -z "$os" ] && os=$(uname -s)

    echo "===== SYSTEM INFORMATION ====="
    echo "Hostname       : $(hostname)"
    echo "Operating sys. : $os"
    echo "Kernel version : $(uname -r)"
    echo "Uptime         : $(get_uptime)"
    echo "Date and time  : $(date '+%Y-%m-%d %H:%M:%S')"
    log_action INFO "System information displayed"
}

# 2. CPU
monitor_cpu() {
    require_commands awk sleep nproc cut || return
    ask_threshold "CPU" CPU_THRESHOLD
    echo "Measuring CPU usage (about 2 seconds)..."
    echo
    print_cpu_section || return

    local procs flagged
    procs=$(sample_cpu_processes)
    echo
    if [ -z "$procs" ]; then
        echo "Error: the process list could not be read."
        log_action ERROR "Process list could not be read"
        return
    fi
    echo "Top 5 processes by CPU usage:"
    printf '%s\n' "$procs" | head -n 5 | print_process_table

    flagged=$(printf '%s\n' "$procs" | awk -v t="$CPU_THRESHOLD" '$3 + 0 > t + 0')
    echo
    if [ -n "$flagged" ]; then
        echo "WARNING: processes using more than ${CPU_THRESHOLD}% CPU:"
        printf '%s\n' "$flagged" | print_process_table
        log_action WARNING "Processes above ${CPU_THRESHOLD}% CPU: $(printf '%s\n' "$flagged" | awk '{ printf "%s%s (PID %s)", sep, $5, $1; sep = ", " }')"
    else
        echo "No process is using more than ${CPU_THRESHOLD}% CPU."
    fi
}

# 3. Memory
monitor_memory() {
    require_commands free awk || return
    ask_threshold "memory" MEM_THRESHOLD
    echo
    print_memory_section
}

# 4. Disk
monitor_disk() {
    require_commands df awk || return
    ask_threshold "disk" DISK_THRESHOLD
    echo
    print_disk_section
}

# 5. View and search processes
search_process() {
    local term result
    ask "Enter a process name or PID: " term
    if [ -z "$term" ]; then
        echo "Error: the search cannot be empty."
        return
    fi

    if [[ "$term" =~ ^[0-9]+$ ]]; then
        result=$(ps -p "$term" -o pid=,user:20=,pcpu=,pmem=,comm= 2>/dev/null)
        if [ -z "$result" ]; then
            echo "No process found with PID $term."
            log_action INFO "Process search by PID $term: not found"
            return
        fi
    else
        result=$(list_processes | awk -v t="$term" 'index(tolower($5), tolower(t)) > 0')
        if [ -z "$result" ]; then
            echo "No process found with a name containing \"$term\"."
            log_action INFO "Process search \"$term\": not found"
            return
        fi
    fi
    printf '%s\n' "$result" | print_process_table
    log_action INFO "Process search \"$term\": $(printf '%s\n' "$result" | wc -l) match(es)"
}

process_menu() {
    require_commands ps awk || return
    local choice total
    while true; do
        echo
        echo "--- View and search processes ---"
        echo "1. Display running processes (top 15 by CPU)"
        echo "2. Search for a process by name or PID"
        echo "0. Back to the main menu"
        ask "Choose an option: " choice
        case "$choice" in
            1)
                total=$(ps -e --no-headers | wc -l)
                echo "Running processes: $total (showing the top 15 by CPU)"
                list_processes | head -n 15 | print_process_table
                log_action INFO "Running processes displayed ($total in total)"
                ;;
            2) search_process ;;
            0) return ;;
            *) echo "Error: invalid choice. Enter 0, 1 or 2." ;;
        esac
    done
}

# Return 0 and print the reason if a process must not be terminated.
protection_reason() {
    local pid="$1" name="$2" parent p
    if [ "$pid" -le 2 ]; then
        echo "it is a core system process (PID $pid)"
        return 0
    fi
    if [ "$pid" -eq "$$" ] || [ "$pid" -eq "$PPID" ]; then
        echo "it is this script or the shell that started it"
        return 0
    fi
    parent=$(ps -o ppid= -p "$pid" 2>/dev/null | tr -d ' ')
    if [ "$parent" = "2" ]; then
        echo "it is a kernel thread"
        return 0
    fi
    for p in $PROTECTED_NAMES; do
        if [ "$name" = "$p" ]; then
            echo "\"$name\" is a protected system service"
            return 0
        fi
    done
    return 1
}

# 6. Terminate a process
terminate_process() {
    require_commands ps kill sleep || return
    local pid info name reason
    ask "Enter the PID of the process to terminate: " pid
    if ! [[ "$pid" =~ ^[0-9]{1,9}$ ]] || [ "$pid" -lt 1 ]; then
        echo "Error: invalid PID. Enter a positive whole number."
        log_action ERROR "Terminate: invalid PID entered"
        return
    fi

    info=$(ps -p "$pid" -o pid=,user:20=,pcpu=,pmem=,comm= 2>/dev/null)
    if [ -z "$info" ]; then
        echo "Error: no process with PID $pid."
        log_action ERROR "Terminate: PID $pid does not exist"
        return
    fi
    name=$(ps -p "$pid" -o comm= | tr -d ' ')
    printf '%s\n' "$info" | print_process_table

    if reason=$(protection_reason "$pid" "$name"); then
        echo "Refused: this process cannot be terminated because $reason."
        log_action WARNING "Terminate refused for PID $pid ($name): protected"
        return
    fi
    if ! kill -0 "$pid" 2>/dev/null; then
        if ps -p "$pid" >/dev/null 2>&1; then
            echo "Error: you do not have permission to terminate PID $pid."
            log_action ERROR "Terminate: no permission for PID $pid ($name)"
        else
            echo "The process ended before it could be terminated."
        fi
        return
    fi

    if ! confirm "Terminate $name (PID $pid)?"; then
        echo "Cancelled. The process was not terminated."
        log_action INFO "Terminate cancelled for PID $pid ($name)"
        return
    fi
    if ! kill "$pid" 2>/dev/null; then
        echo "Error: the termination signal could not be sent."
        log_action ERROR "Terminate failed for PID $pid ($name)"
        return
    fi
    sleep 1
    if ps -p "$pid" >/dev/null 2>&1; then
        echo "The signal was sent, but $name (PID $pid) is still running."
        log_action WARNING "Terminate signal sent to PID $pid ($name) but it is still running"
    else
        echo "Process $name (PID $pid) was terminated."
        log_action INFO "Terminated process $name (PID $pid)"
    fi
}

# Write the full report to standard output.
build_report() {
    echo "===== SYSTEM HEALTH REPORT ====="
    echo "Generated  : $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Hostname   : $(hostname)"
    echo "Uptime     : $(get_uptime)"
    echo
    print_cpu_section
    echo
    print_memory_section
    echo
    print_disk_section
}

# 7. Health report
generate_report() {
    require_commands hostname uptime date df free awk nproc sleep cut || return
    local dir file
    ask "Directory to save the report in [Enter for the current directory]: " dir
    [ -z "$dir" ] && dir="."
    dir="${dir/#\~/$HOME}"

    if [ -e "$dir" ] && [ ! -d "$dir" ]; then
        echo "Error: '$dir' is not a directory."
        log_action ERROR "Report: '$dir' is not a directory"
        return
    fi
    if [ ! -d "$dir" ]; then
        if ! confirm "Directory '$dir' does not exist. Create it"; then
            echo "Report cancelled."
            log_action INFO "Report cancelled (directory '$dir' missing)"
            return
        fi
        if ! mkdir -p "$dir" 2>/dev/null; then
            echo "Error: cannot create the directory '$dir' (permission denied?)."
            log_action ERROR "Report: cannot create directory '$dir'"
            return
        fi
    fi
    if [ ! -w "$dir" ]; then
        echo "Error: you do not have permission to write in '$dir'."
        log_action ERROR "Report: no write permission in '$dir'"
        return
    fi

    file="$dir/health_report_$(date '+%Y%m%d_%H%M%S').txt"
    echo "Collecting data (about 2 seconds)..."
    if ! { build_report; } 2>/dev/null > "$file"; then
        echo "Error: the report could not be written to '$file'."
        log_action ERROR "Report: writing '$file' failed"
        return
    fi
    echo "Report saved to: $file"
    log_action INFO "Health report generated: $file"
}

# 8. View the log
view_log() {
    if [ ! -s "$LOG_FILE" ]; then
        echo "The activity log is empty."
        return
    fi
    local total
    total=$(wc -l < "$LOG_FILE")
    echo "===== ACTIVITY LOG ($LOG_FILE) ====="
    if [ "$total" -gt 50 ]; then
        echo "Showing the last 50 of $total entries."
    else
        echo "Entries: $total"
    fi
    tail -n 50 "$LOG_FILE"
    log_action INFO "Activity log viewed"
}

# 9. Clear the log
clear_log() {
    if [ ! -s "$LOG_FILE" ]; then
        echo "The activity log is already empty."
        return
    fi
    if ! confirm "This permanently deletes all activity log entries. Continue"; then
        echo "Cancelled. The log was not cleared."
        return
    fi
    if : 2>/dev/null > "$LOG_FILE"; then
        echo "Activity log cleared."
        log_action INFO "Activity log cleared by the user"
    else
        echo "Error: the activity log could not be cleared."
    fi
}

# ================= Main program =================

print_menu() {
    echo
    echo "===== LINUX SYSTEM HEALTH AND RESOURCE MONITORING MANAGER ====="
    echo " 1. Display system information"
    echo " 2. Monitor CPU utilization"
    echo " 3. Monitor memory utilization"
    echo " 4. Monitor disk utilization"
    echo " 5. View and search running processes"
    echo " 6. Terminate a process"
    echo " 7. Generate a system health report"
    echo " 8. View activity history"
    echo " 9. Clear activity log"
    echo "10. Exit"
}

main() {
    local choice

    # The log must be writable, otherwise nothing could be recorded.
    if ! : 2>/dev/null >> "$LOG_FILE"; then
        echo "Error: cannot write to the activity log: $LOG_FILE"
        echo "Set HEALTH_MONITOR_LOG to a writable file and run the script again."
        exit 1
    fi
    trap 'echo; log_action INFO "Application interrupted"; exit 130' INT
    log_action INFO "Application started"

    while true; do
        print_menu
        ask "Enter your choice (1-10): " choice
        if ! [[ "$choice" =~ ^[0-9]{1,2}$ ]]; then
            echo "Error: invalid choice. Enter a number from 1 to 10."
            continue
        fi
        case "$((10#$choice))" in
            1)  show_system_info ;;
            2)  monitor_cpu ;;
            3)  monitor_memory ;;
            4)  monitor_disk ;;
            5)  process_menu ;;
            6)  terminate_process ;;
            7)  generate_report ;;
            8)  view_log ;;
            9)  clear_log ;;
            10) log_action INFO "Application exited"
                echo "Goodbye."
                exit 0 ;;
            *)  echo "Error: invalid choice. Enter a number from 1 to 10." ;;
        esac
    done
}

main
