/* Prepare complete traced list nodes before any observing VM callback. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "pool.h"
#include "raise.h"
#include "mapped_prepare.h"
#include "shared_mapped.h"
#include "shared_access.h"
#include "heap_snapshot.h"
#include "gc_fields.h"
#include "disvm/refcount.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern Pool *heapmem; /* Serialized retained allocator, not a parallel pool. */


/* Exclusive native ledger remains stable through protected construction errors. */
typedef struct MappedPreparation
{
    const void *source; /* Borrowed genuine owning source until guarded capture. */
    Type *type; /* Immutable element descriptor, caller retains original owner. */
    List *tail; /* Genuine held old tail or H, independent original owner stays. */
    Heap *node; /* Private unpublished creator owner before root handoff. */
    DVM_RootBatch *roots; /* Reserved native candidate-root metadata. */
    DVM_HeapHold published; /* Exclusive rooted complete node before monitor. */
    int tail_owner; /* Acquired extra tail count, linked before publication. */
    int list_owner; /* Acquired Tlist descriptor owner. */
    int type_owner; /* Acquired element descriptor owner, may also be Tlist. */
    int pinned; /* Ordinary construction root before candidate publication. */
    int copied; /* Entire mapped head was published by actual mapped-copy helper. */
} MappedPreparation;


/* Return1 for nonwrapping native address/size geometry; never infer mapping. */
static int span(const void *address, size_t size)
{
    return (uintptr_t)address >= sizeof(void *) &&
        (uintptr_t)address <= (uintptr_t)-1-size;
}


