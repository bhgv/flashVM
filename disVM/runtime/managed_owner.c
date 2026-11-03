/* Managed owners/root entries survive native cleanup; Fast performs heap drops. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "managed_owner.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "disvm/guard_registry.h"
#include "channel_sync.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* One retained managed owner/root per ticket; native aliases share its lifetime. */
typedef struct ManagedTicket
{
    unsigned long references; /* Atomic native owners; no payload access after successful drop. */
    void *value;              /* Immutable managed referent, rooted until Fast retirement. */
    DVM_GuardLease *access; /* Canonical per-referent sidecar, held until before managed drop. */
    DVM_NativeWorld *world;    /* Immutable identity, borrowed through ticket retirement. */
    struct ManagedTicket *next; /* Native queue link, written only after last native drop. */
} ManagedTicket;


/* Protected native service metadata; no managed pointer is dereferenced here. */
typedef struct ManagedOwners
{
    DVM_NativeWorld *world; /* Borrowed through complete ticket/producer retirement. */
    DVM_GCWorkers *workers; /* Borrowed Fast wake target through producer retirement. */
    ManagedTicket *head;    /* Native pending queue, each ticket still owns root/reference. */
    ManagedTicket *tail;    /* Last pending ticket or NULL. */
    unsigned long tickets; /* All captured owners, including live/pending/draining. */
    unsigned long live;    /* Tickets with at least one native owner. */
    unsigned long pending; /* Rooted tickets awaiting collector retirement. */
    unsigned long producers; /* Drop callbacks pin worker lifetime until wake completes. */
    unsigned long captured; /* Completed capture count for joined diagnostics. */
    unsigned long released; /* Completed managed drops for joined diagnostics. */
    DVM_GuardRegistry *guards; /* Owned canonical sidecars; every ticket retains one lease. */
    DVM_CellDomain *cells; /* Allocation-free pointer-cell exclusion for hosted MOVP. */
    int accepting;         /* Capture admission, independent of native alias retention. */
    int failed;            /* Wake failure, never permission to remove roots early. */
} ManagedOwners;

static ManagedOwners owners; /* Single runtime service, all access under mutex. */
static FVM_PortSync *sync; /* Process-lifetime mutex, published under port registry lock. */


/* Borrow immutable process-lifetime mutex under publication lock, then acquire
 * it without holding registry. No managed reads; NULL means never initialized. */
static FVM_PortSync *lock_service(void)
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
 * managed payload read. Corrupted ownership cannot safely resume a mutator. */
static void failure(const char *message)
{
    fprintf(stderr, "disvm managed owners: %s\n", message);
    abort();
}


/* [GC:] Authorize role BEFORE managed payload reads. Native service identity is
 * read under its mutex; serialized ACTIVE world ownership stabilizes lifecycle
 * through the caller's capture/value operation. No inference from refcount. */
static int admitted(void)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    Proc *host = getup();
    DVM_HeapOwnerStats state;
    DVM_NativeCleanupMark mark;
    FVM_PortSync *mutex;
    int result;
    if (!owner || !dvm_native_owner_statistics(owner, &state) ||
        state.phase != DVM_HEAP_ACTIVE || state.pending_progress ||
        dvm_native_context_object_count(dvm_native_context_current()) ||
        !dvm_native_cleanup_mark(&mark) || !host || !host->env || !host->env->errstr)
    {
        return 0;
    }
    mutex = lock_service();
    if (!mutex)
    {
        return 0;
    }
    result = owners.world && dvm_native_owner_world(owner) == owners.world;
    fvm_port_sync_unlock(mutex);
    return result;
}


/* Native aliases retain the ticket, not an unrooted raw managed pointer.
 * object already owned/protected by caller. Return false without resurrection.
 * No managed field/root access or VM entry occurs. */
static int ticket_retain(void *object)
{
    ManagedTicket *ticket = object;
    return dvm_refcount_retain_u(&ticket->references) == DVM_REFCOUNT_LIVE;
}


/* Consume one native owner, including during native error cleanup/NATIVE phase.
 * Pin service before RMW; a nonfinal releaser NEVER reads ticket afterwards.
 * LAST queues without dropping managed root/reference, then wakes Fast outside
 * the queue mutex. Producer pin survives ticket free until request completes. */
