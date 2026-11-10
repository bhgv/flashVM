/* Per-file native FIFO. Finish releases the gate before reacquiring the heap. */
#include "file_gate.h"
#include "channel_sync.h"
#include "disvm/native_context.h"
#include <stdio.h>
#include <stdlib.h>


/* Queue links, admission and counters are protected by the gate mutex. */
struct DVM_FileGate
{
    FVM_PortSync *sync;          /* Protect ticket state, never held through I/O. */
    DVM_FileGate *next_gate;     /* Live native registry link, under the registry mutex. */
    DVM_FileTicket *head;       /* First ticket, including any admitted operation. */
    DVM_FileTicket *tail;       /* Most recently reserved ticket. */
    size_t pending;             /* Number of linked tickets. */
    size_t operations;          /* Completed native operations under sync. */
    size_t waiting;             /* Admissions that observed an earlier operation. */
    size_t sleepers;            /* Parked admissions, including cancelled tickets waking up. */
    size_t peak;                /* Maximum outstanding reservations for this file. */
    DVM_NativeContext *owner;   /* Borrowed native TLS owner only through the admitted operation. */
    int busy;                   /* Exactly one admitted native operation. */
};

static DVM_FileGate *gates;     /* Live gates, modified only under the registry mutex. */
static size_t created;          /* Successful allocations under the registry mutex. */
static size_t closed;           /* Successful retirements under the registry mutex. */
static size_t completed;        /* Operations accumulated from retired gates. */
static size_t waits;            /* Contended admissions accumulated from retired gates. */
static size_t peak_pending;     /* Largest per-file reservation peak, including retired gates. */


/* Allocate native state before any destructive file open; caller owns result.
 * No managed references/heap allocation occur. NULL reports native exhaustion. */
DVM_FileGate *dvm_file_gate_create(void)
{
    DVM_FileGate *gate = calloc(1, sizeof(*gate));
    if (!gate)
    {
        return NULL;
    }
    gate->sync = fvm_port_sync_create();
    if (!gate->sync)
    {
        free(gate);
        return NULL;
    }
    fvm_port_registry_lock();
    gate->next_gate = gates;
    gates = gate;
    created++;
    fvm_port_registry_unlock();
    return gate;
}


/* Append while ACTIVE. The descriptor root keeps the gate alive. */
void dvm_file_gate_reserve(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    ticket->gate = gate;
    ticket->previous = NULL;
    ticket->next = NULL;
    if (!gate)
    {
        return;
    }
    fvm_port_sync_lock(gate->sync);
    ticket->previous = gate->tail;
    if (gate->tail)
    {
        gate->tail->next = ticket;
    }
    else
    {
        gate->head = ticket;
    }
    gate->tail = ticket;
    gate->pending++;
    if (gate->pending > gate->peak)
    {
        gate->peak = gate->pending;
    }
    fvm_port_sync_unlock(gate->sync);
}


/* Remove a linked ticket with the gate mutex held and wake contenders. */
static void unlink_ticket(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    if (ticket->previous)
    {
        ticket->previous->next = ticket->next;
    }
    else
    {
        gate->head = ticket->next;
    }
    if (ticket->next)
    {
        ticket->next->previous = ticket->previous;
    }
    else
    {
        gate->tail = ticket->previous;
    }
    ticket->gate = NULL;
    ticket->previous = NULL;
    ticket->next = NULL;
    gate->pending--;
    fvm_port_sync_signal_all(gate->sync);
}


/* Cancel a queued operation without waiting for earlier native I/O. */
int dvm_file_gate_cancel(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    int cancelled;

    if (!gate)
    {
        return 0;
    }
    fvm_port_sync_lock(gate->sync);
    cancelled = ticket->gate == gate && (!gate->busy || gate->head != ticket);
    if (cancelled)
    {
        unlink_ticket(gate, ticket);
    }
    fvm_port_sync_unlock(gate->sync);
    return cancelled;
}


