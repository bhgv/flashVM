/* Copyright (c) 2026 bhgv
 *
 * EN: Publish guest roots and move native Dis contexts through heap admission.
 * RU: Публикация корней и переходы нативных контекстов Dis между режимами допуска.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "disvm/gc_workers.h"
#include "disvm/native_owner.h"
#include "disvm/native_process.h"
#include "../../runtime_gc.h"
#include "../collector_passes.h"
#include "runtime_state.h"
#include "exception_host.h"
#include "runqueue.h"
#include "managed_workers.h"
#include "queue_probe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




/* EN: Publish native roots before relinquishing admission. shared requests shared work.
 * RU: Опубликовать нативные корни перед выходом из допуска. shared запрашивает параллельный режим. */
static int native_try_leave(int shared)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats call;
    Prog *process;
    const char *refusal;
    if (!dvm_gc_runtime.world)
    {
        return 0;
    }
    if (dvm_native_owner_world(owner) != dvm_gc_runtime.world ||
        !dvm_native_owner_statistics(owner,&call) || call.phase != DVM_HEAP_ACTIVE ||
        call.pending_progress)
    {
        fprintf(stderr,"disvm: invalid/foreign/nested audited native release\n");
        exit(1);
    }
    /* Reject before executor growth, queue withdrawal or root publication.
     * Audited native operations may not block while holding a cell guard. */
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return -1;
    }
    if (gchalt)
    {
        return 0;
    }
    process = dvm_native_context_process(dvm_native_context_current());
    if (up->prog != process)
    {
        fprintf(stderr,"disvm: unpublished native call owner\n");
        exit(1);
    }
    if (!dvm_runqueue_release_allowed(process))
    {
        return 0;
    }
    refusal = dvm_managed_workers_native_wait();
    if (refusal)
    {
        return -1;
    }
    if (process)
    {
        process->R = DVM_R;
        if (!dvm_runqueue_suspend(process))
        {
            fprintf(stderr,"disvm: native continuation withdrawal failed\n");
            exit(1);
        }
    }
    dvm_managed_workers_native_suspended();
    /* Event mode already wakes retirement consumers at publication. A
     * bounded shared interval gets its periodic trace request at turn end. */
    if ((!shared || !dvm_runtime_event_cycles) && !dvm_runtime_gc_request())
    {
        fprintf(stderr,"disvm: native GC request failed\n");
        exit(1);
    }
    if (shared && !dvm_queue_probe_requested && dvm_native_owner_try_share(owner, dvm_gc_runtime.passes))
    {
        /* NATIVEWORLD: exclusive -> shared, with guest roots published. */
        dvm_native_execution()->world_last_exit = DVM_NW_SITE_SHARED;
        return 1;
    }
    /* NATIVEWORLD: leave exclusive admission before native work. */
    if (!dvm_native_owner_suspend(owner,dvm_gc_runtime.passes))
    {
        fprintf(stderr,"disvm: native heap release failed\n");
        exit(1);
    }
    dvm_native_execution()->world_last_exit = shared ? DVM_NW_SITE_SHARED : DVM_NW_SITE_NATIVE_CALL;
    if (process && dvm_queue_probe_requested)
    {
        dvm_queue_probe_run(process, dvm_native_context_current());
    }
    if (shared && !dvm_native_owner_enter_shared(owner))
    {
        abort();
    }
    return 1;
}


/* EN: Restore admission after a native call. released records the prior outcome; shared selects its mode.
 * RU: Восстановить допуск после нативного вызова. released хранит результат выхода, shared выбирает режим. */