static void ticket_drop(void *object)
{
    ManagedTicket *ticket = object;
    DVM_RefcountResult result;
    DVM_GCWorkers *workers;
    FVM_PortSync *mutex = lock_service();
    if (!mutex || !owners.world || owners.producers == ULONG_MAX)
    {
        failure("invalid producer lifecycle");
    }
    owners.producers++;
    workers = owners.workers;
    fvm_port_sync_unlock(mutex);
    result = dvm_refcount_release_u(&ticket->references);
    if (result == DVM_REFCOUNT_REFUSED)
    {
        failure("invalid native ticket owner");
    }
    if (result == DVM_REFCOUNT_LAST)
    {
        mutex = lock_service();
        if (!owners.live || owners.pending == ULONG_MAX)
        {
            failure("invalid retirement accounting");
        }
        owners.live--;
        ticket->next = NULL;
        if (owners.tail)
        {
            owners.tail->next = ticket;
        }
        else
        {
            owners.head = ticket;
        }
        owners.tail = ticket;
        owners.pending++;
        fvm_port_sync_unlock(mutex);
        if (!dvm_gc_workers_request(workers, DVM_GC_FAST))
        {
            mutex = lock_service();
            owners.failed = 1;
            fvm_port_sync_unlock(mutex);
        }
    }
    mutex = lock_service();
    owners.producers--;
    fvm_port_sync_unlock(mutex);
}

static const DVM_OwnerOps ticket_ops = {ticket_retain, ticket_drop}; /* Immutable native callbacks, no managed destructor. */


/* [GC:] Publish native service only on its ACTIVE serialized owner. No managed
 * allocation/root mutation. Native mutex survives sessions, avoiding stale-lock
 * reads by refused foreign callers during service teardown/restart. */
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
        dvm_native_owner_world(dvm_native_owner_current()) != world ||
        !dvm_world_admission(world, &admission) || !admission.serial)
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
    mutex = lock_service();
    if (owners.world)
    {
        fvm_port_sync_unlock(mutex);
        dvm_guard_registry_destroy(&registry);
        dvm_cell_domain_destroy(&cells);
        return 0;
    }
    memset(&owners, 0, sizeof(owners));
    owners.world = world;
    owners.workers = workers;
    owners.guards = registry;
    owners.cells = cells;
    owners.accepting = 1;
    fvm_port_sync_unlock(mutex);
    return 1;
}


/* [GC:] Register external root through real protected ptradd. argument borrows a
 * live managed Heap; return1 after success. kprotect catches allocation failure
 * without leaking this capture's managed reference or native ticket metadata. */
static int add_root(void *argument)
{
    ptradd(argument);
    return 1;
}


/* [GC:] Capture owned source under exclusive ACTIVE admission. Reserve lifecycle
 * metadata first, then retain and register root. Rollback consumes only the new
 * owner; the caller's source owner is still live, so release must be nonfinal. */
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
    ticket->access = dvm_guard_registry_acquire(owners.guards, value);
    if (!ticket->access)
    {
        free(ticket);
        return 0;
    }
    mutex = lock_service();
    if (!owners.accepting || owners.tickets == ULONG_MAX || owners.live == ULONG_MAX)
    {
        fvm_port_sync_unlock(mutex);
        if (!dvm_guard_registry_release(&ticket->access))
        {
            failure("capture sidecar rollback");
        }
        free(ticket);
        return 0;
    }
    owners.tickets++;
    owners.live++;
    ticket->world = owners.world;
    fvm_port_sync_unlock(mutex);
    heap = D2H(value);
    retained = dvm_refcount_retain_u(&heap->ref) == DVM_REFCOUNT_LIVE;
    if (!retained || !kprotect(add_root, heap))
    {
        if (retained && dvm_refcount_release_u(&heap->ref) != DVM_REFCOUNT_LIVE)
        {
            failure("capture rollback lost caller source owner");
        }
        mutex = lock_service();
        owners.tickets--;
        owners.live--;
        fvm_port_sync_unlock(mutex);
        if (!dvm_guard_registry_release(&ticket->access))
        {
            failure("capture sidecar rollback");
        }
        free(ticket);
        return 0;
    }
    Setmark(heap);
    ticket->references = 1;
    ticket->value = value;
    mutex = lock_service();
    owners.captured++;
    fvm_port_sync_unlock(mutex);
    transfer->object = ticket;
    transfer->ops = &ticket_ops;
    return 1;
}


