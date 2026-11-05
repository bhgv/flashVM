/* Compose capture, preparation, conditional publication and complete retirement. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "isa.h"
#include "shared_access.h"
#include "raise.h"
#include "pointer_transaction.h"
#include "pointer_prepare.h"
#include "heap_commit.h"
#include <string.h>

#define DVM_POINTER_ATTEMPTS 8 /* Bound pathological callback-induced retry work. */


/* Stable native ledger spans the complete protected body and cleanup errors. */
typedef struct PointerTransaction
{
    void *source_cell; /* Borrowed mapped operand, captured once before callbacks. */
    void *destination; /* Borrowed rooted destination, stable through all attempts. */
    DVM_HeapHold source; /* Exclusive retained source snapshot. */
    DVM_HeapHold tail; /* Exclusive observation for the current publication attempt. */
    DVM_HeapHold candidate; /* Exclusive unpublished complete list-node owner. */
    DVM_HeapHold retired; /* Displaced destination owner after successful commit. */
} PointerTransaction;


/* [GC:] Consume one nonzero exclusive hold from argument. Return1 or propagate
 * real retirement error after its fields were cleared; never retry consumption.
 * Caller runs under protected ACTIVE admission, outside object guards. */
static int finish_one(void *argument)
{
    dvm_runtime_heap_finish(argument, 1);
    return 1;
}


/* [GC:] Run all ledger retirements even when one destructor raises. state is
 * stable and exclusively owned, body_ok is completed kprotect result. Preserve
 * body error first, otherwise first retirement error. Return void or rethrow only
 * after every remaining live hold has been consumed outside object guards. */
static void finish_all(PointerTransaction *state, int body_ok)
{
    DVM_HeapHold *holds[] = {&state->candidate, &state->retired, &state->tail, &state->source};
    char first_error[ERRMAX];
    int failed = !body_ok;
    unsigned int index;
    if (failed)
    {
        memmove(first_error, getup()->env->errstr, sizeof(first_error));
    }
    for (index = 0; index < sizeof(holds)/sizeof(holds[0]); index++)
    {
        if (holds[index]->value && !kprotect(finish_one, holds[index]) && !failed)
        {
            memmove(first_error, getup()->env->errstr, sizeof(first_error));
            failed = 1;
        }
    }
    if (failed)
    {
        memmove(getup()->env->errstr, first_error, sizeof(first_error));
        krethrow();
    }
}


/* [GC:] Capture original source once, including aliased source/destination.
 * state owns stable mapped cells and zero ledger. Each stale candidate is fully
 * discarded before refreshing tail; never publish a node carrying the old tail.
 * Return1 on committed publication, otherwise raise for outer complete cleanup. */
static int construct(void *argument)
{
    PointerTransaction *state = argument;
    const char *problem;
    DVM_CellResult claim;
    unsigned int attempt;
    int result;
    problem = dvm_runtime_heap_hold_attempt(state->source_cell, NULL, 1, &state->source, &claim);
    if (problem)
    {
        if (claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(ICONSP))
        {
            return 1;
        }
        error((char *)problem);
    }
    for (attempt = 0; attempt < DVM_POINTER_ATTEMPTS; attempt++)
    {
        problem = dvm_runtime_heap_hold_attempt(state->destination, &Tlist, 1, &state->tail, &claim);
        if (problem)
        {
            if (!attempt && claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(ICONSP))
            {
                return 1;
            }
            error((char *)problem);
        }
        dvm_runtime_pointer_prepare(&state->source, &state->tail, &state->candidate);
        result = dvm_runtime_heap_commit(state->destination, &state->tail,
            &state->candidate, &state->retired);
        if (result == 1)
        {
            return 1;
        }
        if (result < 0)
        {
            error(exHeap);
        }
        dvm_runtime_heap_finish(&state->candidate, 1);
        dvm_runtime_heap_finish(&state->tail, 1);
    }
    error("pointer constructor destination changed repeatedly");
    return 0;
}


/* [PERF CONSP-114] Capture reservations, candidate construction and displaced
 * retirement add work to the guarded profile; see PERFORMANCE-COSTS.md.
 * [GC:] Borrow rooted stable operand cells; use an explicit native ledger rather
 * than REG after callbacks. No admission release or new concurrent GC permission. */
void dvm_runtime_pointer_transaction(void *source, void *destination)
{
    PointerTransaction state = {0};
    int completed;
    state.source_cell = source;
    state.destination = destination;
    completed = kprotect(construct, &state);
    finish_all(&state, completed);
}