static void native_enter(int released, int shared)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats call;
    int returning;
    if (released != 0 && released != 1)
    {
        fprintf(stderr,"disvm: invalid native heap reentry token\n");
        exit(1);
    }
    if (!dvm_gc_runtime.world)
    {
        if (released)
        {
            fprintf(stderr,"disvm: native reentry without a live service\n");
            exit(1);
        }
        return;
    }
    if (dvm_native_owner_world(owner) != dvm_gc_runtime.world ||
        !dvm_native_owner_statistics(owner,&call))
    {
        fprintf(stderr,"disvm: foreign native heap reentry\n");
        exit(1);
    }
    if (!released)
    {
        if (call.phase != DVM_HEAP_ACTIVE || call.pending_progress)
        {
            fprintf(stderr,"disvm: missing native heap reentry token\n");
            exit(1);
        }
        return;
    }
    if (call.phase != (shared ? DVM_HEAP_SHARED : DVM_HEAP_NATIVE) || call.pending_progress ||
        dvm_native_context_object_count(dvm_native_context_current()))
    {
        fprintf(stderr,"disvm: invalid native heap reentry phase\n");
        exit(1);
    }
    /* NATIVEWORLD: reenter exclusive admission before restoring the continuation. */
    DVM_NativeExecution *execution = dvm_native_execution();
    execution->world_entry_site = shared ? DVM_NW_SITE_SHARED : DVM_NW_SITE_NATIVE_CALL;
    execution->world_entry_pending = 1;
    returning = dvm_managed_workers_returning(1);
    if (!(shared ? dvm_native_owner_resume_shared(owner) : dvm_native_owner_resume(owner)) ||
        !dvm_native_owner_progress(owner,dvm_gc_runtime.passes))
    {
        fprintf(stderr,"disvm: native heap reentry failed\n");
        exit(1);
    }
    {
        Prog *process = dvm_native_context_process(dvm_native_context_current());
        if (up->prog != process || (process &&
            (DVM_R.FP != process->R.FP || DVM_R.SP != process->R.SP ||
             DVM_R.EX != process->R.EX || DVM_R.TS != process->R.TS ||
             DVM_R.PC != process->R.PC || DVM_R.xpc != process->R.xpc ||
             DVM_R.IC != process->R.IC ||
             DVM_R.M != process->R.M ||
             DVM_R.MP != process->R.MP || DVM_R.s != process->R.s ||
             DVM_R.m != process->R.m || DVM_R.d != process->R.d ||
             DVM_R.t != process->R.t || DVM_R.st != process->R.st ||
             DVM_R.dt != process->R.dt ||
             !dvm_runqueue_resume(process))))
        {
            fprintf(stderr,"disvm: native continuation restoration failed\n");
            exit(1);
        }
    }
    if (returning)
    {
        dvm_managed_workers_returning(0);
    }
    execution->world_entry_pending = 0;
}


/* EN: Restore the native caller after released indicates whether it left the heap.
 * RU: Вернуть нативного исполнителя. released показывает, выходил ли он из допуска. */
void dvm_runtime_gc_native_enter(int released)
{
    native_enter(released, 0);
}


/* EN: Try to publish roots and leave native heap admission.
 * RU: Попробовать опубликовать корни и выйти из допуска к куче. */
int dvm_runtime_gc_native_try_leave(void)
{
    return native_try_leave(0);
}


/* EN: Try to start a shared guest interval.
 * RU: Попробовать начать параллельный интервал исполнителя. */
int dvm_runtime_gc_shared_try_enter(void)
{
    return native_try_leave(1);
}


/* EN: Publish the current guest registers at a shared boundary.
 * RU: Опубликовать регистры исполнителя на границе параллельного интервала. */
void dvm_runtime_gc_shared_publish(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwnerStats owner;
    Prog *guest = dvm_native_context_process(context);
    if (!guest || guest != up->prog || guest->state != Prelease ||
        dvm_runqueue_owner(guest) != context || dvm_native_context_object_count(context) ||
        !dvm_native_owner_statistics(dvm_native_owner_current(), &owner) ||
        owner.phase != DVM_HEAP_SHARED || owner.pending_progress)
    {
        abort();
    }
    guest->R = DVM_R;
}


/* EN: Poll from an instruction boundary with field guards released.
 * RU: Проверить запросы на границе инструкции после снятия блокировок полей. */
void dvm_runtime_gc_shared_poll(void)
{
    if (!dvm_native_context_poll())
    {
        abort();
    }
}


/* EN: Convert the current claimed turn to read-only shared dispatch.
 * RU: Перевести текущее назначение в параллельный режим чтения инструкции. */
int dvm_runtime_gc_dispatch_share(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    Prog *resident = dvm_native_context_process(context);
    DVM_HeapOwner *owner = dvm_native_owner_current();
    if (!resident || resident->state != Pready || up->prog != resident ||
        dvm_runqueue_dispatch_owner(resident) != context ||
        !dvm_native_owner_try_share(owner, dvm_gc_runtime.passes))
    {
        return 0;
    }
    dvm_native_execution()->world_last_exit = DVM_NW_SITE_DISPATCH;
    return 1;
}


/* EN: Reacquire admission for a legacy handler or error without moving Prog.
 * RU: Вернуть допуск для старого обработчика или ошибки без переноса Prog. */
void dvm_runtime_gc_dispatch_finish(void)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_NativeExecution *execution = dvm_native_execution();
    execution->world_entry_site = DVM_NW_SITE_DISPATCH;
    execution->world_entry_pending = 1;
    if (!dvm_native_owner_resume_shared(owner) ||
        !dvm_native_owner_progress(owner, dvm_gc_runtime.passes))
    {
        abort();
    }
    execution->world_entry_pending = 0;
}


