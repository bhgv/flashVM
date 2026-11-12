/* Copyright (c) 2026 bhgv
 * EN: Per-guest readiness, native continuation leases and legacy FIFO.
 * RU: Готовность отдельных Prog, нативные ожидания и старая очередь FIFO.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runqueue_private.h"
#include "disvm/refcount.h"
#include "managed_workers.h"
#include "table_leases.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

static FVM_PortSync *ready_sync;    /* Borrowed channel-publication mutex, fixed before startup. */
static int dedicated_dispatch;      /* Startup-only direct dispatch mode. */
static Prog *head;                  /* Runnable FIFO, modified only under the exclusive heap turn. */
static Prog *tail;                  /* Last runnable member, never a native waiter. */
static unsigned int runnable_count; /* Atomic count, including startup publication outside admission. */
static unsigned long claims;        /* Successful exclusive managed-turn assignments. */
static unsigned long finishes;      /* Matching turn releases, independent of native leases. */
static unsigned int active;         /* Live turn claims, including suspended C continuations. */
static unsigned long migrated;      /* Turns of the same Prog assigned to another live native context. */


/* EN: Read the number of published ready processes.
 * RU: Прочитать число процессов с опубликованной готовностью. */
unsigned int dvm_runqueue_ready_count(void)
{
    return __atomic_load_n(&runnable_count, __ATOMIC_ACQUIRE);
}


/* EN: Select direct dispatch before publishing the first Prog.
 * RU: Включить прямой запуск перед публикацией первого Prog. */
void dvm_runqueue_enable_direct(void)
{
    if (dvm_runqueue_ready_count() || dvm_runqueue_leases.inflight || active)
    {
        abort();
    }
    dedicated_dispatch = 1;
}


/* EN: Report whether any ready Prog exists without selecting an owner.
 * RU: Проверить наличие готовых Prog без выбора владельца. */
int dvm_runqueue_ready(void)
{
    return dvm_runqueue_ready_count() != 0;
}


/* Check the FIFO endpoints under the caller's exclusive heap turn.
 * A full traversal is reserved for diagnostics. Return zero on inconsistent
 * endpoint/count or continuation accounting before a mutation. */
int dvm_runqueue_endpoints_valid(void)
{
    /* Dedicated residents do not share FIFO links. Their counters may be
     * sampled between independent updates by different owners. */
    if (dedicated_dispatch)
    {
        return 1;
    }
    if (dvm_runqueue_ready_count() + dvm_runqueue_leases.inflight > DVM_RUNTIME_PROCESS_MAX ||
        dvm_runqueue_leases.inflight > active ||
        dvm_runqueue_leases.leases - dvm_runqueue_leases.returns != dvm_runqueue_leases.inflight ||
        claims - finishes != active)
    {
        return 0;
    }
    if (!dvm_runqueue_ready_count())
    {
        return !head && !tail;
    }
    if (!head || !tail || tail->link)
    {
        return 0;
    }
    if (dvm_runqueue_ready_count() == 1)
    {
        return head == tail && !head->link;
    }
    return head != tail && head->link && head->link != head;
}


/* Assign one runnable head to an idle bound native owner, under heap exclusion.
 * Publish its TLS identity before exposing the claim; failure mutates nothing.
 * Live resident registers are never restored from the diagnostic Prog.R image.
 * RU: Привязать готовый process к текущему владельцу без восстановления живых
 * регистров из диагностической копии Prog.R. При отказе состояние не меняется. */
int dvm_runqueue_claim(Prog *process)
{
    DVM_NativeContext *context = dvm_native_context_current();
    if (!context || !process || (!dedicated_dispatch && head != process) || process->state != Pready ||
        !dvm_runqueue_is_ready(process) || runtime_process(process)->channel_claimed || dvm_runqueue_owner(process) ||
        dvm_runqueue_dispatch_owner(process) || dvm_native_context_process(context) ||
        __atomic_load_n(&active, __ATOMIC_RELAXED) == DVM_RUNTIME_PROCESS_MAX || !dvm_runqueue_endpoints_valid())
    {
        return 0;
    }
    if (!dvm_native_context_acquire_turn(context, process))
    {
        return 0;
    }
    if (!dvm_native_context_set_process(context, process))
    {
        (void)dvm_native_context_release_turn(context, process);
        return 0;
    }
    __atomic_store_n(&runtime_process(process)->dispatch_owner, context, __ATOMIC_RELEASE);
    if (runtime_process(process)->last_dispatcher && runtime_process(process)->last_dispatcher != (uintptr)context)
    {
        __atomic_fetch_add(&migrated, 1, __ATOMIC_RELAXED);
    }
    runtime_process(process)->last_dispatcher = (uintptr)context;
    __atomic_fetch_add(&claims, 1, __ATOMIC_RELAXED);
    __atomic_fetch_add(&active, 1, __ATOMIC_RELAXED);
    return 1;
}


