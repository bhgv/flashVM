/* Copyright (c) 2026 bhgv
 * EN: Allocate rooted records, arrays and channels before monitored publication.
 * RU: Выделение укоренённых записей, массивов и каналов до публикации.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "pool.h"
#include "raise.h"
#include "record_new.h"
#include "allocation_publication.h"
#include "heap_pool.h"
#include "runtime_gc.h"
#include "record_storage.h"
#include "gc_fields.h"
#include "thread_port.h"
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
    void (*mover)(void); /* Non-null for channels. */
    WORD message_size; /* Raw mover geometry. */
    Heap *private_array; /* Buffer before the channel owns a registered root. */
    int channel_retained; /* Private Tchannel count. */
    int mover_retained; /* Private mapped mover descriptor count. */
    int array_retained; /* Extra Tarray count before candidate handoff. */
    void *destination; /* Borrowed rooted owning slot. */
    int clear; /* NEWZ initializes scalar bytes, NEW leaves them unspecified. */
    Heap *unpublished; /* Private block before root publication. */
    DVM_RootBatch *roots; /* Inactive reservation until publication succeeds. */
    int retained; /* Extra descriptor count before candidate handoff. */
    DVM_HeapHold tail; /* Observed destination owner. */
    DVM_HeapHold candidate; /* Complete rooted new record. */
    void *result; /* Borrowed identity of the committed object. */
    DVM_HeapHold retired; /* Displaced destination owner. */
} AllocationTransaction;


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


/* [GC:] Retain the descriptor before reserving private managed storage. */
static void retain_type(Type *type, int *retained)
{
    if (dvm_refcount_retain_i(&type->ref) != DVM_REFCOUNT_LIVE)
    {
        error(exHeap);
    }
    *retained = 1;
}


/* [GC:] Allocate a private header under NativeWorld. The caller records it
 * before any further fallible operation. */
static Heap *private_heap(Type *type, size_t extent)
{
    Heap *header = poolalloc(heapmem, sizeof(Heap)+extent);
    if (!header)
    {
        error(exHeap);
    }
    header->t = type;
    header->ref = 1;
    dvm_runtime_gc_store(&header->color, dvm_runtime_gc_load(&mutator));
    return header;
}


/* [GC:] Initialize an owning array. No callbacks run during initialization. */
static Array *build_array(AllocationTransaction *state, Heap **slot)
{
    size_t payload = (size_t)state->type->size * (size_t)state->count;
    Array *array;
    retain_type(state->type, &state->retained);
    retain_type(&Tarray, &state->array_retained);
    *slot = private_heap(&Tarray, sizeof(Array)+payload);
    array = H2D(Array *, *slot);
    dvm_record_initialize_array(array, state->type, state->count, state->clear);
    Setmark(*slot);
    return array;
}


/* [GC:] Build queues and any owning buffer before exposing the channel to GC
 * or a monitor. Partial queues and descriptor retains stay in the ledger. */
static void build_channel(AllocationTransaction *state)
{
    Channel *channel;
    retain_type(&Tchannel, &state->channel_retained);
    state->unpublished = private_heap(&Tchannel, sizeof(Channel));
    channel = H2D(Channel *, state->unpublished);
    memset(channel, 0, sizeof(*channel));
    channel->buf = H;
    channel->send = mallocz(sizeof(Progq), 1);
    if (!channel->send)
    {
        error(exNomem);
    }
    channel->recv = mallocz(sizeof(Progq), 1);
    if (!channel->recv)
    {
        error(exNomem);
    }
    if (state->count)
    {
        channel->buf = build_array(state, &state->private_array);
    }
    channel->mid.w = state->message_size;
    if (state->mover == movtmp)
    {
        retain_type(state->type, &state->mover_retained);
        channel->mid.t = state->type;
    }
    channel->mover = state->mover;
}


/* [GC:] Initialize a private record or array without callbacks.
 * RU: Инициализировать приватную запись или массив без обратных вызовов. */
static void initialize_candidate(AllocationTransaction *state)
{
    Heap *header = state->unpublished;
    if (state->array)
    {
        dvm_record_initialize_array(H2D(Array *, header), state->type, state->count, state->clear);
    }
    else
    {
        dvm_record_initialize_payload(state->type, state->clear, H2D(void *, header), (size_t)state->type->size, 1);
    }
}


