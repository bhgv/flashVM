/* Copyright (c) 2026 bhgv
 * EN: Same-thread native continuation leases and their atomic accounting.
 * RU: Native-ожидания на исходном потоке и их атомарный учёт.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runqueue_private.h"
#include "managed_workers.h"
#include <stdio.h>
#include <stdlib.h>

DVM_RunQueueLeases dvm_runqueue_leases; /* Native counters alive through joins. */


/* Enable prelaunch peak diagnostics without retaining a managed object.
 * RU: До запуска включить диагностику пиков native-ожиданий; управление процессами не меняется. */
void dvm_runqueue_trace_waits(void)
{
    dvm_runqueue_leases.trace_waits = 1;
}


/* Respect the existing synchronous Native64Caller lease. process is borrowed
 * under the heap turn; a nonzero native64holds forbids native suspension even
 * with separate TLS state. NULL denotes rootless native work, not a Prog lease.
 * RU: Проверить заимствованный process на синхронную native64holds-аренду; NULL разрешает работу без Prog. */
int dvm_runqueue_release_allowed(Prog *process)
{
    return !process || !process->native64holds;
}


/* [GC:] Withdraw current runtime Prog before world leave. It stays in the host's
 * GC process list; only its runnable link/state and native lease metadata change.
 * Foreign/current-head mismatch returns zero before any mutation/allocation.
 * RU: Снять текущий process перед native-ожиданием на исходном контексте. При чужом владельце отказать до изменения очереди. */
int dvm_runqueue_suspend(Prog *process)
{
    DVM_NativeContext *context = dvm_native_context_current();
    if (!process || !context || !dvm_runqueue_release_allowed(process) ||
        dvm_native_context_process(context) != process || dvm_runqueue_dispatch_owner(process) != context ||
        dvm_runqueue_owner(process) || process->state != Pready)
    {
        return 0;
    }
    if (dvm_runqueue_pop_expected(process, Prelease) != process)
    {
        return 0;
    }
    __atomic_store_n(&runtime_process(process)->native_owner, context, __ATOMIC_RELEASE);
    __atomic_fetch_add(&dvm_runqueue_leases.leases, 1, __ATOMIC_RELAXED);
    unsigned long count = __atomic_add_fetch(&dvm_runqueue_leases.inflight, 1, __ATOMIC_RELAXED);
    unsigned long previous = __atomic_load_n(&dvm_runqueue_leases.peak, __ATOMIC_RELAXED);
    while (count > previous)
    {
        if (__atomic_compare_exchange_n(
                &dvm_runqueue_leases.peak, &previous, count, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            if (dvm_runqueue_leases.trace_waits)
            {
                fprintf(stderr, "NATIVEWAITERS inflight=%lu\n", count);
            }
            break;
        }
    }
    return 1;
}


/* Restore a native continuation only on its original TLS context. Current Prog
 * identity and FIFO absence are checked before prepending; caller has reacquired
 * the heap turn. No register/root ownership moves to another OS thread.
 * RU: На исходном контексте восстановить готовность process без переноса REG или корней на другой OS-поток; неверная идентичность даёт ноль. */
int dvm_runqueue_resume(Prog *process)
{
    DVM_NativeContext *context = dvm_native_context_current();
    if (!process || !context || dvm_runqueue_owner(process) != context ||
        dvm_runqueue_dispatch_owner(process) != context || dvm_native_context_process(context) != process ||
        process->state != Prelease || process->link ||
        !__atomic_load_n(&dvm_runqueue_leases.inflight, __ATOMIC_RELAXED) ||
        dvm_runqueue_ready_count() == DVM_RUNTIME_PROCESS_MAX || dvm_runqueue_is_ready(process) ||
        !dvm_runqueue_endpoints_valid())
    {
        return 0;
    }
    dvm_runqueue_restore_ready(process);
    __atomic_fetch_add(&dvm_runqueue_leases.returns, 1, __ATOMIC_RELAXED);
    __atomic_fetch_sub(&dvm_runqueue_leases.inflight, 1, __ATOMIC_RELEASE);
    dvm_managed_workers_wake();
    return 1;
}


/* Inspect native continuation identity under exclusive heap access. process is
 * a borrowed live runtime allocation; NULL has no owner. No foreign TLS read.
 * RU: С acquire получить идентичность native-владельца отдельно живого process; NULL даёт NULL и не передаёт владение. */
DVM_NativeContext *dvm_runqueue_owner(Prog *process)
{
    return process ? __atomic_load_n(&runtime_process(process)->native_owner, __ATOMIC_ACQUIRE) : NULL;
}


/* Copy actual in-flight continuation count; caller has exclusive heap ownership
 * or has joined all actors. No reservation or lifecycle transition occurs.
 * RU: Прочитать атомарное число ожидающих native-продолжений без изменения допуска или времени жизни. */
unsigned long dvm_runqueue_inflight(void)
{
    return __atomic_load_n(&dvm_runqueue_leases.inflight, __ATOMIC_ACQUIRE);
}
