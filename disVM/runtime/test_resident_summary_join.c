/* Copyright (c) 2026 bhgv
 * EN: Observe actual detached reapers before their native join, without heap access.
 * RU: Наблюдать actual detached reapers перед native join, без heap access.
 */
#include "managed_workers.h"
#include "thread_registrar.h"
#include <stdio.h>

static int seen;      /* Atomic once-only evidence for an actual reaping partition. */
static int malformed; /* Atomic once-only semantic omission evidence. */
extern int __real_dvm_thread_registrar_join(DVM_ThreadRegistrar *, DVM_ThreadSlot *);
extern int __real_dvm_resident_summary(DVM_ResidentSummary *);


/* EN: Delegate native copy; CONTROL=1 suppresses only copied in-flight count.
 * Borrow output through return; no runtime ownership/execution behavior changes.
 * RU: Делегировать native copy; CONTROL=1 скрывает только copied in-flight count.
 * Заимствовать output до возврата; runtime ownership/execution не меняются. */
int __wrap_dvm_resident_summary(DVM_ResidentSummary *summary)
{
    int result = __real_dvm_resident_summary(summary);
    if (result && CONTROL == 1)
    {
        summary->reaping = 0;
    }
    return result;
}


/* EN: Borrow registrar/slot for actual join. Copy summary in the detached window;
 * record anomalies without aborting, so application and GC retire normally.
 * RU: Заимствовать registrar/slot для actual join. Читать summary в detached window;
 * фиксировать ошибки без abort, чтобы application и GC завершились штатно. */
int __wrap_dvm_thread_registrar_join(DVM_ThreadRegistrar *registrar, DVM_ThreadSlot *slot)
{
    DVM_ResidentSummary summary;
    if (dvm_resident_summary(NULL) || !dvm_resident_summary(&summary) ||
        summary.created != summary.registered + summary.reaping + summary.reaped || summary.done > summary.registered)
    {
        if (!__atomic_exchange_n(&malformed, 1, __ATOMIC_RELAXED))
        {
            fprintf(stderr, "DETECTED resident summary lost in-flight partition\n");
        }
    }
    else if (summary.reaping && !__atomic_exchange_n(&seen, 1, __ATOMIC_RELAXED))
    {
        fprintf(stderr, "NATIVERESIDENT_JOIN observed=1 partition=1\n");
    }
    return __real_dvm_thread_registrar_join(registrar, slot);
}
