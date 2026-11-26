/*
 * document_analyzer.c
 * Multi-threaded Document Search and Analysis System (POSIX threads).
 *
 * Usage: ./document_analyzer "keyword" file1.txt file2.txt ...
 *
 * One thread is created per input file. Each thread works only on its own
 * ThreadArgs record and writes its own report file, so no shared data is
 * modified and no synchronization (mutex) is required.
 *
 * Build: gcc -Wall -Wextra -pthread -o document_analyzer document_analyzer.c
 */

#define _POSIX_C_SOURCE 200809L   /* for getline() */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>`
#include <ctype.h>
#include <pthread.h>

#define MAX_NAME   256
#define MAX_STATUS 128

/* Everything one thread needs (input) and produces (output). */
typedef struct {
    int         id;                    /* thread number shown to the user (1-based) */
    const char *filename;              /* input file (points into argv)             */
    const char *keyword;               /* search keyword (points into argv)         */
    char        report_name[MAX_NAME]; /* output report file name                   */
    char        status[MAX_STATUS];    /* processing status text                    */
    int         success;               /* 1 = analysis succeeded, 0 = failed        */
    int         created;               /* 1 = thread was actually created           */
    long        total_lines;
    long        total_occurrences;
    long        matching_lines;
    pthread_t   tid;
} ThreadArgs;

/* ---------- helpers ---------- */

/* Count case-insensitive, non-overlapping occurrences of kw in line.
 * kw must not be empty (validated in main). */
static int count_occurrences(const char *line, const char *kw)
{
    size_t klen = strlen(kw);
    int count = 0;
    const char *p = line;

    while (*p) {
        size_t i = 0;
        while (i < klen && p[i] &&
               tolower((unsigned char)p[i]) == tolower((unsigned char)kw[i]))
            i++;
        if (i == klen) {
            count++;
            p += klen;
        } else {
            p++;
        }
    }
    return count;
}

/* Build "<name-without-extension>_report.txt" from an input path. */
static void build_report_name(const char *path, char *out, size_t size)
{
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;

    const char *dot = strrchr(base, '.');
    int len = (dot && dot != base) ? (int)(dot - base) : (int)strlen(base);

    snprintf(out, size, "%.*s_report.txt", len, base);
}

/* Initialize one thread record. */
static void init_thread_args(ThreadArgs *a, int id, const char *file, const char *kw)
{
    a->id = id;
    a->filename = file;
    a->keyword = kw;
    a->success = 0;
    a->created = 0;
    a->total_lines = 0;
    a->total_occurrences = 0;
    a->matching_lines = 0;
    snprintf(a->status, MAX_STATUS, "Not started");
    build_report_name(file, a->report_name, MAX_NAME);
}

/* Write this thread's report. Returns 0 on success, -1 on failure. */
static int write_report(const ThreadArgs *a, const long *lines, long nlines)
{
    FILE *fp = fopen(a->report_name, "w");
    if (!fp)
        return -1;

    fprintf(fp, "===== DOCUMENT ANALYSIS REPORT =====\n");
    fprintf(fp, "Input file          : %s\n", a->filename);
    fprintf(fp, "Search keyword      : %s\n", a->keyword);
    fprintf(fp, "Total lines         : %ld\n", a->total_lines);
    fprintf(fp, "Total occurrences   : %ld\n", a->total_occurrences);
    fprintf(fp, "Matching lines      : %ld\n", a->matching_lines);
    fprintf(fp, "Matching line numbers: ");
    if (nlines == 0) {
        fprintf(fp, "none\n");
    } else {
        for (long i = 0; i < nlines; i++)
            fprintf(fp, "%ld%s", lines[i], (i + 1 < nlines) ? ", " : "\n");
    }
    fprintf(fp, "Status              : %s\n", a->status);

    if (fclose(fp) != 0)
        return -1;
    return 0;
}

/* ---------- thread function ---------- */

