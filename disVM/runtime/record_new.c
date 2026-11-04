/* Allocate a rooted record before exposing it to an allocation monitor. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "pool.h"
#include "raise.h"
#include "record_new.h"
#include "heap_commit.h"
#include "gc_fields.h"
#include "disvm/refcount.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern Pool *heapmem; /* Managed pool under NativeWorld. */


/* Stable native ownership ledger across constructor and cleanup errors. */
typedef struct RecordTransaction
{
    Type *type; /* Live immutable descriptor, independently owned by caller. */
    void *destination; /* Borrowed rooted owning slot. */
    int clear; /* NEWZ initializes scalar bytes, NEW leaves them unspecified. */
    Heap *unpublished; /* Private block before root publication. */
    DVM_RootBatch *roots; /* Inactive reservation until publication succeeds. */
    int retained; /* Extra descriptor count before candidate handoff. */
    DVM_HeapHold tail; /* Observed destination owner. */
    DVM_HeapHold candidate; /* Complete rooted new record. */
    DVM_HeapHold retired; /* Displaced destination owner. */
} RecordTransaction;


/* Return1 for nonwrapping native address/size geometry; never infer mapping. */
static int span(const void *address, size_t size)
{
    return (uintptr_t)address >= sizeof(void *) &&
        (uintptr_t)address <= (uintptr_t)-1-size;
}


/* [GC:] Validate immutable genuine Type shape before initmem/map traversal.
 * Return0 for malformed extent/map,1 valid; no allocation/retention/callback.
 * Caller supplies real descriptor lifetime and mapped storage, not just address. */
static int valid_type(Type *type)
{
    int byte;
    int bit;
    if (!span(type, offsetof(Type, map)) || type == H ||
        (uintptr_t)type % sizeof(void *) || type->size < 0 ||
        (ulong)type->size > 0x7fffffff-sizeof(Heap) ||
        dvm_refcount_load_i(&type->ref) <= 0 || type->np < 0 || type->np > type->size/(8*sizeof(void *))+
        (type->size%(8*sizeof(void *)) != 0) ||
        !span(type, offsetof(Type, map)+(size_t)type->np))
    {
        return 0;
    }
    for (byte = 0; byte < type->np; byte++)
    {
        for (bit = 0; bit < 8; bit++)
        {
            if ((type->map[byte] & (0x80U>>bit)) &&
                (size_t)byte*8+bit >= (size_t)type->size/sizeof(void *))
            {
                return 0;
            }
        }
    }
    return 1;
}


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
static void finish_all(RecordTransaction *state, int body_ok)
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


/* [GC:] Build once, then compare and publish. A fresh record has no dependency
 * on the observed destination, so a stale comparison can reuse the candidate. */
static int construct(void *argument)
{
    RecordTransaction *state = argument;
    void *object;
    void *key;
    const char *problem;
    unsigned int attempt;
    int result;
    problem = dvm_runtime_heap_hold(state->destination, NULL, 1, &state->tail);
    if (problem)
    {
        error((char *)problem);
    }
    state->roots = dvm_runtime_root_batch_create(1);
    if (!state->roots)
    {
        error(exNomem);
    }
    if (dvm_refcount_retain_i(&state->type->ref) != DVM_REFCOUNT_LIVE)
    {
        error(exHeap);
    }
    state->retained = 1;
    state->unpublished = poolalloc(heapmem, sizeof(Heap)+state->type->size);
    if (!state->unpublished)
    {
        error(exHeap);
    }
    state->unpublished->t = state->type;
    state->unpublished->ref = 1;
    dvm_runtime_gc_store(&state->unpublished->color, dvm_runtime_gc_load(&mutator));
    object = H2D(void *, state->unpublished);
    if (state->clear)
    {
        memset(object, 0, state->type->size);
    }
    initmem(state->type, object);
    Setmark(state->unpublished);
    key = state->unpublished;
    if (!dvm_runtime_root_batch_publish(state->roots, &key, 1))
    {
        error(exHeap);
    }
    state->candidate.value = object;
    state->candidate.roots = state->roots;
    state->unpublished = NULL;
    state->roots = NULL;
    state->retained = 0;
    if (heapmonitor)
    {
        heapmonitor(0, key, state->type->size);
    }
    for (attempt = 0; attempt < 8; attempt++)
    {
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
        dvm_runtime_heap_finish(&state->tail, 1);
        problem = dvm_runtime_heap_hold(state->destination, NULL, 1, &state->tail);
        if (problem)
        {
            error((char *)problem);
        }
    }
    error("record destination changed repeatedly");
    return 0;
}


/* [GC:] Dispose private pre-publication resources before draining live leases.
 * Caller keeps the descriptor, cell and frame alive under NativeWorld. */
void dvm_runtime_record_new(void *destination, Type *type, int clear)
{
    RecordTransaction state = {0};
    int completed;
    if (!valid_type(type) || !span(destination, sizeof(void *)) || destination == H)
    {
        error(exHeap);
    }
    state.destination = destination;
    state.type = type;
    state.clear = clear;
    completed = kprotect(construct, &state);
    if (state.unpublished)
    {
        poolfree(heapmem, state.unpublished);
    }
    if (state.retained && dvm_refcount_release_i(&type->ref) != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
    if (state.roots && !dvm_runtime_root_batch_destroy(&state.roots))
    {
        abort();
    }
    finish_all(&state, completed);
}