/* Wait for the queue head or cancellation with no managed access. */
static int enter_ticket(DVM_FileGate *gate, DVM_FileTicket *ticket, int block)
{
    if (!gate)
    {
        return 1;
    }
    if (!dvm_native_context_current())
    {
        return 0;
    }
    fvm_port_sync_lock(gate->sync);
    if (gate->busy && ticket == gate->head)
    {
        fvm_port_sync_unlock(gate->sync);
        return 0;
    }
    if ((!block || gate->owner == dvm_native_context_current()) &&
        ticket->gate == gate && (ticket != gate->head || gate->busy))
    {
        unlink_ticket(gate, ticket);
        fvm_port_sync_unlock(gate->sync);
        return 0;
    }
    if (ticket->gate == gate && (ticket != gate->head || gate->busy))
    {
        gate->waiting++;
        gate->sleepers++;
        while (ticket->gate == gate && (ticket != gate->head || gate->busy))
        {
            fvm_port_sync_wait(gate->sync);
        }
        gate->sleepers--;
    }
    if (ticket->gate != gate)
    {
        fvm_port_sync_unlock(gate->sync);
        return 0;
    }
    gate->busy = 1;
    gate->owner = dvm_native_context_current();
    fvm_port_sync_unlock(gate->sync);
    return 1;
}


/* Wait outside NativeWorld until this reservation reaches the head. */
int dvm_file_gate_enter(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    return enter_ticket(gate, ticket, 1);
}


/* Admit only an idle head; a protected caller cannot park with the heap. */
int dvm_file_gate_enter_now(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    return enter_ticket(gate, ticket, 0);
}


/* Finish before reentering the heap. Wrong ticket or owner refuses. */
int dvm_file_gate_leave(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    if (!gate)
    {
        return 1;
    }
    fvm_port_sync_lock(gate->sync);
    if (!gate->busy || ticket != gate->head || gate->owner != dvm_native_context_current())
    {
        fvm_port_sync_unlock(gate->sync);
        return 0;
    }
    gate->busy = 0;
    gate->owner = NULL;
    gate->operations++;
    unlink_ticket(gate, ticket);
    fvm_port_sync_unlock(gate->sync);
    return 1;
}


/* Registry changes require heap exclusion. FD pins must make pending retirement
 * impossible; refuse rather than freeing mutex/storage under a native borrower. */
int dvm_file_gate_destroy(DVM_FileGate *gate)
{
    DVM_FileGate **slot;
    if (!gate)
    {
        return 1;
    }
    fvm_port_registry_lock();
    fvm_port_sync_lock(gate->sync);
    if (gate->busy || gate->pending || gate->sleepers)
    {
        fvm_port_sync_unlock(gate->sync);
        fvm_port_registry_unlock();
        return 0;
    }
    for (slot = &gates; *slot && *slot != gate; slot = &(*slot)->next_gate)
    {
    }
    if (!*slot)
    {
        fvm_port_sync_unlock(gate->sync);
        fvm_port_registry_unlock();
        return 0;
    }
    completed += gate->operations;
    waits += gate->waiting;
    if (gate->peak > peak_pending)
    {
        peak_pending = gate->peak;
    }
    *slot = gate->next_gate;
    closed++;
    fvm_port_sync_unlock(gate->sync);
    fvm_port_registry_unlock();
    fvm_port_sync_destroy(gate->sync);
    free(gate);
    return 1;
}


/* Outputs are native caller-owned scalars; no lifetime is extended by copying. */
void dvm_file_gate_snapshot(DVM_FileGate *gate, size_t *pending, size_t *waiting)
{
    if (gate)
    {
        fvm_port_sync_lock(gate->sync);
    }
    if (pending)
    {
        *pending = gate ? gate->pending : 0;
    }
    if (waiting)
    {
        *waiting = gate ? gate->waiting : 0;
    }
    if (gate)
    {
        fvm_port_sync_unlock(gate->sync);
    }
}


/* Snapshot the live registry and each gate in registry-to-gate lock order. */
void dvm_file_gate_statistics(void)
{
    DVM_FileGate *gate;
    fvm_port_registry_lock();
    size_t operations = completed;
    size_t contended = waits;
    size_t pending = 0;
    size_t peak = peak_pending;
    for (gate = gates; gate; gate = gate->next_gate)
    {
        fvm_port_sync_lock(gate->sync);
        operations += gate->operations;
        contended += gate->waiting;
        pending += gate->pending;
        if (gate->peak > peak)
        {
            peak = gate->peak;
        }
        fvm_port_sync_unlock(gate->sync);
    }
    size_t opened = created;
    size_t retired = closed;
    fvm_port_registry_unlock();
    fprintf(stderr, "NATIVEFILES created=%lu closed=%lu live=%lu operations=%lu waits=%lu pending=%lu peak=%lu\n",
        (unsigned long)opened, (unsigned long)retired, (unsigned long)(opened - retired),
        (unsigned long)operations, (unsigned long)contended, (unsigned long)pending,
        (unsigned long)peak);
}