/* EN: Recognize a rooted dispatch, distinct from an audited native lease.
 * RU: Отличить SHARED текущего назначения от нативной аренды. */
int dvm_runtime_gc_dispatch_is_shared(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    Prog *resident = dvm_native_context_process(context);
    DVM_HeapOwnerStats state;
    DVM_HeapOwner *owner = dvm_native_owner_current();
    return resident && resident->state == Pready && up->prog == resident &&
        dvm_runqueue_dispatch_owner(resident) == context &&
        dvm_native_owner_world(owner) == dvm_gc_runtime.world &&
        dvm_native_owner_statistics(owner, &state) &&
        state.phase == DVM_HEAP_SHARED && !state.pending_progress;
}


/* EN: Restore a caller's shared dispatch after completing a legacy operation.
 * RU: Вернуть SHARED вызывающему после завершения старой операции. */
void dvm_runtime_gc_dispatch_restore(void)
{
    Prog *resident = dvm_native_context_process(dvm_native_context_current());
    resident->R = DVM_R;
    if (!dvm_runtime_gc_dispatch_share())
    {
        DVM_HeapOwner *owner = dvm_native_owner_current();
        if (!dvm_native_owner_suspend(owner, dvm_gc_runtime.passes) ||
            !dvm_native_owner_enter_shared(owner))
        {
            abort();
        }
        dvm_native_execution()->world_last_exit = DVM_NW_SITE_DISPATCH;
    }
}


/* EN: Observe exclusive requests without taking one.
 * RU: Проверить наличие эксклюзивных запросов. */
int dvm_runtime_gc_shared_has_exclusive_waiters(void)
{
    return !dvm_gc_runtime.world || dvm_world_exclusive_waiting(dvm_gc_runtime.world) != 0;
}


/* EN: Withdraw the current guest while Fast GC drains queued owners.
 * RU: Вывести текущего исполнителя из допуска на время обработки очереди Fast GC. */
void dvm_runtime_gc_shared_drain(void)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    if (!dvm_native_owner_leave_shared(owner) ||
        !dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_FAST) ||
        !dvm_gc_workers_wait_fast_idle(dvm_gc_runtime.workers) ||
        !dvm_native_owner_enter_shared(owner))
    {
        abort();
    }
}


/* EN: Close the current shared guest interval.
 * RU: Завершить параллельный интервал текущего исполнителя. */
void dvm_runtime_gc_shared_finish(void)
{
    native_enter(1, 1);
}


/* EN: Register a new bound native context and enter its heap interval.
 * RU: Зарегистрировать новый привязанный контекст и войти в допуск к куче. */
int dvm_runtime_gc_attach(void)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    if (!dvm_gc_runtime.world || !up || up->prog ||
        dvm_native_context_process(dvm_native_context_current()))
    {
        return 0;
    }
    dvm_gc_collector_environment();
    if (!dvm_native_owner_attach(owner,dvm_gc_runtime.world))
    {
        return 0;
    }
    /* NATIVEWORLD: a newly attached executor enters exclusive admission. */
    DVM_NativeExecution *execution = dvm_native_execution();
    execution->world_entry_site = DVM_NW_SITE_ATTACH;
    execution->world_entry_pending = 1;
    if (!dvm_native_owner_enter(owner))
    {
        dvm_native_owner_detach(owner);
        return 0;
    }
    dvm_runtime_exception_reserve_start();
    execution->world_entry_pending = 0;
    return 1;
}


/* EN: Retire a non-controller native context after publishing its final roots.
 * RU: Отсоединить нативный контекст исполнителя после публикации его корней. */
int dvm_runtime_gc_detach(void)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    if (!dvm_gc_runtime.world || !up || dvm_native_context_current() == dvm_gc_runtime.owner ||
        up->prog || dvm_native_context_process(dvm_native_context_current()) ||
        dvm_native_owner_world(owner) != dvm_gc_runtime.world)
    {
        return 0;
    }
    {
        DVM_HeapOwnerStats call;
        if (!dvm_native_owner_statistics(owner,&call) || call.pending_progress)
        {
            return 0;
        }
        if (call.phase != DVM_HEAP_ACTIVE && call.phase != DVM_HEAP_IDLE)
        {
            return 0;
        }
        dvm_runtime_exception_reserve_stop();
        if (call.phase == DVM_HEAP_ACTIVE)
        {
            /* NATIVEWORLD: final exclusive exit before unregistering the executor. */
            if (!dvm_native_owner_leave(owner))
            {
                return 0;
            }
            dvm_native_execution()->world_last_exit = DVM_NW_SITE_DETACH;
        }
        return dvm_native_owner_detach(owner);
    }
}
