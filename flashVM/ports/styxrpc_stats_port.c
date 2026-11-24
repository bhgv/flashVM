/* Copyright (c) 2026 bhgv
 * EN: Read Linux kernel process and thread counters outside the script namespace.
 * RU: Чтение счётчиков процесса и потоков Linux вне пространства путей сценария.
 */
#define _POSIX_C_SOURCE 200809L /* POSIX monotonic clock and process accounting settings. */
#include "styxrpc_stats_port.h"
#if defined(__linux__)
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PROC_TASK_PATH "/proc/self/task" /* Task directory of the current process only. */
#define PROC_MEMORY_PATH "/proc/self/statm" /* Total and resident system pages. */
#define PROC_THREAD_LINE 1024 /* Maximum parsed kernel thread stat record. */
#define PROC_USER_FIELD 14 /* User CPU field in proc stat. */
#define PROC_SYSTEM_FIELD 15 /* System CPU field in proc stat. */
#define PROC_CREATED_FIELD 22 /* Thread's monotonic start tick in proc stat. */
#define PROC_AFTER_NAME_FIELD 3 /* Process state immediately following the parenthesized name. */


/* EN: Parse the task stat counters beyond the final parenthesis of the kernel thread name.
 * RU: Разобрать счётчики задачи после последней скобки имени потока ядра. */
static int sample_thread(unsigned long id, long ticks, FVM_StyxRpcThreadSample *sample)
{
    char path[sizeof(PROC_TASK_PATH) + 32], line[PROC_THREAD_LINE];
    FILE *file;
    char *field, *end;
    int index;
    double cpu = 0;
    snprintf(path, sizeof(path), "%s/%lu/stat", PROC_TASK_PATH, id);
    file = fopen(path, "r");
    if (!file)
    {
        return 0;
    }
    field = fgets(line, sizeof(line), file);
    fclose(file);
    field = field ? strrchr(line, ')') : NULL;
    if (!field)
    {
        return 0;
    }
    ++field;
    for (index = PROC_AFTER_NAME_FIELD; index <= PROC_CREATED_FIELD; ++index)
    {
        while (*field == ' ')
        {
            ++field;
        }
        if (!*field)
        {
            return 0;
        }
        if (index == PROC_USER_FIELD || index == PROC_SYSTEM_FIELD)
        {
            cpu += strtod(field, &end);
            if (end == field || *end != ' ')
            {
                return 0;
            }
        }
        else if (index == PROC_CREATED_FIELD)
        {
            sample->created = strtoul(field, &end, 10);
            if (end == field || *end != ' ')
            {
                return 0;
            }
        }
        while (*field && *field != ' ')
        {
            ++field;
        }
    }
    sample->id = id;
    sample->cpu = cpu / (double)ticks;
    return 1;
}


/* EN: Convert the kernel's resident page count to bytes when representable.
 * RU: Преобразовать число резидентных страниц ядра в байты при наличии диапазона. */
static unsigned long resident(void)
{
    unsigned long total, pages, bytes = 0;
    long size = sysconf(_SC_PAGESIZE);
    FILE *file = fopen(PROC_MEMORY_PATH, "r");
    if (file)
    {
        if (size > 0 && fscanf(file, "%lu%lu", &total, &pages) == 2 && pages <= ULONG_MAX / (unsigned long)size)
        {
            bytes = pages * (unsigned long)size;
        }
        fclose(file);
    }
    return bytes;
}
#endif


/* EN/RU: See styxrpc_stats_port.h. / См. styxrpc_stats_port.h. */
int fvm_styxrpc_process_stats(FVM_StyxRpcProcessStats *stats)
{
#if defined(__linux__)
    struct timespec time;
    struct dirent *entry;
    DIR *tasks = opendir(PROC_TASK_PATH);
    long ticks = sysconf(_SC_CLK_TCK);
    if (!tasks)
    {
        return 0;
    }
    if (ticks <= 0 || clock_gettime(CLOCK_MONOTONIC, &time))
    {
        closedir(tasks);
        return 0;
    }
    memset(stats, 0, sizeof(*stats));
    stats->time = (double)time.tv_sec + (double)time.tv_nsec / 1000000000.0;
    while ((entry = readdir(tasks)) != NULL)
    {
        if (entry->d_name[0] >= '1' && entry->d_name[0] <= '9')
        {
            char *end;
            unsigned long id = strtoul(entry->d_name, &end, 10);
            if (!*end)
            {
                ++stats->count;
                if (stats->used < FVM_STYXRPC_THREAD_ROWS &&
                    sample_thread(id, ticks, stats->rows + stats->used))
                {
                    ++stats->used;
                }
            }
        }
    }
    closedir(tasks);
    stats->rss = resident();
    return 1;
#else
    (void)stats;
    return 0;
#endif
}
