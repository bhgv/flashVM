/* Per-file native FIFO. Finish releases the gate before reacquiring the heap. */
#include "file_gate.h"
#include "channel_sync.h"
#include "disvm/native_context.h"
#include <stdio.h>
#include <stdlib.h>


/* Only next/serving/busy/counters are shared outside runtime heap exclusion. */
struct DVM_FileGate
{
    FVM_PortSync *sync;          /* Protect ticket state, never held through I/O. */
    DVM_FileGate *next_gate;     /* Live native registry link, under heap exclusion. */
    size_t next;                /* Next reserved ticket; unsigned wrap is intentional. */
    size_t serving;             /* Ticket allowed to use the native file cursor. */
    size_t operations;          /* Completed native operations under sync. */
    size_t waiting;             /* Admissions that observed an earlier operation. */
    size_t peak;                /* Maximum outstanding reservations for this file. */
    DVM_NativeContext *owner;   /* Borrowed native TLS owner only through the admitted operation. */
    int busy;                   /* Exactly one admitted native operation. */
};

static DVM_FileGate *gates;     /* Live gates, modified only under heap exclusion. */
static size_t created;          /* Successful allocations under heap exclusion. */
static size_t closed;           /* Successful retirements under heap exclusion. */
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
    gate->next_gate = gates;
    gates = gate;
    created++;
    return gate;
}


/* Reserve while ACTIVE so dispatch order determines file-operation order.
 * The FD/private slot keeps gate alive even while the owner waits natively. */
size_t dvm_file_gate_reserve(DVM_FileGate *gate)
{
    size_t ticket;
    size_t pending;
    if (!gate)
    {
        return 0;
    }
    fvm_port_sync_lock(gate->sync);
    ticket = gate->next++;
    pending = gate->next - gate->serving;
    if (pending > gate->peak)
    {
        gate->peak = pending;
    }
    fvm_port_sync_unlock(gate->sync);
    return ticket;
}


/* Native-only wait. Do not hold heap ownership around a contended call unless
 * the caller is a protected synchronous callback that cannot release its turn. */
int dvm_file_gate_enter(DVM_FileGate *gate, size_t ticket)
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
    if (ticket != gate->serving || gate->busy)
    {
        gate->waiting++;
    }
    while (ticket != gate->serving || gate->busy)
    {
        fvm_port_sync_wait(gate->sync);
    }
    gate->busy = 1;
    gate->owner = dvm_native_context_current();
    fvm_port_sync_unlock(gate->sync);
    return 1;
}


/* Finish before native lease reentry, allowing waiting operations to progress
 * independently of the heap. Wrong ticket/duplicate completion refuses. */
int dvm_file_gate_leave(DVM_FileGate *gate, size_t ticket)
{
    if (!gate)
    {
        return 1;
    }
    fvm_port_sync_lock(gate->sync);
    if (!gate->busy || ticket != gate->serving || gate->owner != dvm_native_context_current())
    {
        fvm_port_sync_unlock(gate->sync);
        return 0;
    }
    gate->busy = 0;
    gate->owner = NULL;
    gate->serving++;
    gate->operations++;
    fvm_port_sync_signal_all(gate->sync);
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
    fvm_port_sync_lock(gate->sync);
    if (gate->busy || gate->next != gate->serving)
    {
        fvm_port_sync_unlock(gate->sync);
        return 0;
    }
    completed += gate->operations;
    waits += gate->waiting;
    if (gate->peak > peak_pending)
    {
        peak_pending = gate->peak;
    }
    fvm_port_sync_unlock(gate->sync);
    for (slot = &gates; *slot && *slot != gate; slot = &(*slot)->next_gate)
    {
    }
    if (!*slot)
    {
        return 0;
    }
    *slot = gate->next_gate;
    fvm_port_sync_destroy(gate->sync);
    free(gate);
    closed++;
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
        *pending = gate ? gate->next - gate->serving : 0;
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


/* Called after executor joins. Registry lifecycle is excluded by the heap gate;
 * mutexes also make snapshots well-defined if a native operation is inspected. */
void dvm_file_gate_statistics(void)
{
    DVM_FileGate *gate;
    size_t operations = completed;
    size_t contended = waits;
    size_t pending = 0;
    size_t peak = peak_pending;
    for (gate = gates; gate; gate = gate->next_gate)
    {
        fvm_port_sync_lock(gate->sync);
        operations += gate->operations;
        contended += gate->waiting;
        pending += gate->next - gate->serving;
        if (gate->peak > peak)
        {
            peak = gate->peak;
        }
        fvm_port_sync_unlock(gate->sync);
    }
    fprintf(stderr, "NATIVEFILES created=%lu closed=%lu live=%lu operations=%lu waits=%lu pending=%lu peak=%lu\n",
        (unsigned long)created, (unsigned long)closed, (unsigned long)(created - closed),
        (unsigned long)operations, (unsigned long)contended, (unsigned long)pending,
        (unsigned long)peak);
}
