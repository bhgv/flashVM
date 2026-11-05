/* Mapped replacement separates guarded publication from rooted retirement. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "isa.h"
#include "raise.h"
#include "shared_mapped.h"
#include "shared_access.h"
#include "shared_bytes.h"
#include "byte_transfer.h"
#include "managed_owner.h"
#include "root_batch.h"
#include "runtime_gc.h"
#include "heap_hold.h"
#include "shared_arithmetic.h"
#include "disvm/native_owner.h"
#include "gc_fields.h"
#include "disvm/refcount.h"
#include "disvm/native_process.h"
#include <stdint.h>
#include <stdlib.h>


int dvm_runtime_parallel_slicela;
int dvm_runtime_parallel_movmp;
int dvm_runtime_parallel_channel_buffer;
unsigned long dvm_runtime_parallel_channel_sends;
unsigned long dvm_runtime_parallel_channel_refills;
unsigned long dvm_runtime_parallel_channel_receives;
unsigned long dvm_runtime_parallel_movmp_commits;
unsigned long dvm_runtime_parallel_slicela_commits;

typedef struct SliceCopy
{
    Array *source;
    Array *destination;
    Array source_shape;
    Array destination_shape;
    const void *from;
    void *to;
    Type *type;
    size_t count;
} SliceCopy;

/* Check the borrowed geometry again after the exclusive-to-shared handoff. */
static int same_shape(const Array *array, const Array *shape)
{
    return array->len == shape->len && array->t == shape->t &&
        array->root == shape->root && array->data == shape->data;
}

/* One native map occurrence, in historical freeptrs byte/reverse-bit order. */
typedef struct MappedSlot
{
    size_t offset; /* Checked native pointer-cell offset within the record. */
    void *incoming; /* Borrowed under source guards, then retained for destination. */
    void *outgoing; /* Owner transferred from destination, rooted until retirement. */
    size_t root; /* Original occurrence index in displaced-owner root group. */
    int retained; /* This transaction's extra strong count, rollback only. */
} MappedSlot;


/* All native allocations finish before any cell claim or ownership effect. */
typedef struct MappedCopy
{
    MappedSlot *slots; /* Owned native map/snapshot metadata. */
    void **keys; /* Owned native root-key vector, not managed owners. */
    unsigned char *snapshot; /* Owned complete incoming byte image. */
    DVM_RootBatch *roots; /* Owned preallocated displaced-owner occurrences. */
    size_t count; /* Checked mapped pointer occurrence count. */
    size_t key_count; /* Non-H outgoing root occurrences. */
} MappedCopy;


/* Dispose owned native fields in copy after its group is inactive. Return void;
 * no callback/managed release. A live group is fatal internal misuse. */
static void dispose(MappedCopy *copy)
{
    if (copy->roots && !dvm_runtime_root_batch_destroy(&copy->roots))
    {
        abort();
    }
    free(copy->snapshot);
    free(copy->keys);
    free(copy->slots);
}


/* Return 1 for a sane H/live value, 0 for refused geometry/count. No retain or
 * ownership is acquired. Caller supplies actual independently live owner
 * and mapped header; bare addresses cannot acquire lifetime through this check. */
static int referent(void *value)
{
    unsigned long count;
    if (value == H)
    {
        return 1;
    }
    if ((uintptr_t)value <= sizeof(Heap) || (uintptr_t)value % sizeof(void *))
    {
        return 0;
    }
    count = dvm_refcount_load_u(&D2H(value)->ref);
    return count && count != (unsigned long)-1;
}


/* Undo copy's recorded extra retains; copy stays native owner-local. Return void.
 * Source slots remain guarded and unchanged.
 * Their original owners guarantee these drops cannot retire an object. Mark
 * barriers already performed remain conservative; no color rollback is claimed. */
static void rollback(MappedCopy *copy)
{
    size_t index;
    for (index = copy->count; index > 0; index--)
    {
        MappedSlot *slot = &copy->slots[index - 1];
        if (slot->retained)
        {
            if (dvm_refcount_release_u(&D2H(slot->incoming)->ref) != DVM_REFCOUNT_LIVE)
            {
                abort();
            }
            slot->retained = 0;
        }
    }
}


/* copy owns reserved metadata/snapshot; source/destination are mapped size-byte
 * spans whose full guard union caller holds. Return NULL after publication or a
 * borrowed static error with new retains undone and destination unchanged.
 * All validation
 * precedes retains, then roots precede the single destination publication. */