/* Return1 for overlap of already checked a/an and b/bn extents, no dereference. */
static int overlaps(const void *a, size_t an, const void *b, size_t bn)
{
    return (uintptr_t)a < (uintptr_t)b+bn && (uintptr_t)b < (uintptr_t)a+an;
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
        (ulong)type->size > 0x7fffffff-sizeof(Heap)-offsetof(List, data) ||
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


/* Require original descriptor owner to survive rollback; release is nonfinal. */
static void release_type(int *count)
{
    if (dvm_refcount_release_i(count) != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
}


/* Require genuine source/tail owner to survive pre-callback rollback. */
static void release_heap(void *value)
{
    if (value != H && dvm_refcount_release_u(&D2H(value)->ref) != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
}


/* [GC:] Only for errors BEFORE published-root handoff/monitor. Source/tail
 * originals remain untouched; all acquired drops are nonfinal. No destructor or
 * callback. state exclusively owns flags/node; return void or abort invariant. */
static void rollback(MappedPreparation *state)
{
    int byte;
    int bit;
    void *value;
    if (state->pinned)
    {
        ptrdel(state->node);
    }
    if (state->copied)
    {
        for (byte = 0; byte < state->type->np; byte++)
        {
            for (bit = 0; bit < 8; bit++)
            {
                if (state->type->map[byte] & (0x80U>>bit))
                {
                    memmove(&value, (uchar *)H2D(List *,state->node)->data+
                        ((size_t)byte*8+bit)*sizeof(value), sizeof(value));
                    release_heap(value);
                }
            }
        }
    }
    if (state->node)
    {
        if (dvm_refcount_release_u(&state->node->ref) != DVM_REFCOUNT_LAST)
        {
            abort();
        }
        poolfree(heapmem, state->node);
    }
    if (state->tail_owner)
    {
        release_heap(state->tail);
    }
    if (state->type_owner)
    {
        release_type(&state->type->ref);
    }
    if (state->list_owner)
    {
        release_type(&Tlist.ref);
    }
}


/* [GC:] state owns reserved root metadata and genuine immutable Type/source.
 * Actual mapped-copy into H-initialized slots either rolls back guarded retains
 * or succeeds with no outgoing destructor (all old fields H). This is required
 * for copied to precisely identify ownership. Publish complete graph before
 * monitor; post-handoff errors use consuming rooted cleanup, not raw rollback. */
static int prepare(void *argument)
{
    MappedPreparation *state = argument;
    List *node;
    Heap *header;
    void *key;
    ulong extent = offsetof(List, data)+state->type->size;
    if (state->tail != H)
    {
        if (dvm_refcount_retain_u(&D2H(state->tail)->ref) != DVM_REFCOUNT_LIVE)
        {
            error(exHeap);
        }
        state->tail_owner = 1;
    }
    if (dvm_refcount_retain_i(&Tlist.ref) != DVM_REFCOUNT_LIVE)
    {
        error(exHeap);
    }
    state->list_owner = 1;
    if (dvm_refcount_retain_i(&state->type->ref) != DVM_REFCOUNT_LIVE)
    {
        error(exHeap);
    }
    state->type_owner = 1;
    state->node = poolalloc(heapmem, sizeof(Heap)+extent);
    if (!state->node)
    {
        error(exHeap);
    }
    state->node->t = &Tlist;
    state->node->ref = 1;
    dvm_runtime_gc_store(&state->node->color, dvm_runtime_gc_load(&mutator));
    node = H2D(List *, state->node);
    node->tail = H;
    node->t = state->type;
    memset(node->data, 0, state->type->size);
    initmem(state->type, node->data);
    ptradd(state->node);
    state->pinned = 1;
    Setmark(state->node);
    dvm_runtime_mapped_copy(state->source, node->data, state->type, state->type->size);
    state->copied = 1;
    node->tail = state->tail;
    if (state->tail != H)
    {
        Setmark(D2H(state->tail));
    }
    key = state->node;
    if (!dvm_runtime_root_batch_publish(state->roots, &key, 1))
    {
        error(exHeap);
    }
    state->published.value = node;
    state->published.roots = state->roots;
    state->roots = NULL;
    header = state->node;
    ptrdel(state->node);
    state->node = NULL;
    state->pinned = 0;
    state->copied = 0;
    state->tail_owner = 0;
    state->type_owner = 0;
    state->list_owner = 0;
    if (heapmonitor)
    {
        heapmonitor(0, header, extent);
    }
    return 1;
}


/* [PERF MAPPEDPREP-118] Typed empty node plus actual mapped-copy metadata/capture
 * precede candidate-root handoff. Known paths, no isolated timing claimed.
 * [GC:] Rooted complete graph moves only after all fallible pre-callback stages.
 * A monitor error may follow application writes: retire via real held List owner,
 * preserving first error; pre-callback rollback only borrows original owners. */
void dvm_runtime_mapped_prepare(const void *source, Type *type,
    const DVM_HeapHold *tail, DVM_HeapHold *candidate)
{
    MappedPreparation state = {0};
    Heap observed;
    if (dvm_runtime_shared_mode() != 2 || !valid_type(type) ||
        !span(tail,sizeof(*tail)) || !span(candidate,sizeof(*candidate)) ||
        overlaps(tail,sizeof(*tail),candidate,sizeof(*candidate)) ||
        (type->size && (!span(source,(size_t)type->size) || source == H ||
         overlaps(source,(size_t)type->size,candidate,sizeof(*candidate)))))
    {
        error(exHeap);
    }
    if (!tail->value || !tail->roots || candidate->value || candidate->roots)
    {
        error(exHeap);
    }
    state.source = source;
    state.type = type;
    state.tail = tail->value;
    if (state.tail != H &&
        (!dvm_runtime_heap_snapshot(&observed,D2H(state.tail)) || observed.t != &Tlist))
    {
        error(exType);
    }
    state.roots = dvm_runtime_root_batch_create(1);
    if (!state.roots)
    {
        error(exNomem);
    }
    if (!kprotect(prepare,&state))
    {
        if (state.published.value)
        {
            dvm_runtime_heap_finish(&state.published,0);
        }
        rollback(&state);
        if (!dvm_runtime_root_batch_destroy(&state.roots))
        {
            abort();
        }
        krethrow();
    }
    candidate->value = state.published.value;
    candidate->roots = state.published.roots;
    state.published.value = NULL;
    state.published.roots = NULL;
}
