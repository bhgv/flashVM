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
#include "gc_fields.h"
#include "disvm/refcount.h"
#include <stdint.h>
#include <stdlib.h>


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
static void mapped_transfer(const void *source, void *destination, Type *type, int size,
    size_t elements, int *consumed, int replay_opcode)
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
    char first_error[ERRMAX];
    if (dvm_runtime_shared_mode() != 2 || !type || type == H || size < 0 ||
        type->size != size || !elements || (size && elements > SIZE_MAX / (size_t)size) ||
        type->np < 0 || type->np > size / (8 * sizeof(void *)) +
        (size % (8 * sizeof(void *)) != 0))
    {
        error(exHeap);
    }
    bytes = elements * (size_t)size;
    if (consumed)
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
    if (!copy.count)
    {
        problem = replay_opcode ?
            dvm_runtime_bytes_copy_opcode(replay_opcode, source, destination, bytes) :
            dvm_runtime_bytes_copy(source, destination, bytes);
        if (problem)
        {
            error((char *)problem);
        }
        if (consumed)
        {
            *consumed = 1;
        }
        return;
    }
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
    claim = dvm_managed_ranges_begin(source, bytes, destination, bytes, &access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        dispose(&copy);
        if (claim == DVM_CELL_BUSY && replay_opcode && dvm_runtime_scalar_retry(replay_opcode))
        {
            return;
        }
        error(exHeap);
    }
    problem = publish(&copy, source, destination, bytes, consumed != NULL);
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    if (problem)
    {
        dispose(&copy);
        error((char *)problem);
    }
    if (consumed)
    {
        *consumed = 1;
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
    if (!dvm_runtime_root_batch_clear(copy.roots))
    {
        abort();
    }
    dispose(&copy);
    if (failed)
    {
        memmove(getup()->env->errstr, first_error, sizeof(first_error));
        krethrow();
    }
}


/* Keep the source ownership and retain each incoming destination owner. */
void dvm_runtime_mapped_copy(const void *source, void *destination, Type *type, int size)
{
    mapped_transfer(source, destination, type, size, 1, NULL, 0);
}


/* Execute decoded MOVMP with replay allowed only before ownership changes. */
void dvm_runtime_mapped_move_opcode(const void *source, void *destination, Type *type, int size)
{
    mapped_transfer(source, destination, type, size, 1, NULL, IMOVMP);
}


/* Transfer source ownership and report commitment even if retirement raises. */
void dvm_runtime_mapped_consume(void *source, void *destination, Type *type, int size,
    int *consumed)
{
    *consumed = 0;
    mapped_transfer(source, destination, type, size, 1, consumed, 0);
}


/* [GC:] Copy count live array elements with the supplied descriptor. Their
 * array owners remain live under NativeWorld through preparation and commit. */
void dvm_runtime_array_slice_copy(const void *source, void *destination, Type *type, size_t count)
{
    mapped_transfer(source, destination, type, type->size, count, NULL, ISLICELA);
}