static const char *publish(MappedCopy *copy, const void *source, void *destination, size_t size, int consume)
{
    size_t index;
    memmove(copy->snapshot, source, size);
    for (index = 0; index < copy->count; index++)
    {
        MappedSlot *slot = &copy->slots[index];
        memmove(&slot->incoming, copy->snapshot + slot->offset, sizeof(void *));
        memmove(&slot->outgoing, (unsigned char *)destination + slot->offset, sizeof(void *));
        if (!referent(slot->incoming) || !referent(slot->outgoing))
        {
            return exHeap;
        }
        if (slot->outgoing != H)
        {
            slot->root = copy->key_count;
            copy->keys[copy->key_count++] = D2H(slot->outgoing);
        }
    }
    for (index = 0; index < copy->count; index++)
    {
        MappedSlot *slot = &copy->slots[index];
        if (slot->incoming != H)
        {
            Heap *heap = D2H(slot->incoming);
            if (!consume)
            {
                if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)
                {
                    rollback(copy);
                    return exHeap;
                }
                slot->retained = 1;
            }
            if (!dvm_gc_field_mark(&heap->color, &mutator, &nprop))
            {
                rollback(copy);
                return exHeap;
            }
        }
    }
    if (!dvm_runtime_root_batch_publish(copy->roots, copy->keys, copy->key_count))
    {
        rollback(copy);
        return exHeap;
    }
    dvm_runtime_byte_transfer(destination, copy->snapshot, size);
    if (consume)
    {
        void *empty = H;
        for (index = 0; index < copy->count; index++)
        {
            memmove((unsigned char *)source + copy->slots[index].offset, &empty, sizeof(empty));
        }
    }
    return NULL;
}


/* Check both retained array descriptors before using their captured spans. */
static int slice_shape_stable(const SliceCopy *slice)
{
    return !slice || (same_shape(slice->source, &slice->source_shape) &&
        same_shape(slice->destination, &slice->destination_shape));
}

static const char *publish_and_unlock(MappedCopy *copy, const void *source,
    void *destination, size_t bytes, int consume, DVM_RangeAccess *access)
{
    const char *problem = NULL;
    if (copy->count)
    {
        problem = publish(copy, source, destination, bytes, consume);
    }
    else
    {
        dvm_runtime_byte_transfer(destination, source, bytes);
    }
    if (!dvm_range_access_end(access))
    {
        abort();
    }
    return problem;
}


/* Consume value's displaced owner through the retained destructor, without
 * guards. Return 1 normally. A genuine VM
 * error is caught by the outer kprotect; this function never retries an owner. */
static int retire(void *value)
{
    destroy(value);
    return 1;
}


/* [PERF MAPPED-091] Five native preparation blocks for mapped references.
 * Whole-profile timing and reuse/locality investigation: PERFORMANCE-COSTS.md. */


/* [GC:] Descriptor/lifetime checks are independently provided by the opcode.
 * No payload/Type read happens without existing ACTIVE world admission. Native
 * buffers root nothing until publish; no VM/error callback spans range guards. */