/* [GC:] Try private construction and publish its temporary root in SHARED.
 * RU: Попробовать собрать приватный объект и опубликовать корень в SHARED. */
static int allocate_shared_candidate(AllocationTransaction *state, Type *header_type, size_t extent)
{
    if (dvm_runtime_gc_shared_try_enter() <= 0)
    {
        return 0;
    }
    Heap *header = dvm_heap_pool_try_alloc(heapmem, sizeof(Heap) + extent);
    int rooted = 0;
    if (header)
    {
        header->t = header_type;
        header->ref = 1;
        dvm_runtime_gc_store(&header->color, dvm_runtime_gc_load(&mutator));
        state->unpublished = header;
        initialize_candidate(state);
        Setmark(header);
        void *key = header;
        rooted = dvm_runtime_root_batch_publish(state->roots, &key, 1);
        if (!rooted)
        {
            poolfree(heapmem, header);
            state->unpublished = NULL;
            rooted = -1;
        }
    }
    dvm_runtime_gc_shared_finish();
    return rooted;
}


/* [GC:] Hand the complete candidate to the rooted ledger before invoking any
 * allocation monitor, including the monitor for a channel's owned buffer. */
static void allocate_candidate(AllocationTransaction *state)
{
    void *object;
    void *key;
    Heap *buffer;
    size_t extent;
    int rooted = 0;
    state->roots = dvm_runtime_root_batch_create(1);
    if (!state->roots)
    {
        error(exNomem);
    }
    if (state->mover)
    {
        build_channel(state);
        extent = sizeof(Channel);
    }
    else
    {
        Type *header_type = state->array ? &Tarray : state->type;
        extent = state->array ? sizeof(Array) +
            (size_t)state->type->size * (size_t)state->count : (size_t)state->type->size;
        retain_type(state->type, &state->retained);
        if (state->array)
        {
            retain_type(&Tarray, &state->array_retained);
        }
        rooted = allocate_shared_candidate(state, header_type, extent);
        if (rooted < 0)
        {
            error(exHeap);
        }
        if (!rooted)
        {
            state->unpublished = private_heap(header_type, extent);
            initialize_candidate(state);
        }
    }
    object = H2D(void *, state->unpublished);
    key = state->unpublished;
    if (!rooted)
    {
        Setmark(state->unpublished);
        if (!dvm_runtime_root_batch_publish(state->roots, &key, 1))
        {
            error(exHeap);
        }
    }
    buffer = state->private_array;
    state->private_array = NULL;
    state->mover_retained = 0;
    state->channel_retained = 0;
    state->candidate.value = object;
    state->candidate.roots = state->roots;
    state->unpublished = NULL;
    state->roots = NULL;
    state->retained = 0;
    state->array_retained = 0;
    if (heapmonitor)
    {
        heapmonitor(0, key, extent);
        if (buffer)
        {
            heapmonitor(0, buffer, sizeof(Array)+(size_t)state->type->size*(size_t)state->count);
        }
    }
}


/* [GC:] Build once, then compare and publish. A fresh allocation has no dependency
 * on the observed destination, so a stale comparison can reuse the candidate. */
static int construct(void *argument)
{
    AllocationTransaction *state = argument;
    const char *problem;
    int result;
    problem = dvm_allocation_hold_destination(state->destination, &state->tail);
    if (problem)
    {
        error((char *)problem);
    }
    allocate_candidate(state);
    for (;;)
    {
        void *candidate = state->candidate.value;
        result = dvm_allocation_commit_destination(state->destination, &state->tail,
            &state->candidate, &state->retired);
        if (result == 1)
        {
            state->result = candidate;
            return 1;
        }
        if (result < 0)
        {
            error("allocation publication refused");
        }
        dvm_runtime_heap_finish(&state->tail, 1);
        if (!dvm_native_context_poll())
        {
            error("allocation publication GC poll failed");
        }
        fvm_port_thread_sleep_us(50);
        problem = dvm_allocation_hold_destination(state->destination, &state->tail);
        if (problem)
        {
            error((char *)problem);
        }
    }
}