/* [GC:] Capture and register one rooted ticket directly in local cleanup ledger.
 * value is independently owned by caller under ACTIVE serialized admission;
 * scope/entry remain current-owner-local. Preflight rejects bad destinations
 * before managed access. No user callback/checkpoint occurs after capture and
 * before accept. Revalidate accept anyway; refusal retires ticket via Fast.
 * No untracked owner escapes to caller and source ownership stays independent. */
int dvm_managed_owner_capture_into(void *value, DVM_OwnerScope *scope,
    DVM_OwnerEntry *entry)
{
    DVM_OwnerTransfer transfer = {0};
    if (!dvm_owner_scope_can_accept(scope, entry) ||
        !dvm_managed_owner_capture(value, &transfer))
    {
        return 0;
    }
    if (!dvm_owner_scope_accept(scope, entry, &transfer))
    {
        if (!dvm_owner_transfer_cancel(&transfer))
        {
            failure("direct capture rollback lost rooted ticket");
        }
        return 0;
    }
    return 1;
}


/* [GC:] Ticket entry is borrowed and remains linked/lifetime-protected. ACTIVE
 * role is checked first. Do not keep returned pointer across drop/world leave;
 * no implicit lock is granted for future parallel shared mutable access. */
void *dvm_managed_owner_value(const DVM_OwnerEntry *entry)
{
    ManagedTicket *ticket;
    if (!admitted() || !entry || !entry->scope ||
        entry->scope->context != dvm_native_context_current() ||
        entry->ops != &ticket_ops || !entry->object)
    {
        return NULL;
    }
    ticket = entry->object;
    return ticket->world == dvm_native_owner_world(dvm_native_owner_current()) ? ticket->value : NULL;
}


/* Native queue observation. The process-lifetime mutex, not world admission,
 * protects metadata. No caller receives a root/payload lifetime reservation. */
unsigned long dvm_managed_owner_pending(void)
{
    FVM_PortSync *mutex = lock_service();
    unsigned long result = 0;
    if (mutex)
    {
        result = owners.pending;
        fvm_port_sync_unlock(mutex);
    }
    return result;
}


/* [GC:] Only exact stopped collector may remove roots or run retained destroy.
 * Drop queue mutex before managed callbacks. The ticket's root/reference remain
 * owned through dequeue; accounting retires AFTER real destruction completes. */
int dvm_managed_owner_drain(unsigned int budget)
{
    FVM_PortSync *mutex = lock_service();
    DVM_NativeWorld *world;
    if (!mutex)
    {
        return 0;
    }
    world = owners.world;
    fvm_port_sync_unlock(mutex);
    if (!world || !dvm_world_collector_owns(world))
    {
        failure("retirement outside stopped collector");
    }
    while (budget--)
    {
        ManagedTicket *ticket;
        void *value;
        mutex = lock_service();
        ticket = owners.head;
        if (!ticket)
        {
            fvm_port_sync_unlock(mutex);
            break;
        }
        owners.head = ticket->next;
        if (!owners.head)
        {
            owners.tail = NULL;
        }
        owners.pending--;
        fvm_port_sync_unlock(mutex);
        value = ticket->value;
        if (!dvm_guard_registry_release(&ticket->access))
        {
            failure("retiring ticket has a live sidecar borrower");
        }
        ptrdel(D2H(value));
        destroy(value);
        free(ticket);
        mutex = lock_service();
        owners.tickets--;
        owners.released++;
        fvm_port_sync_unlock(mutex);
    }
    return dvm_managed_owner_pending() != 0;
}


/* Native close preflight, used BEFORE irreversible world registration sealing.
 * No live aliases/producers means no later legitimate ticket wake can begin. */
int dvm_managed_owner_can_close(void)
{
    FVM_PortSync *mutex = lock_service();
    int result = 1;
    if (mutex)
    {
        result = !owners.live && !owners.producers;
        fvm_port_sync_unlock(mutex);
    }
    return result;
}


/* [GC:] Close only in the admitted ACTIVE controller phase. Pending rooted work
 * is still drained by Fast; no root/reference is discarded on refusal. */
