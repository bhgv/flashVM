/* Allocate a rooted record or array before exposing it to an allocation monitor. */
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
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern Pool *heapmem; /* Managed pool under NativeWorld. */


/* Stable native ownership ledger across constructor and cleanup errors. */
typedef struct AllocationTransaction
{
    Type *type; /* Live immutable record or element descriptor owned by caller. */
    WORD count; /* Array length after acheck, unused for records. */
    int array; /* Select the owning Array header. */
    int array_retained; /* Extra Tarray count before candidate handoff. */
    void *destination; /* Borrowed rooted owning slot. */
    int clear; /* NEWZ initializes scalar bytes, NEW leaves them unspecified. */
    Heap *unpublished; /* Private block before root publication. */
    DVM_RootBatch *roots; /* Inactive reservation until publication succeeds. */
    int retained; /* Extra descriptor count before candidate handoff. */
    DVM_HeapHold tail; /* Observed destination owner. */
    DVM_HeapHold candidate; /* Complete rooted new record. */
    DVM_HeapHold retired; /* Displaced destination owner. */
} AllocationTransaction;


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
static void finish_all(AllocationTransaction *state, int body_ok)
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


/* [GC:] Initialize privately allocated bytes and every mapped slot before rooting.
 * count is already bounded, and a zero-sized element has an empty map. */
static void initialize_payload(AllocationTransaction *state, uchar *data, size_t bytes, WORD count)
{
    WORD index;
    if (state->clear && bytes)
    {
        memset(data, 0, bytes);
    }
    if (state->type->np)
    {
        for (index = 0; index < count; index++)
        {
            initmem(state->type, data);
            data += state->type->size;
        }
    }
}


/* [GC:] Build a complete candidate and hand its ownership to the rooted ledger
 * before invoking the monitor. Pre-root failures leave private resources in state. */
static void allocate_candidate(AllocationTransaction *state)
{
    void *object;
    void *key;
    size_t payload = (size_t)state->type->size;
    size_t extent;
    if (state->array)
    {
        payload *= (size_t)state->count;
    }
    extent = payload + (state->array ? sizeof(Array) : 0);
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
    if (state->array)
    {
        if (dvm_refcount_retain_i(&Tarray.ref) != DVM_REFCOUNT_LIVE)
        {
            error(exHeap);
        }
        state->array_retained = 1;
    }
    state->unpublished = poolalloc(heapmem, sizeof(Heap)+extent);
    if (!state->unpublished)
    {
        error(exHeap);
    }
    state->unpublished->t = state->array ? &Tarray : state->type;
    state->unpublished->ref = 1;
    dvm_runtime_gc_store(&state->unpublished->color, dvm_runtime_gc_load(&mutator));
    object = H2D(void *, state->unpublished);
    if (state->array)
    {
        Array *array = object;
        array->len = state->count;
        array->t = state->type;
        array->root = H;
        array->data = (uchar *)array + sizeof(*array);
        initialize_payload(state, array->data, payload, state->count);
    }
    else
    {
        initialize_payload(state, object, payload, 1);
    }
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
    state->array_retained = 0;
    if (heapmonitor)
    {
        heapmonitor(0, key, extent);
    }
}


/* [GC:] Build once, then compare and publish. A fresh allocation has no dependency
 * on the observed destination, so a stale comparison can reuse the candidate. */
static int construct(void *argument)
{
    AllocationTransaction *state = argument;
    const char *problem;
    unsigned int attempt;
    int result;
    problem = dvm_runtime_heap_hold(state->destination, NULL, 1, &state->tail);
    if (problem)
    {
        error((char *)problem);
    }
    allocate_candidate(state);
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
    error("allocation destination changed repeatedly");
    return 0;
}


/* [GC:] Dispose private pre-publication resources before draining live leases.
 * Caller keeps the descriptor, cell and frame alive under NativeWorld. */
static void allocate(void *destination, Type *type, int clear, int array, WORD count)
{
    AllocationTransaction state = {0};
    int completed;
    if (!valid_type(type) || !span(destination, sizeof(void *)) || destination == H)
    {
        error(exHeap);
    }
    if (array)
    {
        int header_refs = dvm_refcount_load_i(&Tarray.ref);
        int element_refs = dvm_refcount_load_i(&type->ref);
        if (header_refs <= 0 || header_refs == INT_MAX ||
            element_refs > INT_MAX - 1 - (type == &Tarray))
        {
            error(exHeap);
        }
        acheck(type->size, count);
    }
    state.destination = destination;
    state.type = type;
    state.clear = clear;
    state.array = array;
    state.count = count;
    completed = kprotect(construct, &state);
    if (state.unpublished)
    {
        poolfree(heapmem, state.unpublished);
    }
    if (state.retained && dvm_refcount_release_i(&type->ref) != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
    if (state.array_retained && dvm_refcount_release_i(&Tarray.ref) != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
    if (state.roots && !dvm_runtime_root_batch_destroy(&state.roots))
    {
        abort();
    }
    finish_all(&state, completed);
}

/* Construct one record in the rooted destination, with optional scalar zeroing. */
void dvm_runtime_record_new(void *destination, Type *type, int clear)
{
    allocate(destination, type, clear, 0, 0);
}


/* Construct count owning elements after checking the full native WORD extent. */
void dvm_runtime_array_new(void *destination, Type *type, WORD count, int clear)
{
    allocate(destination, type, clear, 1, count);
}
