/* Copyright (c) 2026 bhgv
 *
 * EN: GC lifecycle, notifications and coordination with DisVM execution.
 * RU: Запуск и остановка GC, уведомления и взаимодействие с исполнением DisVM.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "disvm/gc_workers.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include "runtime_gc.h"
#include "gc/collector_passes.h"
#include "gc/common/runtime_state.h"
#include "channel_sync.h"
#include "exception_host.h"
#include "runtime_rc.h"
#include "shared_pointer.h"
#include "shared_mapped.h"
#include "shared_bytes.h"
#include "shared_arithmetic.h"
#include "managed_owner.h"
#include "managed_owner_probe.h"
#include "runqueue.h"
#include "managed_workers.h"
#include "queue_probe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern ulong gcepochs; /* Completed retained tracing epochs. */
int dvm_runtime_idle_cycles; /* Startup-only quiescent cycle admission. */
int dvm_runtime_disable_cycles; /* Startup-only Fast-only policy. */
int dvm_runtime_event_cycles; /* Startup-only cycle admission policy. */
extern uvlong gcdestroys; /* Retained cycle collector reclaimed-object count. */
RuntimeGC dvm_gc_runtime;  /* One managed runtime heap, not multi-instance admission. */


/* EN: Publish the registers and process of context under the world stop. size is the register extent, user is runtime state.
 * RU: Опубликовать регистры registers и процесс process контекста context. size задаёт размер, user хранит состояние GC. */
static void publish(DVM_NativeContext *context, void *registers,
    size_t size, void *process, void *user)
{
    RuntimeGC *gc = user;
    USED(context);
    Prog *guest = process;
    if (!guest || guest->state == Prelease)
    {
        return;
    }
    if (!registers || size != sizeof(REG))
    {
        fprintf(stderr, "disvm GC cannot publish live process registers\n");
        abort();
    }
    memmove(&guest->R, registers, sizeof(guest->R));
    gc->snapshots++;
}


/* EN: Create a collector context. user is unused.
 * RU: Создать контекст сборщика. user не используется. */
static DVM_NativeContext *collector_create(void *user)
{
    USED(user);
    return dvm_native_process_create(NULL);
}


/* EN: Signal the native lanes with queued work without taking heap admission.
 * RU: Разбудить нативные потоки с заданиями без захвата допуска к куче. */
static void wake_fast(void)
{
    if (!dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_FAST) ||
        (dvm_runtime_rc_candidates_ready() &&
         !dvm_gc_workers_request_candidates(dvm_gc_runtime.workers)) ||
        (dvm_runtime_rc_duke_ready() &&
         !dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_DUKE)))
    {
        abort();
    }
}


/* EN: Adapt user runtime state to one retained cycle trace. context is bound; budget is unused.
 * RU: Передать состояние user трассировщику. context привязан к потоку, budget не используется. */
static DVM_GCWorkResult cycle_pass(DVM_NativeContext *context, size_t budget, void *user)
{
    RuntimeGC *gc = user;
    (void)context;
    (void)budget;
    return dvm_gc_cycles_trace(gc->roots, &gc->idle_cycle_passes, &gc->idle_cycle_epoch, &gc->passes);
}


/* EN: Start collector lanes for roots. locked_poll selects the mutex polling path.
 * RU: Запустить потоки GC для roots. locked_poll выбирает проверку под мьютексом. */