static int mapped_transfer(const void *source, void *destination, Type *type, int size,
    size_t elements, int consume, int *committed, int replay_opcode, const SliceCopy *slice, const Prog *refill_sender)
{
    MappedCopy copy = {0};
    DVM_RangeAccess access = {0};
    DVM_CellResult claim;
    const char *problem = NULL;
    size_t index;
    size_t element;
    size_t bytes;
    int byte;
    int bit;
    int failed = 0;
    int released = 0;
    int shared_channel;
    DVM_CellDomain *domain = NULL;
    DVM_HeapOwner *owner = NULL;
    char first_error[ERRMAX];
    if (dvm_runtime_shared_mode() != 2 || !type || type == H || size < 0 ||
        type->size != size || !elements || (size && elements > SIZE_MAX / (size_t)size) ||
        type->np < 0 || type->np > size / (8 * sizeof(void *)) +
        (size % (8 * sizeof(void *)) != 0))
    {
        error(exHeap);
    }
    bytes = elements * (size_t)size;
    if (consume)
    {
        uintptr_t from = (uintptr_t)source;
        uintptr_t to = (uintptr_t)destination;
        if ((from < to ? to-from : from-to) < bytes)
        {
            error(exHeap);
        }
    }
    for (byte = 0; byte < type->np; byte++)
    {
        for (bit = 0; bit < 8; bit++)
        {
            if (type->map[byte] & (1U << bit))
            {
                if ((size_t)byte * 8 + 7 - bit >= (size_t)size / sizeof(void *))
                {
                    error(exHeap);
                }
                copy.count++;
            }
        }
    }
    if (!copy.count && !slice)
    {
        problem = replay_opcode ?
            dvm_runtime_bytes_copy_opcode(replay_opcode, source, destination, bytes) :
            dvm_runtime_bytes_copy(source, destination, bytes);
        if (problem)
        {
            error((char *)problem);
        }
        if (replay_opcode && dvm_native_execution()->arithmetic_retry_opcode == -1)
        {
            return 0;
        }
        if (committed)
        {
            *committed = 1;
        }
        return 1;
    }
    if (copy.count)
    {
        if (elements > SIZE_MAX / sizeof(*copy.slots) / copy.count)
        {
            error(exNomem);
        }
        copy.count *= elements;
        copy.slots = calloc(copy.count, sizeof(*copy.slots));
        copy.keys = calloc(copy.count, sizeof(*copy.keys));
        copy.snapshot = malloc(bytes);
        copy.roots = dvm_runtime_root_batch_create(copy.count);
        if (!copy.slots || !copy.keys || !copy.snapshot || !copy.roots)
        {
            dispose(&copy);
            error(exNomem);
        }
        index = 0;
        for (element = 0; element < elements; element++)
        {
            /* Keep the legacy SLICELA retirement direction for overlapping views. */
            size_t offset = ((uintptr_t)destination > (uintptr_t)source ?
                elements - element - 1 : element) * (size_t)size;
            for (byte = 0; byte < type->np; byte++)
            {
                for (bit = 0; bit < 8; bit++)
                {
                    if (type->map[byte] & (1U << bit))
                    {
                        copy.slots[index++].offset = offset + ((size_t)byte * 8 + 7 - bit) * sizeof(void *);
                    }
                }
            }
        }
    }
    /* The buffered caller retains the channel and reserves its slot until
     * exclusive reentry. Only a decoded direct operation owns the other
     * record in its suspended frame. A refill instead supplies a pinned sender. */
    shared_channel = dvm_runtime_parallel_channel_buffer && committed && up->prog &&
        ((refill_sender && !consume &&
          dvm_runtime_frame_record_for(&refill_sender->R, source, type)) ||
         (dvm_native_execution()->arithmetic_retry_opcode == replay_opcode &&
          ((replay_opcode == ISEND && !consume && dvm_runtime_frame_record(source, type)) ||
           (replay_opcode == IRECV && consume && dvm_runtime_frame_record(destination, type)))));
    if (slice || shared_channel || (dvm_runtime_parallel_movmp && !consume && replay_opcode == IMOVMP &&
        up->prog && dvm_native_execution()->arithmetic_retry_opcode == IMOVMP &&
        dvm_runtime_frame_record(source, type) && dvm_runtime_frame_record(destination, type)))
    {
        domain = dvm_managed_cell_domain();
        if (domain)
        {
            owner = dvm_native_owner_current();
            released = dvm_runtime_gc_native_try_leave();
        }
    }
    if (released > 0)
    {
        if (!dvm_native_owner_enter_shared(owner))
        {
            abort();
        }
        claim = slice_shape_stable(slice) ?
            dvm_range_access_begin(domain, source, bytes, destination, bytes, &access) :
            DVM_CELL_BUSY;
    }
    else
    {
        claim = dvm_managed_ranges_begin(source, bytes, destination, bytes, &access);
    }
    if (claim == DVM_CELL_ACQUIRED)
    {
        problem = publish_and_unlock(&copy, source, destination, bytes, consume, &access);
    }
    if (released > 0)
    {
        if (!dvm_native_owner_leave_shared(owner))
        {
            abort();
        }
        dvm_runtime_gc_native_enter(released);
        if (claim == DVM_CELL_ACQUIRED && !problem)
        {
            if (slice)
            {
                dvm_runtime_parallel_slicela_commits++;
            }
            else if (shared_channel)
            {
                if (refill_sender)
                    dvm_runtime_parallel_channel_refills++;
                else if (consume)
                    dvm_runtime_parallel_channel_receives++;
                else
                    dvm_runtime_parallel_channel_sends++;
            }
            else
            {
                dvm_runtime_parallel_movmp_commits++;
            }
        }
    }
    if (claim == DVM_CELL_BUSY && replay_opcode && dvm_runtime_scalar_retry(replay_opcode))
    {
        dispose(&copy);
        return 0;
    }
    /* Exhausting the shared retry budget must not turn contention into a
     * guest error. Exclusive admission permits one attempt with the same
     * rooted operands, provided their captured geometry still matches. */
    if (released > 0 && claim == DVM_CELL_BUSY && slice_shape_stable(slice))
    {
        claim = dvm_managed_ranges_begin(source, bytes, destination, bytes, &access);
        if (claim == DVM_CELL_ACQUIRED)
        {
            problem = publish_and_unlock(&copy, source, destination, bytes, consume, &access);
        }
    }
    if (claim != DVM_CELL_ACQUIRED)
    {
        dispose(&copy);
        error(exHeap);
    }
    if (problem)
    {
        dispose(&copy);
        error((char *)problem);
    }
    if (committed)
    {
        *committed = 1;
    }
    for (index = 0; index < copy.count; index++)
    {
        MappedSlot *slot = &copy.slots[index];
        if (slot->outgoing != H)
        {
            if (!dvm_runtime_root_batch_remove(copy.roots, slot->root))
            {
                abort();
            }
            if (!kprotect(retire, slot->outgoing) && !failed)
            {
                failed = 1;
                memmove(first_error, getup()->env->errstr, sizeof(first_error));
            }
        }
    }
    if (copy.roots && !dvm_runtime_root_batch_clear(copy.roots))
    {
        abort();
    }
    dispose(&copy);
    if (failed)
    {
        memmove(getup()->env->errstr, first_error, sizeof(first_error));
        krethrow();
    }
    return 1;
}