int dvm_managed_owner_close(void)
{
    FVM_PortSync *mutex;
    if (!admitted())
    {
        return 0;
    }
    mutex = lock_service();
    if (owners.live || owners.producers)
    {
        fvm_port_sync_unlock(mutex);
        return 0;
    }
    owners.accepting = 0;
    fvm_port_sync_unlock(mutex);
    return 1;
}


/* [GC:] Joined/quiescent finish. Do not reset borrowed worker/world pointers on
 * outstanding work. Mutex remains native process-lifetime synchronization. */
int dvm_managed_owner_finish(int diagnostics)
{
    FVM_PortSync *mutex = lock_service();
    int result;
    if (!mutex)
    {
        return 1;
    }
    if (owners.live || owners.pending || owners.producers || owners.tickets || owners.accepting)
    {
        fvm_port_sync_unlock(mutex);
        return 0;
    }
    if (diagnostics)
    {
        printf("MANAGEDOWNERS captured=%lu released=%lu live=%lu pending=%lu tickets=%lu producers=%lu failed=%d rooted_until_fast=1\n",
            owners.captured, owners.released, owners.live, owners.pending, owners.tickets, owners.producers, owners.failed);
    }
    if (owners.cells && !dvm_cell_domain_destroy(&owners.cells))
    {
        fvm_port_sync_unlock(mutex);
        return 0;
    }
    if (owners.guards && !dvm_guard_registry_destroy(&owners.guards))
    {
        fvm_port_sync_unlock(mutex);
        return 0;
    }
    result = !owners.failed;
    owners.world = NULL;
    owners.workers = NULL;
    fvm_port_sync_unlock(mutex);
    return result;
}


/* [GC:] Borrow one canonical sidecar from an already rooted ticket. entry is
 * current-owner-local and linked; exact ACTIVE world is checked before access.
 * Keep entry/key alive and obtain all guards BEFORE locking any of them. This
 * adds no payload access permission; legacy world exclusion still applies. */
DVM_ObjectGuard *dvm_managed_owner_guard(const DVM_OwnerEntry *entry)
{
    ManagedTicket *ticket;
    if (!admitted() || !entry || !entry->scope ||
        entry->scope->context != dvm_native_context_current() ||
        entry->ops != &ticket_ops || !entry->object)
    {
        return NULL;
    }
    ticket = entry->object;
    if (ticket->world != dvm_native_owner_world(dvm_native_owner_current()))
    {
        return NULL;
    }
    return dvm_guard_registry_guard(ticket->access);
}


/* Native metadata only. Holding service sync prevents finish from destroying
 * the registry while its own mutex supplies the count snapshot. No VM callback,
 * root/payload access or persistent node lifetime permission is acquired. */
unsigned long dvm_managed_owner_guard_entries(void)
{
    FVM_PortSync *mutex = lock_service();
    unsigned long result = 0;
    if (mutex)
    {
        result = dvm_guard_registry_entries(owners.guards);
        fvm_port_sync_unlock(mutex);
    }
    return result;
}


/* [GC:] Stable service/world identity is checked before native registry use.
 * Existing serialized admission stabilizes the service and caller-owned spans.
 * Cell guards do not replace source lifetime, roots or future GC epoch proof. */
DVM_CellResult dvm_managed_cells_begin(const void *source, void *destination,
    DVM_CellAccess *access)
{
    if (!admitted())
    {
        return DVM_CELL_REFUSED;
    }
    return dvm_cell_access_begin(owners.cells, source, destination, access);
}


/* [GC:] Service identity and ACTIVE exclusion stabilize domain/span metadata.
 * No ownership/root permission follows; native small-span begin never allocates. */
DVM_CellResult dvm_managed_spans_begin(const void *source, size_t source_size,
    void *destination, size_t destination_size, DVM_CellAccess *access)
{
    if (!admitted())
    {
        return DVM_CELL_REFUSED;
    }
    return dvm_cell_access_spans_begin(owners.cells, source, source_size,
        destination, destination_size, access);
}


/* [GC:] Same role/lifetime contract for wide spans, sharing the SAME domain.
 * Size geometry is not a managed root, provenance check or payload capability. */
DVM_CellResult dvm_managed_ranges_begin(const void *source, size_t source_size,
    void *destination, size_t destination_size, DVM_RangeAccess *access)
{
    if (!admitted())
    {
        return DVM_CELL_REFUSED;
    }
    return dvm_range_access_begin(owners.cells, source, source_size,
        destination, destination_size, access);
}