/* Search one file, count lines/occurrences, and write the report. */
static void *process_file(void *arg)
{
    ThreadArgs *a = (ThreadArgs *)arg;
    long *lines = NULL;       /* matching line numbers (dynamic array) */
    long nlines = 0, cap = 0;
    int failed = 0;

    printf("[Thread %d] Processing %s ...\n", a->id, a->filename);

    FILE *fp = fopen(a->filename, "r");
    if (!fp) {
        snprintf(a->status, MAX_STATUS, "Failed: cannot open input file");
        failed = 1;
    } else {
        char *buf = NULL;
        size_t bufsize = 0;

        while (getline(&buf, &bufsize, fp) != -1) {
            a->total_lines++;
            int n = count_occurrences(buf, a->keyword);
            if (n > 0) {
                a->total_occurrences += n;
                a->matching_lines++;

                if (nlines == cap) {          /* grow array when full */
                    long newcap = cap ? cap * 2 : 16;
                    long *tmp = realloc(lines, newcap * sizeof(long));
                    if (!tmp) {
                        snprintf(a->status, MAX_STATUS, "Failed: out of memory");
                        failed = 1;
                        break;
                    }
                    lines = tmp;
                    cap = newcap;
                }
                lines[nlines++] = a->total_lines;
            }
        }
        free(buf);
        fclose(fp);

        if (!failed) {
            if (a->total_occurrences == 0)
                snprintf(a->status, MAX_STATUS, "Success (keyword not found)");
            else
                snprintf(a->status, MAX_STATUS, "Success");
        }
    }

    /* A report is written in every case so the status is recorded. */
    if (write_report(a, lines, nlines) != 0) {
        snprintf(a->status, MAX_STATUS, "Failed: cannot write report file");
        failed = 1;
    }
    free(lines);

    a->success = !failed;
    if (a->success)
        printf("[Thread %d] Successfully processed %s\n", a->id, a->filename);
    else
        printf("[Thread %d] %s (%s)\n", a->id, a->status, a->filename);

    return NULL;
}

/* ---------- output ---------- */

static void display_summary(const ThreadArgs *args, int n)
{
    int ok = 0;

    printf("\n===== PROCESSING SUMMARY =====\n\n");
    for (int i = 0; i < n; i++) {
        if (args[i].success) {
            printf("%s: Completed (%ld occurrences in %ld lines, report: %s)\n",
                   args[i].filename, args[i].total_occurrences,
                   args[i].matching_lines, args[i].report_name);
            ok++;
        } else {
            printf("%s: %s\n", args[i].filename, args[i].status);
        }
    }

    if (ok == n)
        printf("\nAll processing tasks completed.\nReports generated successfully.\n");
    else
        printf("\n%d of %d files processed successfully.\n", ok, n);
}

/* ---------- main ---------- */

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s \"keyword\" file1 [file2 ...]\n", argv[0]);
        return 1;
    }
    if (argv[1][0] == '\0') {
        fprintf(stderr, "Error: search keyword must not be empty.\n");
        return 1;
    }

    const char *keyword = argv[1];
    int nfiles = argc - 2;

    ThreadArgs *args = calloc(nfiles, sizeof(ThreadArgs));
    if (!args) {
        fprintf(stderr, "Error: memory allocation failed.\n");
        return 1;
    }

    printf("===== MULTI-THREADED DOCUMENT ANALYZER =====\n\n");
    printf("Search Keyword: %s\n", keyword);
    printf("Number of Input Files: %d\n\n", nfiles);

    /* Create one thread per file. */
    for (int i = 0; i < nfiles; i++) {
        init_thread_args(&args[i], i + 1, argv[i + 2], keyword);

        if (pthread_create(&args[i].tid, NULL, process_file, &args[i]) != 0) {
            snprintf(args[i].status, MAX_STATUS, "Failed: thread creation error");
            printf("[Main] ERROR: could not create thread %d for %s\n",
                   args[i].id, args[i].filename);
        } else {
            args[i].created = 1;
            printf("[Main] Created thread %d for %s\n", args[i].id, args[i].filename);
        }
    }

    /* Wait for every thread that was successfully created. */
    for (int i = 0; i < nfiles; i++) {
        if (args[i].created)
            pthread_join(args[i].tid, NULL);
    }

    display_summary(args, nfiles);

    int exit_code = 0;
    for (int i = 0; i < nfiles; i++)
        if (!args[i].success)
            exit_code = 1;

    free(args);
    return exit_code;
}
