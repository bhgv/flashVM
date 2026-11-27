/* Copyright (c) 2026 bhgv
 * EN: Sample and format owner-thread playback and process statistics for .state.
 * RU: Получение и форматирование статистики плеера и процесса для .state.
 */
#define _POSIX_C_SOURCE 200809L /* Expose optional POSIX owner CPU and monotonic clocks. */
#include "playback_internal.h"
#include "flashvm/gc.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* EN: Format CPU use for this owner thread since the previous sample when clocks exist.
 * RU: Вычислить загрузку потока владельца с прошлого снимка при наличии часов. */
static int owner_cpu(FVM_StyxRpcPlayback *playback, double *percent)
{
#if defined(CLOCK_MONOTONIC) && defined(CLOCK_THREAD_CPUTIME_ID)
    struct timespec wall_time, cpu_time;
    double wall, cpu;
    int measured;
    if (clock_gettime(CLOCK_MONOTONIC, &wall_time) || clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpu_time))
    {
        return 0;
    }
    wall = (double)wall_time.tv_sec + (double)wall_time.tv_nsec / 1000000000.0;
    cpu = (double)cpu_time.tv_sec + (double)cpu_time.tv_nsec / 1000000000.0;
    measured = playback->last_wall > 0 && wall > playback->last_wall && cpu >= playback->last_cpu;
    if (measured)
    {
        *percent = (cpu - playback->last_cpu) / (wall - playback->last_wall) * 100.0;
    }
    playback->last_wall = wall;
    playback->last_cpu = cpu;
    return measured;
#else
    (void)playback;
    (void)percent;
    return 0;
#endif
}


/* EN: Match the prior kernel row by thread ID and creation time and report a rate.
 * RU: Найти предыдущую запись ядра по ID и времени создания и вычислить загрузку. */
static void process_cpu(const FVM_StyxRpcProcessStats *prior, const FVM_StyxRpcThreadSample *row,
    double elapsed, char *bytes, size_t capacity)
{
    unsigned i;
    strcpy(bytes, "unavailable");
    if (elapsed <= 0)
    {
        return;
    }
    for (i = 0; i < prior->used; ++i)
    {
        const FVM_StyxRpcThreadSample *old = prior->rows + i;
        if (old->id == row->id && old->created == row->created && old->cpu <= row->cpu)
        {
            snprintf(bytes, capacity, "%.2f", (row->cpu - old->cpu) / elapsed * 100.0);
            return;
        }
    }
}


/* EN: Append measured process metrics while preserving space for application telemetry.
 * RU: Дописать измерения процесса, оставив место для статистики приложения. */
static size_t append_process(FVM_StyxRpcPlayback *playback, char *bytes, size_t capacity)
{
    FVM_StyxRpcProcessStats current;
    unsigned i;
    size_t used;
    int count;
    if (!fvm_styxrpc_process_stats(&current))
    {
        count = snprintf(bytes, capacity, "process.threads=unavailable\nprocess.rss_bytes=unavailable\n");
        return count >= 0 && (size_t)count < capacity ? (size_t)count : 0;
    }
    count = snprintf(bytes, capacity, "process.threads=%u\nprocess.rss_bytes=%lu\nprocess.cpu_rows=%u\n",
        current.count, current.rss, current.used);
    if (count < 0 || (size_t)count >= capacity)
    {
        return 0;
    }
    used = (size_t)count;
    for (i = 0; i < current.used; ++i)
    {
        char cpu[32];
        process_cpu(&playback->process, current.rows + i, current.time - playback->process.time, cpu, sizeof(cpu));
        count = snprintf(bytes + used, capacity - used, "process.thread.%lu.cpu_percent=%s\n", current.rows[i].id, cpu);
        if (count < 0 || (size_t)count >= capacity - used)
        {
            break;
        }
        used += (size_t)count;
    }
    playback->process = current;
    return used;
}


/* EN/RU: See playback_internal.h. / См. playback_internal.h. */
int fvm_styxrpc_playback_snapshot(void *data, char *bytes, size_t capacity, size_t *used)
{
    static const char *const states[] = {"empty", "ready", "running", "paused", "debug", "failed"};
    FVM_StyxRpcPlayback *playback = data;
    FVM_GCStats gc;
    FVM_GCStringStats strings;
    FVM_ExecutionStats execution;
    double cpu;
    char load[32];
    int length;
    size_t used_bytes, extra = 0;
    memset(&gc, 0, sizeof(gc));
    memset(&strings, 0, sizeof(strings));
    memset(&execution, 0, sizeof(execution));
    gc.struct_size = sizeof(gc);
    execution.struct_size = sizeof(execution);
    if (playback->context &&
        (fvm_gc_stats(playback->context, &gc) != FVM_OK ||
         fvm_gc_string_stats(playback->context, &strings) != FVM_OK ||
         fvm_execution_get_stats(playback->context, &execution) != FVM_OK))
    {
        return 0;
    }
    strcpy(load, "unavailable");
    if (owner_cpu(playback, &cpu))
    {
        snprintf(load, sizeof(load), "%.2f", cpu);
    }
    length = snprintf(bytes, capacity,
        "state=%s\nlast_result=%d\nticks=%lu\nthreads.owner=1\nthread.owner.cpu_percent=%s\n"
        "swf_bytes=%lu\nstring_bytes=%lu\nlive_objects=%lu\npinned_objects=%lu\ngc_pending=%lu\n"
        "interpreter_entries=%lu\nnative_entries=%lu\ncompiled_bodies=%lu\n"
        "movies_loaded=%lu\nmovie_load_errors=%lu\nlast_movie_result=%d\n",
        states[playback->debug.stopped ? PLAYBACK_DEBUG : playback->state], (int)playback->last, playback->ticks, load,
        (unsigned long)playback->size, (unsigned long)strings.allocated_bytes,
        gc.live_objects, gc.pinned_objects, gc.pending_allocations,
        execution.interpreter_entries, execution.native_entries, execution.compiled_bodies,
        playback->movies ? playback->movies->loaded : 0UL,
        playback->movies ? playback->movies->failed : 0UL,
        playback->movies ? (int)playback->movies->last : FVM_OK);
    if (length < 0 || (size_t)length >= capacity)
    {
        return 0;
    }
    used_bytes = (size_t)length;
    used_bytes += fvm_styxrpc_debug_sample(&playback->debug, bytes + used_bytes, capacity - used_bytes);
    used_bytes += append_process(playback, bytes + used_bytes, capacity - used_bytes);
    if (playback->spec.statistics)
    {
        size_t remaining = capacity - used_bytes;
        if (!playback->spec.statistics(playback->spec.host, bytes + used_bytes, remaining, &extra) || extra > remaining)
        {
            return 0;
        }
    }
    *used = used_bytes + extra;
    return 1;
}