/* Keep the source ownership and retain each incoming destination owner. */
void dvm_runtime_mapped_copy(const void *source, void *destination, Type *type, int size)
{
    mapped_transfer(source, destination, type, size, 1, 0, NULL, 0, NULL, NULL);
}


/* Execute decoded MOVMP with replay allowed only before ownership changes. */
void dvm_runtime_mapped_move_opcode(const void *source, void *destination, Type *type, int size)
{
    mapped_transfer(source, destination, type, size, 1, 0, NULL, IMOVMP, NULL, NULL);
}


/* Transfer source ownership and report commitment even if retirement raises. */
void dvm_runtime_mapped_consume(void *source, void *destination, Type *type, int size,
    int *consumed)
{
    *consumed = 0;
    mapped_transfer(source, destination, type, size, 1, 1, consumed, 0, NULL, NULL);
}


/* The wrapper owns both array pins across publication and error unwinding. */
static int copy_pinned_slice(void *argument)
{
    SliceCopy *slice = argument;
    mapped_transfer(slice->from, slice->to, slice->type, slice->type->size,
        slice->count, 0, NULL, ISLICELA, slice, NULL);
    return 1;
}

static int finish_slice_pin(void *argument)
{
    dvm_runtime_heap_finish(argument, 1);
    return 1;
}

void dvm_runtime_array_slice_copy(const void *source, void *destination, Type *type, size_t count,
    Array *source_array, Array *destination_array)
{
    DVM_HeapHold from = {0}, to = {0};
    const char *problem;
    int completed, destination_released;
    SliceCopy slice;
    char first_error[ERRMAX];

    /* A retained view keeps its backing array live through its root link.
     * Stable decoded frame slots supply the array identities and index. */
    if (!dvm_runtime_parallel_slicela ||
        !dvm_runtime_frame_pointer(DVM_R.s) || !dvm_runtime_frame_pointer(DVM_R.d) ||
        !dvm_runtime_slice_index_stable(DVM_R.m))
    {
        mapped_transfer(source, destination, type, type->size, count, 0, NULL, ISLICELA, NULL, NULL);
        return;
    }
    slice = (SliceCopy){source_array, destination_array, *source_array, *destination_array,
        source, destination, type, count};
    problem = dvm_runtime_heap_hold(&source_array, &Tarray, 0, &from);
    if (problem)
    {
        error((char *)problem);
    }
    problem = dvm_runtime_heap_hold(&destination_array, &Tarray, 0, &to);
    if (problem)
    {
        kprotect(finish_slice_pin, &from);
        error((char *)problem);
    }
    completed = kprotect(copy_pinned_slice, &slice);
    if (!completed)
    {
        memmove(first_error, getup()->env->errstr, sizeof(first_error));
    }
    destination_released = kprotect(finish_slice_pin, &to);
    if (!completed)
    {
        memmove(getup()->env->errstr, first_error, sizeof(first_error));
    }
    dvm_runtime_heap_finish(&from, completed && destination_released);
}


/* Report publication before retiring displaced owners, for either direction. */
int dvm_runtime_mapped_buffer_transfer(const void *source, void *destination, Type *type,
    int consume, int *committed, int replay_opcode, const Prog *refill_sender)
{
    *committed = 0;
    return mapped_transfer(source, destination, type, type->size, 1, consume, committed,
        replay_opcode, NULL, refill_sender);
}
