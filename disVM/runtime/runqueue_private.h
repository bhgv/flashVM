/* Copyright (c) 2026 bhgv
 * EN: Private native-lease accounting and shared runqueue validation helpers.
 * RU: Закрытый учёт native-ожиданий и общие проверки очереди исполнения.
 */
#ifndef DVM_RUNQUEUE_PRIVATE_H
#define DVM_RUNQUEUE_PRIVATE_H
#include "runqueue.h"


/* EN: Native counters use atomics after startup; trace_waits is immutable.
 * RU: После запуска счётчики атомарны; trace_waits не меняется. */
typedef struct DVM_RunQueueLeases
{
    unsigned long leases;   /* Native withdrawals before audited world leave. */
    unsigned long returns;  /* Matched same-owner restorations after world enter. */
    unsigned long inflight; /* Native continuations absent from runnable dispatch. */
    unsigned long peak;     /* Maximum simultaneous audited native continuation leases. */
    int trace_waits;        /* Immutable opt-in diagnostics, enabled before thread launch. */
} DVM_RunQueueLeases;

extern DVM_RunQueueLeases dvm_runqueue_leases; /* Native process-lifetime counters. */


/* EN: Cast a separately retained Prog allocation to its native extension.
 * RU: Получить нативное расширение отдельно удерживаемой аллокации process. */
static inline DVM_RuntimeProcess *runtime_process(Prog *process)
{
    return (DVM_RuntimeProcess *)process;
}


/* EN: Read native ready count; no process identity or ownership is acquired.
 * RU: Прочитать число готовых процессов без приобретения владения. */
unsigned int dvm_runqueue_ready_count(void);


/* EN: Check legacy endpoints under caller exclusion; direct mode has no FIFO.
 * RU: Проверить старую FIFO под защитой вызывающего; прямой режим обходится без неё. */
int dvm_runqueue_endpoints_valid(void);


/* EN: Publish resumed process readiness after exact owner validation by caller.
 * RU: Опубликовать готовность process после точной проверки владельца вызывающим. */
void dvm_runqueue_restore_ready(Prog *process);
#endif
