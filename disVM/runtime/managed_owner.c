/* Copyright (c) 2026 bhgv
 * EN: Rooted managed-ticket capture and native alias lifecycle.
 * RU: Захват managed tickets с корнями и жизнь нативных алиасов.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "managed_owner.h"
#include "gc/common/managed_queue.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "disvm/guard_registry.h"
#include "channel_sync.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


ManagedOwners dvm_managed_owners; /* Single runtime service, all access under mutex. */
static FVM_PortSync *sync;        /* Process-lifetime mutex, published under port registry lock. */


/* Borrow immutable process-lifetime mutex under publication lock, then acquire
 * it without holding registry. No managed reads; NULL means never initialized.
 * RU: Захватить нативный мьютекс службы; NULL означает, что служба ещё не создана. Управляемая память не читается. */
FVM_PortSync *dvm_managed_lock_service(void)
{
    FVM_PortSync *result;
    fvm_port_registry_lock();
    result = sync;
    fvm_port_registry_unlock();
    if (result)
    {
        fvm_port_sync_lock(result);
    }
    return result;
}


/* Fatal native protocol error. message borrowed; no VM callback/longjmp or
 * managed payload read. Corrupted ownership cannot safely resume a mutator.
 * RU: Аварийно завершиться с заимствованным message при нарушении владения; VM callbacks не вызываются. */
void dvm_managed_failure(const char *message)
{
    fprintf(stderr, "disvm managed dvm_managed_owners: %s\n", message);
    abort();
}


/* [GC:] Authorize role BEFORE managed payload reads. Native service identity is
 * read under its mutex; registered ACTIVE world ownership stabilizes lifecycle
 * through the caller's capture/value operation. No inference from refcount.
 * RU: Проверить текущий ACTIVE-контекст, мир, отсутствие незавершённого возврата и наличие error environment; разрешение доступа к payload не создаётся. */
static int admitted(void)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    Proc *host = getup();
    DVM_HeapOwnerStats state;
    DVM_NativeCleanupMark mark;
    FVM_PortSync *mutex;
    int result;
    if (!owner || !dvm_native_owner_statistics(owner, &state) || state.phase != DVM_HEAP_ACTIVE ||
        state.pending_progress || dvm_native_context_object_count(dvm_native_context_current()) ||
        !dvm_native_cleanup_mark(&mark) || !host || !host->env || !host->env->errstr)
    {
        return 0;
    }
    mutex = dvm_managed_lock_service();
    if (!mutex)
    {
        return 0;
    }
    result = dvm_managed_owners.world && dvm_native_owner_world(owner) == dvm_managed_owners.world;
    fvm_port_sync_unlock(mutex);
    return result;
}


/* [GC:] EN: Expose the checked capture role to private access/service modules.
 * RU: Передать проверку роли закрытым модулям доступа и службы. */
int dvm_managed_owner_admitted(void)
{
    return admitted();
}


/* Native aliases retain the ticket, not an unrooted raw managed pointer.
 * object already owned/protected by caller. Return false without resurrection.
 * No managed field/root access or VM entry occurs.
 * RU: Добавить нативного владельца object без доступа к управляемому объекту; вернуть ноль без воскрешения при отказе. */
static int ticket_retain(void *object)
{
    ManagedTicket *ticket = object;
    return dvm_refcount_retain_u(&ticket->references) == DVM_REFCOUNT_LIVE;
}


/* Consume one native owner, including during native error cleanup/NATIVE phase.
 * Pin service before RMW; a nonfinal releaser NEVER reads ticket afterwards.
 * LAST queues without dropping managed root/reference, then wakes Fast outside
 * the queue mutex. Producer pin survives ticket free until request completes.
 * RU: Снять нативного владельца object. Последний публикует ticket с корнем и эпохой; producer удерживает службу до завершения уведомления. */