int dvm_runtime_gc_start(Prog **roots, int locked_poll)
{
    DVM_GCWorkersConfig config;
    DVM_HeapOwner *owner = dvm_native_owner_current();
    int attached = 0;
    if (!roots || dvm_gc_runtime.world)
    {
        return 0;
    }
    memset(&dvm_gc_runtime,0,sizeof(dvm_gc_runtime));
    memset(&config,0,sizeof(config));
    dvm_gc_runtime.roots = roots;
    dvm_gc_runtime.owner = dvm_native_context_current();
    dvm_gc_runtime.world = dvm_world_create_serial(locked_poll);
    if (dvm_gc_runtime.world)
    {
        attached = dvm_native_owner_attach(owner,dvm_gc_runtime.world);
    }
    config.world = dvm_gc_runtime.world;
    config.create_context = collector_create;
    config.roots = publish;
    config.Fast.run = dvm_gc_fast_retained;
    config.fast_ready = dvm_gc_fast_ready;
    config.fast_shared = dvm_gc_fast_shared;
    config.cycles_shared = dvm_gc_cycles_shared;
    config.duke_shared = dvm_gc_duke_shared;
    config.parallel_shared = 1;
    config.duke_budget = 1;
    config.duke_pause_us = 1000;
    config.fast_quiescent_only = 1;
    config.Fast.pause_us = 1000;
    config.Fast.budget = 256;
    config.cycles_quiescent_only = dvm_runtime_idle_cycles;
    config.Cycles.pause_us = dvm_runtime_idle_cycles ? 1000 : 0;
    config.Cycles.run = cycle_pass;
    config.Cycles.budget = 256;
    config.user = &dvm_gc_runtime;
    if (attached)
    {
        dvm_gc_runtime.workers = dvm_gc_workers_create(&config);
    }
    if (dvm_gc_runtime.workers && dvm_native_owner_enter(owner))
    {
        if (dvm_managed_owner_start(dvm_gc_runtime.world, dvm_gc_runtime.workers))
        {
            dvm_runtime_rc_start();
            dvm_runtime_rc_set_notifier(wake_fast);
            dvm_runtime_exception_reserve_start();
            if (dvm_managed_owner_probe_requested)
            {
                dvm_managed_owner_probe_run(dvm_gc_runtime.world, dvm_gc_runtime.workers);
            }
            return 1;
        }
        dvm_native_owner_leave(owner);
    }
    if (dvm_gc_runtime.workers)
    {
        dvm_gc_workers_destroy(dvm_gc_runtime.workers);
    }
    if (attached)
    {
        dvm_native_owner_detach(owner);
    }
    dvm_world_destroy(dvm_gc_runtime.world);
    memset(&dvm_gc_runtime,0,sizeof(dvm_gc_runtime));
    return 0;
}


/* EN: Record a managed owner loss for event-driven cycle requests.
 * RU: Зарегистрировать снижение числа владельцев для запуска сборки циклов по событиям. */
void dvm_runtime_gc_owner_lost(void)
{
    if (dvm_runtime_event_cycles && dvm_gc_runtime.world)
    {
        fvm_port_registry_lock();
        dvm_gc_runtime.cycle_losses++;
        dvm_gc_runtime.cycle_epoch = gcepochs;
        dvm_gc_runtime.clean_requests = 0;
        fvm_port_registry_unlock();
    }
}


/* EN: Coalesce collector requests from the admitted runtime owner.
 * RU: Объединить запросы сборщиков от владельца с действующим допуском. */
int dvm_runtime_gc_request(void)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats call;
    if (!dvm_gc_runtime.world || dvm_native_owner_world(owner) != dvm_gc_runtime.world ||
        !dvm_native_owner_statistics(owner,&call) || call.phase != DVM_HEAP_ACTIVE ||
        call.pending_progress)
    {
        return 0;
    }
    if (dvm_managed_owner_pending())
    {
        if (!dvm_gc_workers_request(dvm_gc_runtime.workers,DVM_GC_FAST))
        {
            return 0;
        }
    }
    else
    {
        dvm_gc_runtime.fast_empty_skips++;
    }
    if (dvm_runtime_disable_cycles)
    {
        return 1;
    }
    if (dvm_runtime_event_cycles)
    {
        fvm_port_registry_lock();
        if (gcepochs - dvm_gc_runtime.cycle_epoch >= 3)
        {
            if (++dvm_gc_runtime.clean_requests < 256)
            {
                dvm_gc_runtime.cycle_skips++;
                fvm_port_registry_unlock();
                return 1;
            }
            dvm_gc_runtime.clean_requests = 0;
            dvm_gc_runtime.cycle_epoch = gcepochs;
        }
        fvm_port_registry_unlock();
    }
    return dvm_gc_workers_request(dvm_gc_runtime.workers,DVM_GC_CYCLES);
}


/* EN: Close admission, drain collectors and join. diagnostics enables statistics.
 * RU: Закрыть допуск, обработать очереди и дождаться потоков. diagnostics включает статистику. */
