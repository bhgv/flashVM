/* Copyright (c) 2026 bhgv
 * EN: Native process and OS thread accounting for playback snapshots.
 * RU: Нативный учёт процесса и потоков ОС для снимков плеера.
 */
#ifndef FLASHVM_STYXRPC_STATS_PORT_H
#define FLASHVM_STYXRPC_STATS_PORT_H
#include <stddef.h>

#define FVM_STYXRPC_THREAD_ROWS 48U /* Maximum CPU rows that fit alongside built-in VM stats. */

typedef struct FVM_StyxRpcThreadSample
{
    unsigned long id;      /* System process-local thread ID. */
    unsigned long created; /* Kernel start tick, distinguishing recycled IDs. */
    double cpu;             /* Cumulative user + system seconds. */
} FVM_StyxRpcThreadSample;

typedef struct FVM_StyxRpcProcessStats
{
    unsigned count;                          /* Total OS threads seen in this sample. */
    unsigned used;                            /* Number of recorded rows. */
    double time;                              /* Monotonic wall seconds. */
    FVM_StyxRpcThreadSample rows[FVM_STYXRPC_THREAD_ROWS]; /* Current thread counters. */
    unsigned long rss;                       /* Resident process bytes, or zero if unavailable. */
} FVM_StyxRpcProcessStats;


/* EN: Measure current process stats; return zero on unsupported platforms or measurement failure.
 * RU: Измерить статистику текущего процесса. Ноль при отсутствии поддержки или ошибке. */
int fvm_styxrpc_process_stats(FVM_StyxRpcProcessStats *stats);

#endif