/* Release one complete managed turn on its original owner. Error cleanup may
 * already clear TLS process roots, but cannot clear the native dispatch claim.
 * The caller must first finish any native lease and publish stable registers. */
int dvm_runqueue_finish(Prog *process)
{
    DVM_NativeContext *context = dvm_native_context_current();
    void *current = dvm_native_context_process(context);
    if (!context || !process || dvm_runqueue_dispatch_owner(process) != context || dvm_runqueue_owner(process) ||
        !__atomic_load_n(&active, __ATOMIC_RELAXED) || (current && current != process))
    {
        return 0;
    }
    if (current && !dvm_native_context_set_process(context, NULL))
    {
        return 0;
    }
    if (!dvm_native_context_release_turn(context, process))
    {
        if (current)
        {
            (void)dvm_native_context_set_process(context, current);
        }
        return 0;
    }
    __atomic_store_n(&runtime_process(process)->dispatch_owner, NULL, __ATOMIC_RELEASE);
    __atomic_fetch_add(&finishes, 1, __ATOMIC_RELAXED);
    __atomic_fetch_sub(&active, 1, __ATOMIC_RELEASE);
    return 1;
}


/* Validate the bounded FIFO and inspect one borrowed process. All reads require
 * exclusive heap ownership; -1 reports cycles/count/state/lease corruption. */
int dvm_runqueue_contains(Prog *process)
{
    Prog *cursor;
    Prog *last = NULL;
    unsigned int seen = 0;
    int found = 0;
    if (!dvm_runqueue_endpoints_valid())
    {
        return -1;
    }
    if (dedicated_dispatch)
    {
        return process && dvm_runqueue_is_ready(process);
    }
    for (cursor = head; cursor; cursor = cursor->link)
    {
        if (++seen > dvm_runqueue_ready_count() || cursor->state != Pready || !dvm_runqueue_is_ready(cursor) ||
            dvm_runqueue_owner(cursor) || runtime_process(cursor)->ready_previous != last)
        {
            return -1;
        }
        if (cursor == process)
        {
            found = 1;
        }
        last = cursor;
    }
    return seen == dvm_runqueue_ready_count() && last == tail && (!process || dvm_runqueue_is_ready(process) == found)
               ? found
               : -1;
}


/* Return the borrowed head without polling or modifying queue/root storage.
 * The caller already holds the heap turn, including before first scheduler entry. */
Prog *dvm_runqueue_head(void)
{
    if (dedicated_dispatch)
    {
        Prog *current = dvm_native_context_process(dvm_native_context_current());
        return current && dvm_runqueue_is_ready(current) ? current : NULL;
    }
    return head;
}


/* EN: Select process for its dedicated thread. Direct mode leaves all ready
 * states untouched. Legacy mode reorders the FIFO.
 * RU: Выбрать process для его потока. Прямой режим сохраняет состояния
 * готовности. Старый режим меняет порядок FIFO. */
Prog *dvm_runqueue_select_guest(Prog *process)
{
    Prog *previous;
    if (!process || !dvm_runqueue_is_ready(process) || process->state != Pready ||
        runtime_process(process)->channel_claimed || dvm_runqueue_dispatch_owner(process) ||
        dvm_runqueue_owner(process) || !dvm_runqueue_endpoints_valid())
    {
        return NULL;
    }
    if (dedicated_dispatch)
    {
        return process;
    }
    previous = runtime_process(process)->ready_previous;
    if (previous ? previous->link != process : head != process)
    {
        return NULL;
    }
    if (previous)
    {
        previous->link = process->link;
        if (process->link)
        {
            runtime_process(process->link)->ready_previous = previous;
        }
        else
        {
            tail = previous;
        }
        runtime_process(head)->ready_previous = process;
        process->link = head;
        runtime_process(process)->ready_previous = NULL;
        head = process;
    }
    return process;
}


/* Append one borrowed runtime Prog, refusing native leases and duplicates before
 * mutation. Each withdrawn native continuation reserves one slot for return.
 * No allocation/throw; success gives the FIFO one runnable member. */