int dvm_runtime_gc_stop(int diagnostics)
{
    DVM_GCWorkersState workers;
    DVM_WorldAdmission admission;
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats call;
    int ok;
    size_t fast_polls = 0;
    size_t locked_polls = 0;
    if (!dvm_gc_runtime.world)
    {
        return 1;
    }
    if (dvm_native_context_current() != dvm_gc_runtime.owner ||
        !dvm_native_owner_statistics(owner,&call) || call.phase != DVM_HEAP_ACTIVE ||
        call.pending_progress || dvm_runqueue_inflight() || !dvm_managed_owner_can_close() || !dvm_native_owner_seal(owner) ||
        !dvm_world_admission(dvm_gc_runtime.world,&admission) || !admission.serial || admission.waiting)
    {
        return 0;
    }
    if (!dvm_native_owner_polls(owner,&fast_polls,&locked_polls))
    {
        return 0;
    }
    if (!dvm_managed_owner_close())
    {
        return 0;
    }
    dvm_runtime_exception_reserve_stop();
    dvm_runtime_rc_close();
    if (dvm_runtime_rc_ready())
    {
        wake_fast();
    }
    dvm_native_context_set_process(dvm_native_context_current(),NULL);
    ok = dvm_runtime_gc_request();
    if (!dvm_native_owner_leave(owner))
    {
        ok = 0;
    }
    if (!dvm_gc_workers_wait_idle(dvm_gc_runtime.workers))
    {
        ok = 0;
    }
    else if (dvm_runtime_idle_cycles &&
        (!dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_CYCLES) ||
         !dvm_gc_workers_wait_idle(dvm_gc_runtime.workers)))
    {
        /* Queue owners may have been released after the last retained pass. */
        ok = 0;
    }
    if (!dvm_managed_owner_finish(diagnostics))
    {
        return 0;
    }
    memset(&workers,0,sizeof(workers));
    if (!dvm_gc_workers_state(dvm_gc_runtime.workers,&workers))
    {
        ok = 0;
    }
    if (!dvm_gc_workers_destroy(dvm_gc_runtime.workers))
    {
        ok = 0;
    }
    if (!dvm_runtime_rc_finish(diagnostics))
    {
        ok = 0;
    }
    if (diagnostics)
    {
        printf("NATIVEGCLANES shared_peak=%u duke_passes=%zu\n",
            workers.shared_peak, workers.Duke.passes);
        printf("NATIVEGC collector=worker passes=%lu snapshots=%lu reclaimed=%llu joined=1 failed=%d fast_passes=%lu workers=3 fast_polls=%lu locked_polls=%lu empty_fast_skips=%lu native_releases=%lu native_reentries=%lu native_gc_progress=%lu serial_heap=1 admissions=%lu contentions=%lu waiters=%lu owner_state=tls parallel_movp_commits=%lu parallel_slicela_commits=%lu parallel_movmp_commits=%lu parallel_channel_sends=%lu parallel_channel_receives=%lu parallel_channel_refills=%lu parallel_channel_alts=%lu parallel_rendezvous=%lu parallel_movp_module=%lu parallel_movmp_module=%lu parallel_channel_raw=%lu parallel_byte_commits=%lu cycle_losses=%lu cycle_skips=%lu shared_peak=%lu shared_integer_commits=%lu shared_integer_batches=%lu shared_integer_branches=%lu shared_byte_steps=%lu shared_long_steps=%lu shared_calls=%lu shared_returns=%lu shared_module_calls=%lu shared_module_returns=%lu shared_extended_steps=%lu\n",
            dvm_gc_runtime.passes,dvm_gc_runtime.snapshots,(unsigned long long)gcdestroys,
            workers.failed,(unsigned long)workers.Fast.passes,
            (unsigned long)fast_polls,(unsigned long)locked_polls,dvm_gc_runtime.fast_empty_skips,
            call.releases,call.reentries,call.gc_progress,
            (unsigned long)admission.admissions,(unsigned long)admission.contentions,
            (unsigned long)admission.waiting, dvm_runtime_parallel_movp_commits, dvm_runtime_parallel_slicela_commits,
            dvm_runtime_parallel_movmp_commits, dvm_runtime_parallel_channel_sends,
            dvm_runtime_parallel_channel_receives, dvm_runtime_parallel_channel_refills,
            dvm_runtime_parallel_channel_alts, dvm_runtime_parallel_rendezvous,
            dvm_runtime_parallel_movp_module_commits, dvm_runtime_parallel_movmp_module_commits, dvm_runtime_parallel_channel_raw,
            dvm_runtime_parallel_byte_commits, dvm_gc_runtime.cycle_losses, dvm_gc_runtime.cycle_skips,
            (unsigned long)dvm_world_shared_peak(dvm_gc_runtime.world), dvm_runtime_shared_integer_commits, dvm_runtime_shared_integer_batches, dvm_runtime_shared_integer_branches,
            dvm_runtime_shared_byte_steps, dvm_runtime_shared_long_steps,
            dvm_runtime_shared_calls, dvm_runtime_shared_returns,
            dvm_runtime_shared_module_calls, dvm_runtime_shared_module_returns,
            dvm_runtime_shared_extended_steps);
    }
    if (!dvm_native_owner_detach(owner) || !dvm_world_destroy(dvm_gc_runtime.world))
    {
        ok = 0;
    }
    memset(&dvm_gc_runtime,0,sizeof(dvm_gc_runtime));
    return ok;
}