/* [GC:] Drop a private retain while the constructor still owns its input Type. */
static void drop_private_type(Type *type, int retained)
{
    if (retained && dvm_refcount_release_i(&type->ref) != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
}


/* [GC:] Dispose pre-publication resources before draining live leases. */
static void publish(AllocationTransaction *state)
{
    int completed = kprotect(construct, state);
    if (state->unpublished)
    {
        if (state->mover)
        {
            Channel *channel = H2D(Channel *, state->unpublished);
            free(channel->send);
            free(channel->recv);
        }
        poolfree(heapmem, state->unpublished);
    }
    if (state->private_array)
    {
        poolfree(heapmem, state->private_array);
    }
    drop_private_type(state->type, state->mover_retained);
    drop_private_type(state->type, state->retained);
    drop_private_type(&Tarray, state->array_retained);
    drop_private_type(&Tchannel, state->channel_retained);
    if (state->roots && !dvm_runtime_root_batch_destroy(&state->roots))
    {
        abort();
    }
    finish_all(state, completed);
}


/* [GC:] Check the native extent and aliasing descriptor retains before use. */
static void check_array(Type *type, WORD count, int extra_refs)
{
    int header_refs = dvm_refcount_load_i(&Tarray.ref);
    int element_refs = dvm_refcount_load_i(&type->ref);
    if (header_refs <= 0 || header_refs == INT_MAX ||
        element_refs > INT_MAX - 1 - (type == &Tarray) - extra_refs)
    {
        error(exHeap);
    }
    acheck(type->size, count);
}


static void allocate(void *destination, Type *type, int clear, int array, WORD count)
{
    AllocationTransaction state = {0};
    if (!dvm_record_valid_type(type) || !dvm_record_span(destination, sizeof(void *)) || destination == H)
    {
        error(exHeap);
    }
    if (array)
    {
        check_array(type, count, 0);
    }
    state.destination = destination;
    state.type = type;
    state.clear = clear;
    state.array = array;
    state.count = count;
    publish(&state);
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


/* [GC:] Construct a complete channel, optionally with an owning buffer.
 * Raw buffered callers keep their temporary descriptor alive through cleanup. */
Channel *dvm_runtime_channel_new(void *destination, Type *type, WORD count,
    void (*mover)(void), WORD message_size)
{
    AllocationTransaction state = {0};
    if (count < 0)
    {
        error(exNegsize);
    }
    if (count > INT_MAX || !mover ||
        !dvm_record_span(destination, sizeof(void *)) || destination == H)
    {
        error(exHeap);
    }
    if ((count || mover == movtmp) && !dvm_record_valid_type(type))
    {
        error(exHeap);
    }
    if (count)
    {
        check_array(type, count, (mover == movtmp) + (type == &Tchannel));
    }
    state.destination = destination;
    state.type = type;
    state.count = count;
    state.mover = mover;
    state.message_size = message_size;
    publish(&state);
    return state.result;
}


/* [GC:] Keep the synthetic payload descriptor live through all cleanup. */
static int publish_raw_channel(void *argument)
{
    AllocationTransaction *state = argument;
    dvm_runtime_channel_new(state->destination, state->type, state->count,
        state->mover, state->message_size);
    return 1;
}


/* [GC:] An unbuffered raw channel needs only the message width. */
void dvm_runtime_raw_channel_new(void *destination, WORD count, WORD size,
    void (*mover)(void))
{
    AllocationTransaction state = {0};
    int completed;
    if (size < 0 || count < 0)
    {
        error(exNegsize);
    }
    if (size > INT_MAX || count > INT_MAX)
    {
        error(exHeap);
    }
    if (count)
    {
        acheck(size, count);
        state.type = dtype(NULL, size, NULL, 0);
        if (!state.type)
        {
            error(exNomem);
        }
    }
    state.destination = destination;
    state.count = count;
    state.message_size = size;
    state.mover = mover;
    completed = kprotect(publish_raw_channel, &state);
    freetype(state.type);
    if (!completed)
    {
        krethrow();
    }
}