static int append_ready(Prog *process, int complete_wait, char *cause)
{
    /* Table wake holds the table scheduler lock. A dedicated resident can
     * become ready before it finishes its own parked turn on the same thread. */
    DVM_NativeContext *dispatch = dvm_runqueue_dispatch_owner(process);
    int table_wake = process && dedicated_dispatch && !complete_wait && process->state == DVM_TABLE_WAIT;
    if (!process || dvm_runqueue_owner(process) || process->link ||
        (!table_wake && dispatch && dispatch != dvm_native_context_current() &&
         !(dedicated_dispatch && complete_wait && runtime_process(process)->channel_claimed &&
           process->state != Palt)) ||
        dvm_runqueue_ready_count() + __atomic_load_n(&dvm_runqueue_leases.inflight, __ATOMIC_RELAXED) >=
            DVM_RUNTIME_PROCESS_MAX ||
        dvm_runqueue_is_ready(process) || !dvm_runqueue_endpoints_valid())
    {
        return 0;
    }
    if (complete_wait)
    {
        if (process->state != Psend && process->state != Precv && process->state != Palt)
        {
            return 0;
        }
        DVM_MessageEndpoint *messages = runtime_process(process)->messages;
        if (messages)
        {
            DVM_OwnerMessage *message;
            while ((message = dvm_messages_take(messages, 0, 0)))
            {
                dvm_runtime_wait_request_finished(process, message);
            }
        }
        process->ptr = nil;
        process->chan = nil;
        if (cause)
        {
            process->kill = cause;
        }
    }
    if (complete_wait && ready_sync)
    {
        fvm_port_sync_lock(ready_sync);
    }
    dvm_runqueue_set_state(process, Pready);
    __atomic_store_n(&runtime_process(process)->queued, 1, __ATOMIC_RELEASE);
    if (!dedicated_dispatch)
    {
        runtime_process(process)->ready_previous = tail;
        if (tail)
        {
            tail->link = process;
        }
        else
        {
            head = process;
        }
        tail = process;
    }
    __atomic_fetch_add(&runnable_count, 1, __ATOMIC_RELEASE);
    if (complete_wait && ready_sync)
    {
        fvm_port_sync_unlock(ready_sync);
    }
    if (!runtime_process(process)->channel_claimed)
    {
        dvm_managed_workers_wake_guest(process);
    }
    return 1;
}


/* EN: Publish a prepared resident from its dedicated thread before admission.
 * RU: Опубликовать подготовленный процесс из его потока до допуска к куче. */
int dvm_runqueue_publish_startup(Prog *process)
{
    DVM_RuntimeProcess *resident = runtime_process(process);
    if (!dedicated_dispatch || !__atomic_load_n(&resident->startup_pending, __ATOMIC_ACQUIRE))
    {
        return 0;
    }
    /* The creator reserved a process slot and initialized Pready before spawn. */
    __atomic_fetch_add(&runnable_count, 1, __ATOMIC_RELEASE);
    __atomic_store_n(&resident->queued, 1, __ATOMIC_RELEASE);
    __atomic_store_n(&resident->startup_pending, 0, __ATOMIC_RELEASE);
    dvm_managed_workers_wake();
    return 1;
}


/* Append a borrowed runtime process after validation. Keep its wait fields. */
int dvm_runqueue_append(Prog *process)
{
    return append_ready(process, 0, nil);
}


/* Complete a detached channel wait. Preserve an existing kill if cause is nil.
 * Return zero without mutation on admission refusal. NativeWorld is required. */
int dvm_runqueue_complete_channel_wait(Prog *process, char *cause)
{
    return append_ready(process, 1, cause);
}


/* EN: Withdraw ready process and set state without changing its GC links.
 * RU: Снять готовый process и установить state, сохранив связи с GC. */
static Prog *withdraw_ready(Prog *process, int state)
{
    DVM_NativeContext *dispatch = dvm_runqueue_dispatch_owner(process);
    if (!process || !dvm_runqueue_ready_count() || !dvm_runqueue_is_ready(process) ||
        runtime_process(process)->channel_claimed || (dispatch && dispatch != dvm_native_context_current()) ||
        dvm_runqueue_owner(process) || process->state != Pready || !dvm_runqueue_endpoints_valid())
    {
        return NULL;
    }
    if (!dedicated_dispatch)
    {
        head = process->link;
        if (head)
        {
            runtime_process(head)->ready_previous = NULL;
        }
        else
        {
            tail = NULL;
        }
    }
    __atomic_fetch_sub(&runnable_count, 1, __ATOMIC_RELEASE);
    __atomic_store_n(&runtime_process(process)->queued, 0, __ATOMIC_RELEASE);
    runtime_process(process)->ready_previous = NULL;
    process->link = NULL;
    dvm_runqueue_set_state(process, state);
    dvm_managed_workers_wake();
    return process;
}