static void ticket_drop(void *object)
{
    ManagedTicket *ticket = object;
    DVM_RefcountResult result;
    DVM_GCWorkers *workers;
    FVM_PortSync *mutex = dvm_managed_lock_service();
    if (!mutex || !dvm_managed_owners.world || dvm_managed_owners.producers == ULONG_MAX)
    {
        dvm_managed_failure("invalid producer lifecycle");
    }
    dvm_managed_owners.producers++;
    workers = dvm_managed_owners.workers;
    fvm_port_sync_unlock(mutex);
    result = dvm_refcount_release_u(&ticket->references);
    if (result == DVM_REFCOUNT_REFUSED)
    {
        dvm_managed_failure("invalid native ticket owner");
    }
    if (result == DVM_REFCOUNT_LAST)
    {
        ticket->epoch = dvm_world_retire_epoch(ticket->world);
        mutex = dvm_managed_lock_service();
        if (!dvm_managed_owners.live || dvm_managed_owners.pending == ULONG_MAX)
        {
            dvm_managed_failure("invalid retirement accounting");
        }
        dvm_managed_owners.live--;
        ticket->next = NULL;
        if (dvm_managed_owners.tail)
        {
            dvm_managed_owners.tail->next = ticket;
        }
        else
        {
            dvm_managed_owners.head = ticket;
        }
        dvm_managed_owners.tail = ticket;
        dvm_managed_owners.pending++;
        fvm_port_sync_unlock(mutex);
        if (!dvm_gc_workers_request(workers, DVM_GC_FAST))
        {
            mutex = dvm_managed_lock_service();
            dvm_managed_owners.failed = 1;
            fvm_port_sync_unlock(mutex);
        }
    }
    mutex = dvm_managed_lock_service();
    dvm_managed_owners.producers--;
    fvm_port_sync_unlock(mutex);
}

const DVM_OwnerOps dvm_managed_ticket_ops = {ticket_retain,
                                             ticket_drop}; /* Immutable native callbacks, no managed destructor. */


/* [GC:] Publish native service only on its ACTIVE registered owner. No managed
 * allocation/root mutation. Native mutex survives sessions, avoiding stale-lock
 * reads by refused foreign callers during service teardown/restart.
 * RU: Создать службу для world/workers на текущем ACTIVE-контексте. При отказе вернуть ноль; мир и потоки должны жить до полного завершения службы. */
int dvm_managed_owner_start(DVM_NativeWorld *world, DVM_GCWorkers *workers)
{
    DVM_HeapOwnerStats state;
    DVM_WorldAdmission admission;
    FVM_PortSync *candidate;
    FVM_PortSync *mutex;
    DVM_GuardRegistry *registry;
    DVM_CellDomain *cells;
    if (!world || !workers || !dvm_native_owner_statistics(dvm_native_owner_current(), &state) ||
        state.phase != DVM_HEAP_ACTIVE || state.pending_progress ||
        dvm_native_owner_world(dvm_native_owner_current()) != world || !dvm_world_admission(world, &admission) ||
        !admission.serial)
    {
        return 0;
    }
    candidate = fvm_port_sync_create();
    if (!candidate)
    {
        return 0;
    }
    fvm_port_registry_lock();
    if (!sync)
    {
        sync = candidate;
        candidate = NULL;
    }
    fvm_port_registry_unlock();
    if (candidate)
    {
        fvm_port_sync_destroy(candidate);
    }
    registry = dvm_guard_registry_create();
    if (!registry)
    {
        return 0;
    }
    cells = dvm_cell_domain_create();
    if (!cells)
    {
        dvm_guard_registry_destroy(&registry);
        return 0;
    }
    mutex = dvm_managed_lock_service();
    if (dvm_managed_owners.world)
    {
        fvm_port_sync_unlock(mutex);
        dvm_guard_registry_destroy(&registry);
        dvm_cell_domain_destroy(&cells);
        return 0;
    }
    memset(&dvm_managed_owners, 0, sizeof(dvm_managed_owners));
    dvm_managed_owners.world = world;
    dvm_managed_owners.controller = dvm_native_context_current();
    dvm_managed_owners.workers = workers;
    dvm_managed_owners.guards = registry;
    dvm_managed_owners.cells = cells;
    dvm_managed_owners.accepting = 1;
    fvm_port_sync_unlock(mutex);
    return 1;
}


/* [GC:] Register external root through real protected ptradd. argument borrows a
 * live managed Heap; return1 after success. kprotect catches allocation failure
 * without leaking this capture's managed reference or native ticket metadata.
 * RU: Зарегистрировать корень живого Heap argument; вернуть один. Вызывающий kprotect обрабатывает отказ выделения памяти. */
static int add_root(void *argument)
{
    ptradd(argument);
    return 1;
}


/* [GC:] Capture owned source under current ACTIVE admission. Reserve lifecycle
 * metadata first, then retain and register root. Rollback consumes only the new
 * owner; the caller's source owner is still live, so release must be nonfinal.
 * RU: Создать в пустом transfer владельца value с корнем и каноническим sidecar. При отказе сохранить исходное владение; нужен текущий ACTIVE-контекст. */
