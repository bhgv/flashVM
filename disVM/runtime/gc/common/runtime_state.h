/* Copyright (c) 2026 bhgv
 *
 * EN: Process-owned GC coordinator state shared with native admission.
 * RU: Состояние координатора GC процесса, общее с нативным допуском.
 */
#ifndef DVM_GC_RUNTIME_STATE_H
#define DVM_GC_RUNTIME_STATE_H

#include "disvm/gc_workers.h"

struct Prog;

typedef struct RuntimeGC
{
    DVM_NativeContext *owner;         /* Immutable Dis context through collector joins. */
    DVM_NativeWorld *world;           /* Owned root coordination domain. */
    DVM_GCWorkers *workers;           /* Native Fast, Cycles and Duke contexts. */
    struct Prog **roots;                    /* Borrowed complete process-list slot. */
    unsigned long fast_empty_skips;   /* Mutator skipped an empty Fast request. */
    unsigned long passes;            /* Real retained rungc callbacks. */
    unsigned long idle_cycle_epoch;  /* First epoch of the current idle burst. */
    unsigned int idle_cycle_passes;   /* Bound work per idle request. */
    unsigned long cycle_losses;     /* Managed owner losses observed under world admission. */
    unsigned long cycle_skips;      /* Clean requests that avoided a heap pass. */
    unsigned long cycle_epoch;      /* Epoch at the last loss or fallback scan. */
    unsigned int clean_requests;    /* Bounded fallback request counter. */
    unsigned long snapshots;         /* Stopped active register snapshots. */
} RuntimeGC;

extern RuntimeGC dvm_gc_runtime; /* One managed heap, valid through worker joins. */

#endif