/* EN: Withdraw the legacy head and set its state.
 * RU: Снять голову старой очереди и установить state. */
Prog *dvm_runqueue_pop(int state)
{
    return withdraw_ready(head, state);
}


/* EN: Withdraw expected directly, or verify it is the legacy head first.
 * RU: Снять expected напрямую или проверить, что он первый в старой очереди. */
Prog *dvm_runqueue_pop_expected(Prog *expected, int state)
{
    return dedicated_dispatch || head == expected ? withdraw_ready(expected, state) : NULL;
}


/* EN: Report resident dispatches still completing a transition.
 * RU: Проверить, заканчивает ли какой-либо поток переход состояния. */
int dvm_runqueue_dispatch_pending(void)
{
    return __atomic_load_n(&active, __ATOMIC_ACQUIRE) != 0;
}


/* Print native counters after owner/collector joins; no managed/root operations.
 * A successful managed return or ordinary error unwind must have inflight=0. */
void dvm_runqueue_statistics(void)
{
    fprintf(stderr,
            "NATIVEQUEUE queued=%u leases=%lu returns=%lu inflight=%lu peak=%lu\n",
            dvm_runqueue_ready_count(),
            dvm_runqueue_leases.leases,
            dvm_runqueue_leases.returns,
            dvm_runqueue_leases.inflight,
            dvm_runqueue_leases.peak);
    fprintf(
        stderr, "NATIVEDISPATCH claims=%lu finishes=%lu active=%u migrated=%lu\n", claims, finishes, active, migrated);
}


/* EN: Restore process readiness on its validated original owner; legacy FIFO
 * links and ready publication stay in this coordinator. No register reload.
 * RU: Восстановить готовность process на проверенном исходном владельце.
 * Связи старой FIFO и публикация готовности остаются здесь, без загрузки REG. */
void dvm_runqueue_restore_ready(Prog *process)
{
    __atomic_store_n(&runtime_process(process)->native_owner, NULL, __ATOMIC_RELEASE);
    __atomic_store_n(&runtime_process(process)->queued, 1, __ATOMIC_RELEASE);
    dvm_runqueue_set_state(process, Pready);
    if (!dedicated_dispatch)
    {
        process->link = head;
        if (head)
        {
            runtime_process(head)->ready_previous = process;
        }
        head = process;
        if (!tail)
        {
            tail = process;
        }
    }
    __atomic_fetch_add(&runnable_count, 1, __ATOMIC_RELEASE);
}


/* EN: Configure only before native actors start; caller keeps sync alive.
 * Message callbacks run before this mutex, which guards only ready publication.
 * RU: Настроить до запуска нативных потоков; sync остаётся живым у вызывающего.
 * Обработка сообщений идёт до мьютекса; он защищает лишь публикацию готовности. */
void dvm_runqueue_ready_sync(FVM_PortSync *sync)
{
    if (__atomic_load_n(&active, __ATOMIC_ACQUIRE) || dvm_runqueue_inflight())
    {
        abort();
    }
    ready_sync = sync;
}


/* EN: Publish only the scalar state after the authorized execution transition.
 * process is live and borrowed, state is its new value; no locks or errors.
 * RU: Опубликовать скаляр после разрешённого перехода исполнения. process живой
 * и заимствован, state — новое значение; без блокировок и ошибок. */
void dvm_runqueue_set_state(Prog *process, int state)
{
    process->state = state;
    __atomic_store_n(&runtime_process(process)->observed_state, state, __ATOMIC_RELEASE);
}


/* EN: Observe a live borrowed process without reading its owner-local state.
 * Caller protects lifetime; result is a scalar hint, not access to other fields.
 * RU: Наблюдать живой заимствованный process без чтения локального state владельца.
 * Caller защищает lifetime; результат — скаляр, не доступ к остальным полям. */
int dvm_runqueue_observed_state(Prog *process)
{
    return __atomic_load_n(&runtime_process(process)->observed_state, __ATOMIC_ACQUIRE);
}
