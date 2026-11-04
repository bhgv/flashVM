/* Compose capture, preparation, conditional publication and complete retirement. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "raise.h"
#include "scalar_cons.h"
#include "value_prepare.h"
#include "heap_commit.h"
#include "shared_bytes.h"
#include "shared_access.h"
#include <string.h>

#define DVM_SCALAR_ATTEMPTS 8 /* Bound pathological callback-induced retry work. */


/* Stable native ledger spans the complete protected body and cleanup errors. */
typedef struct ScalarTransaction
{
    void *source_cell; /* Borrowed mapped operand, captured once before callbacks. */
    void *destination; /* Borrowed rooted destination, stable through all attempts. */
    uchar snapshot[24]; /* Private zero-padded scalar bytes, no managed references. */
    size_t width; /* Actual operand width, one/four/eight native bytes. */
    size_t extent; /* Allocated head extent, including byte padding. */
    DVM_HeapHold tail; /* Exclusive observation for the current publication attempt. */
    DVM_HeapHold candidate; /* Exclusive unpublished complete list-node owner. */
    DVM_HeapHold retired; /* Displaced destination owner after successful commit. */
} ScalarTransaction;


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
static void finish_all(ScalarTransaction *state, int body_ok)
{
    DVM_HeapHold *holds[] = {&state->candidate, &state->retired, &state->tail};
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


/* [GC:] Capture raw source bytes once before any callback, including overlapping cells.
 * state owns stable mapped cells and zero ledger. Each stale candidate is fully
 * discarded before refreshing tail; never publish a node carrying the old tail.
 * Return1 on committed publication, otherwise raise for outer complete cleanup. */
static int construct(void *argument)
{
    ScalarTransaction *state = argument;
    const char *problem;
    unsigned int attempt;
    int result;
    problem = dvm_runtime_bytes_copy(state->source_cell, state->snapshot, state->width);
    if (problem)
    {
        error((char *)problem);
    }
    for (attempt = 0; attempt < DVM_SCALAR_ATTEMPTS; attempt++)
    {
        problem = dvm_runtime_heap_hold(state->destination, &Tlist, 1, &state->tail);
        if (problem)
        {
            error((char *)problem);
        }
        dvm_runtime_value_prepare(state->snapshot, state->extent, &state->tail, &state->candidate);
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
    error("scalar constructor destination changed repeatedly");
    return 0;
}


/* [PERF SCALARCONS-116] Capture reservations, candidate construction and displaced
 * retirement add work to the guarded profile; see PERFORMANCE-COSTS.md.
 * [GC:] Borrow rooted stable operand cells; use an explicit native ledger rather
 * than REG after callbacks. No admission release or new concurrent GC permission. */
int dvm_runtime_scalar_cons(void *source, void *destination, size_t width, size_t extent)
{
    ScalarTransaction state = {0};
    int completed;
    int selected;
    if (!dvm_runtime_guarded_copies)
    {
        return 0;
    }
    selected = dvm_runtime_shared_mode();
    if (!selected || (width != 1 && width != 4 && width != 8) ||
        extent < width || extent > sizeof(state.snapshot))
    {
        error(exHeap);
    }
    if (selected == 1)
    {
        return 0;
    }
    state.width = width;
    state.extent = extent;
    state.source_cell = source;
    state.destination = destination;
    completed = kprotect(construct, &state);
    finish_all(&state, completed);
    return 1;
}