int dvm_managed_owner_capture(void *value, DVM_OwnerTransfer *transfer)
{
    ManagedTicket *ticket;
    FVM_PortSync *mutex;
    Heap *heap;
    int retained;
    if (!admitted() || !transfer || transfer->object || transfer->ops || !value || value == H)
    {
        return 0;
    }
    ticket = calloc(1, sizeof(*ticket));
    if (!ticket)
    {
        return 0;
    }
    ticket->access = dvm_guard_registry_acquire(dvm_managed_owners.guards, value);
    if (!ticket->access)
    {
        free(ticket);
        return 0;
    }
    mutex = dvm_managed_lock_service();
    if (!dvm_managed_owners.accepting || dvm_managed_owners.tickets == ULONG_MAX ||
        dvm_managed_owners.live == ULONG_MAX)
    {
        fvm_port_sync_unlock(mutex);
        if (!dvm_guard_registry_release(&ticket->access))
        {
            dvm_managed_failure("capture sidecar rollback");
        }
        free(ticket);
        return 0;
    }
    dvm_managed_owners.tickets++;
    dvm_managed_owners.live++;
    ticket->world = dvm_managed_owners.world;
    fvm_port_sync_unlock(mutex);
    heap = D2H(value);
    retained = dvm_refcount_retain_u(&heap->ref) == DVM_REFCOUNT_LIVE;
    if (!retained || !kprotect(add_root, heap))
    {
        if (retained && dvm_refcount_release_u(&heap->ref) != DVM_REFCOUNT_LIVE)
        {
            dvm_managed_failure("capture rollback lost caller source owner");
        }
        mutex = dvm_managed_lock_service();
        dvm_managed_owners.tickets--;
        dvm_managed_owners.live--;
        fvm_port_sync_unlock(mutex);
        if (!dvm_guard_registry_release(&ticket->access))
        {
            dvm_managed_failure("capture sidecar rollback");
        }
        free(ticket);
        return 0;
    }
    Setmark(heap);
    ticket->references = 1;
    ticket->value = value;
    mutex = dvm_managed_lock_service();
    dvm_managed_owners.captured++;
    fvm_port_sync_unlock(mutex);
    transfer->object = ticket;
    transfer->ops = &dvm_managed_ticket_ops;
    return 1;
}


/* [GC:] Capture and register one rooted ticket directly in local cleanup ledger.
 * value is independently owned by caller under ACTIVE registered admission;
 * scope/entry remain current-owner-local. Preflight rejects bad destinations
 * before managed access. No user callback/checkpoint occurs after capture and
 * before accept. Revalidate accept anyway; refusal retires ticket via Fast.
 * No untracked owner escapes to caller and source ownership stays independent.
 * RU: Захватить value в принадлежащие текущему контексту scope/entry. При отказе откатить новый ticket, сохранив владельца исходного значения. */
int dvm_managed_owner_capture_into(void *value, DVM_OwnerScope *scope, DVM_OwnerEntry *entry)
{
    DVM_OwnerTransfer transfer = {0};
    if (!dvm_owner_scope_can_accept(scope, entry) || !dvm_managed_owner_capture(value, &transfer))
    {
        return 0;
    }
    if (!dvm_owner_scope_accept(scope, entry, &transfer))
    {
        if (!dvm_owner_transfer_cancel(&transfer))
        {
            dvm_managed_failure("direct capture rollback lost rooted ticket");
        }
        return 0;
    }
    return 1;
}


/* [GC:] Ticket entry is borrowed and remains linked/lifetime-protected. ACTIVE
 * role is checked first. Do not keep returned pointer across drop/world leave;
 * no implicit lock is granted for future parallel shared mutable access.
 * RU: Получить заимствованное значение связанного entry на его текущем ACTIVE-владельце; неверная роль или чужой мир дают NULL. */
void *dvm_managed_owner_value(const DVM_OwnerEntry *entry)
{
    ManagedTicket *ticket;
    if (!admitted() || !entry || !entry->scope || entry->scope->context != dvm_native_context_current() ||
        entry->ops != &dvm_managed_ticket_ops || !entry->object)
    {
        return NULL;
    }
    ticket = entry->object;
    return ticket->world == dvm_native_owner_world(dvm_native_owner_current()) ? ticket->value : NULL;
}


/* Native queue observation. The process-lifetime mutex, not world admission,
 * protects metadata. No caller receives a root/payload lifetime reservation.
 * RU: Получить число ожидающих tickets под нативным мьютексом. Снимок не удерживает очередь и не разрешает чтение её объектов. */
unsigned long dvm_managed_owner_pending(void)
{
    FVM_PortSync *mutex = dvm_managed_lock_service();
    unsigned long result = 0;
    if (mutex)
    {
        result = dvm_managed_owners.pending;
        fvm_port_sync_unlock(mutex);
    }
    return result;
}
