/* Copyright (c) 2026 bhgv
 * EN: Snapshot bounded ADTs, preserve aliases, and publish copy-back atomically.
 * RU: Снимки ограниченных ADT, сохранение общих ссылок и атомарная обратная запись.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "kernel.h"
#include "managed_ref.h"
#include "managed_owner.h"
#include "thread_port.h"
#include "disvm/refcount.h"
#include <stdlib.h>
#include <string.h>

typedef struct RefAllocation
{
    Type *type; /* Borrowed descriptor retained through allocation. */
    void *object; /* Owned receiver-local ADT. */
} RefAllocation;


/* EN: Allocate an initialized ADT inside the caller's error boundary.
 * RU: Выделить инициализированный ADT внутри обработчика ошибок. */
static int allocate_ref(void *opaque)
{
    RefAllocation *allocation = opaque;
    allocation->object = H2D(void *, heapz(allocation->type));
    return 1;
}


/* EN: Check the compiled layout and pointer map against an existing ADT.
 * RU: Сверить скомпилированную раскладку и карту ссылок существующего ADT. */
static int ref_layout(const Type *type, const StyxScalarLayout *layout)
{
    size_t size = (layout->size + 7) & ~(size_t)7;
    if (!type || type->size < (int)layout->size || type->size > (int)size)
    {
        return 0;
    }
    unsigned char map = 0;
    for (size_t i = 0; i < layout->count; i++)
    {
        if (layout->pointers & (1U << i))
        {
            map |= 0x80U >> (layout->offset[i] / sizeof(void *));
        }
    }
    return type->np == 0 ? !map : type->np == 1 && type->map[0] == map;
}


/* EN/RU: See managed_ref.h. / См. managed_ref.h. */
int dvm_styx_ref_read(DVM_StyxRefTable *table, const char *shape, const FVM_SRecord *node, void **result)
{
    if (node->tag == STYX_MANAGED_NIL_TAG && !node->size)
    {
        *result = H;
        return 1;
    }
    if (node->tag != STYX_MANAGED_REF_TAG || !node->size || node->payload[0] > table->count)
    {
        return 0;
    }
    unsigned index = node->payload[0];
    if (index < table->count)
    {
        void *value = table->objects[index];
        if (!value || value == H || node->size != 1 || dvm_refcount_retain_u(&D2H(value)->ref) != DVM_REFCOUNT_LIVE)
        {
            return 0;
        }
        *result = value;
        return 1;
    }
    char fields[STYX_SHAPE_MAX + 1];
    if (index >= (table->limit ? table->limit : STYX_CALL_LEGACY_REF_KEYS) || !(table->result ?
        styxmanaged_result_ref_fields(shape, fields) : styxmanaged_ref_fields(shape, fields)))
    {
        return 0;
    }
    DVM_StyxValueFrame frame = {0};
    int valid;
    if (table->result)
    {
        table->objects[table->count++] = NULL;
        valid = dvm_styx_result_fields_read(table, fields, node->payload + 1, node->size - 1, &frame);
    }
    else
    {
        size_t extent = node->size + 16, used;
        unsigned char *record = malloc(extent);
        valid = record && fvm_srecord_write(STYX_MANAGED_REF_TAG, node->payload + 1, node->size - 1,
            record, extent, &used) == FVM_S_OK &&
            dvm_styx_value_frame_read(STYX_MANAGED_REF_TAG, fields, record, used, &frame);
        free(record);
    }
    if (!valid)
    {
        return 0;
    }
    unsigned char map = 0;
    for (size_t i = 0; i < frame.layout.count; i++)
    {
        if (frame.layout.pointers & (1U << i))
        {
            map |= 0x80U >> (frame.layout.offset[i] / sizeof(void *));
        }
    }
    RefAllocation allocation = {.type = dtype(freeheap, (frame.layout.size + 7) & ~(size_t)7, &map, !!map)};
    int allocated = allocation.type && kprotect(allocate_ref, &allocation);
    freetype(allocation.type);
    if (!allocated)
    {
        dvm_styx_value_frame_clear(&frame);
        return 0;
    }
    memcpy(allocation.object, frame.words, frame.layout.size);
    for (size_t i = 0; i < frame.layout.count; i++)
    {
        if (frame.roots & (1U << i))
        {
            void *value;
            memcpy(&value, (unsigned char *)frame.words + frame.layout.offset[i], sizeof(value));
            ptrdel(D2H(value));
        }
    }
    if (!table->result)
    {
        table->count++;
    }
    table->objects[index] = allocation.object;
    *result = allocation.object;
    return 1;
}


typedef struct RefEncode
{
    DVM_StyxValueFrame *snapshot; /* Owned fields; released by caller on all paths. */
    const char *fields; /* Borrowed checked field schema. */
    unsigned char *record; /* Scratch for the field list. */
    size_t capacity; /* Scratch extent. */
    size_t size; /* Encoded field list extent. */
    DVM_StyxRefTable *table; /* Shared result references or NULL for a standalone snapshot. */
} RefEncode;


/* EN: Root retained snapshots across the field codec and its guarded reads.
 * RU: Укоренить удерживаемый снимок на время кодирования и чтения полей. */
static int encode_snapshot(void *opaque)
{
    RefEncode *state = opaque;
    DVM_StyxValueFrame *snapshot = state->snapshot;
    for (size_t i = 0; i < snapshot->layout.count; i++)
    {
        if (snapshot->layout.pointers & (1U << i))
        {
            void *value;
            memcpy(&value, (unsigned char *)snapshot->words + snapshot->layout.offset[i], sizeof(value));
            if (value != H)
            {
                ptradd(D2H(value));
                snapshot->roots |= 1U << i;
                Setmark(D2H(value));
            }
        }
    }
    if (state->table)
    {
        return dvm_styx_result_fields_write(state->table, state->fields, snapshot->words,
            snapshot->layout.size, state->record, state->capacity, &state->size);
    }
    return dvm_styx_value_frame_write(STYX_MANAGED_REF_TAG, state->fields, snapshot->words,
        snapshot->layout.size, state->record, state->capacity, &state->size);
}


/* EN: Register an encoder-owned temporary root within an error boundary.
 * RU: Зарегистрировать временный корень кодировщика под обработчиком ошибок. */
static int root_result_object(void *object)
{
    ptradd(D2H(object));
    Setmark(D2H(object));
    return 1;
}


/* EN: Reserve an index and pin its object until the complete result has been encoded.
 * RU: Зарезервировать индекс и удержать объект до кодирования всего результата. */
static int pin_result_object(DVM_StyxRefTable *table, void *object)
{
    if (dvm_refcount_retain_u(&D2H(object)->ref) != DVM_REFCOUNT_LIVE)
    {
        return 0;
    }
    if (!kprotect(root_result_object, object))
    {
        destroy(object);
        return 0;
    }
    table->objects[table->count++] = object;
    return 1;
}


/* EN/RU: See managed_ref.h. / См. managed_ref.h. */
int dvm_styx_ref_write(DVM_StyxRefTable *table, const char *shape, void *object,
    void *bytes, size_t capacity, size_t *used)
{
    if (object == H)
    {
        return fvm_srecord_write(STYX_MANAGED_NIL_TAG, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    char fields[STYX_SHAPE_MAX + 1];
    DVM_StyxValueFrame snapshot = {0};
    if (!object || !(table->result ? styxmanaged_result_ref_fields(shape, fields) :
        styxmanaged_ref_fields(shape, fields)) || !styxmanaged_layout(fields, &snapshot.layout) ||
        !ref_layout(D2H(object)->t, &snapshot.layout))
    {
        return 0;
    }
    size_t index = 0;
    while (index < table->count && table->objects[index] != object)
    {
        index++;
    }
    unsigned char wire_index = (unsigned char)index;
    if (index < table->count)
    {
        return fvm_srecord_write(STYX_MANAGED_REF_TAG, &wire_index, 1, bytes, capacity, used) == FVM_S_OK;
    }
    if (index >= (table->limit ? table->limit : STYX_CALL_LEGACY_REF_KEYS) || !dvm_styx_value_snapshot(object, &snapshot))
    {
        dvm_styx_value_frame_clear(&snapshot);
        return 0;
    }
    if (table->result && !pin_result_object(table, object))
    {
        dvm_styx_value_frame_clear(&snapshot);
        return 0;
    }
    RefEncode encode = {.snapshot = &snapshot, .fields = fields, .record = malloc(capacity),
        .capacity = capacity, .table = table->result ? table : NULL};
    FVM_SRecord record;
    size_t consumed;
    int valid = encode.record && kprotect(encode_snapshot, &encode) &&
        fvm_srecord_read(encode.record, encode.size, capacity, &record, &consumed) == FVM_S_OK &&
        consumed == encode.size && fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &wire_index, 1,
            record.payload, record.size, bytes, capacity, used) == FVM_S_OK;
    dvm_styx_value_frame_clear(&snapshot);
    free(encode.record);
    if (valid && !table->result)
    {
        table->objects[table->count++] = object;
    }
    return valid;
}


/* EN/RU: See managed_ref.h. / См. managed_ref.h. */
int dvm_styx_ref_slots(const char *shape, const void *arguments, char *refs, void **objects)
{
    StyxScalarLayout layout;
    unsigned mask;
    if (!styxmanaged_layout(shape, &layout) || !styxmanaged_refs(shape, refs, &mask))
    {
        return 0;
    }
    for (size_t i = 0; i < layout.count; i++)
    {
        if (mask & (1U << i))
        {
            memcpy(objects++, (const unsigned char *)arguments + layout.offset[i], sizeof(void *));
        }
    }
    return 1;
}


typedef struct RefSwap
{
    void *original; /* Live caller ADT kept by the pending call frame. */
    void *decoded; /* Rooted private ADT carrying the new fields. */
    StyxScalarLayout layout; /* Exact native fields, excluding tail padding. */
    unsigned mask; /* Only these fields are swapped. */
} RefSwap;


/* EN: Validate every selected object and prepare disjoint targets.
 * RU: Проверить все выбранные объекты и подготовить непересекающиеся цели. */
static int prepare_swaps(const char *refs, void *const *objects, DVM_StyxValueFrame *updates, const unsigned *masks, RefSwap *swaps)
{
    const char *type = refs + 1;
    for (size_t i = 0; i < updates->layout.count; i++)
    {
        char fields[STYX_SHAPE_MAX + 1];
        StyxScalarLayout layout;
        void *decoded;
        memcpy(&decoded, (unsigned char *)updates->words + updates->layout.offset[i], sizeof(decoded));
        if (!objects[i] || objects[i] == H || !decoded || decoded == H ||
            !styxmanaged_ref_fields(type, fields) || !styxmanaged_layout(fields, &layout) ||
            !ref_layout(D2H(objects[i])->t, &layout) || !ref_layout(D2H(decoded)->t, &layout))
        {
            return 0;
        }
        for (size_t j = 0; j < i; j++)
        {
            if (objects[i] == objects[j] || decoded == swaps[j].decoded)
            {
                return 0;
            }
        }
        swaps[i] = (RefSwap){objects[i], decoded, layout, masks ? masks[i] : (1U << layout.count) - 1};
        type = styxshape_end(type);
    }
    return *type == ')';
}


/* EN/RU: See managed_ref.h. / См. managed_ref.h. */
static int ref_commit(const char *refs, void *const *objects, DVM_StyxValueFrame *updates, const unsigned *masks,
    void *destination, DVM_StyxValueFrame *result)
{
    RefSwap swaps[STYX_CALL_VALUES];
    if (!prepare_swaps(refs, objects, updates, masks, swaps) ||
        (result->layout.size && (!destination || destination == H)))
    {
        return 0;
    }
    DVM_RangeAccess access = {0};
    for (;;)
    {
        DVM_CellResult claim = dvm_managed_ranges_begin(NULL, 0, destination, result->layout.size, &access);
        if (claim == DVM_CELL_ACQUIRED)
        {
            for (size_t i = 0; i < updates->layout.count && claim == DVM_CELL_ACQUIRED; i++)
            {
                claim = dvm_range_access_extend(&access, swaps[i].original, swaps[i].layout.size);
            }
            if (claim == DVM_CELL_ACQUIRED)
            {
                break;
            }
            if (!dvm_range_access_end(&access))
            {
                abort();
            }
        }
        if (claim != DVM_CELL_BUSY || !dvm_native_context_poll())
        {
            return 0;
        }
        fvm_port_thread_sleep_us(50);
    }
    /* Swap ownership, leaving the displaced fields in the private decoded ADTs. */
    unsigned char old[64];
    for (size_t i = 0; i < updates->layout.count; i++)
    {
        RefSwap *swap = &swaps[i];
        for (size_t j = 0; j < swap->layout.count; j++)
        {
            if (!(swap->mask & (1U << j)))
            {
                continue;
            }
            size_t offset = swap->layout.offset[j], width = swap->layout.width[j];
            unsigned char *original = (unsigned char *)swap->original + offset;
            unsigned char *decoded = (unsigned char *)swap->decoded + offset;
            memcpy(old, original, width);
            memcpy(original, decoded, width);
            memcpy(decoded, old, width);
            if (swap->layout.pointers & (1U << j))
            {
                void *previous, *next;
                memcpy(&previous, old, sizeof(previous));
                memcpy(&next, original, sizeof(next));
                if (previous && previous != H)
                {
                    Setmark(D2H(previous));
                }
                if (next && next != H)
                {
                    Setmark(D2H(next));
                }
            }
        }
    }
    void *roots[STYX_CALL_VALUES] = {0};
    for (size_t i = 0; i < result->layout.count; i++)
    {
        if (result->roots & (1U << i))
        {
            memcpy(&roots[i], (unsigned char *)result->words + result->layout.offset[i], sizeof(void *));
        }
    }
    if (result->layout.size)
    {
        memcpy(old, destination, result->layout.size);
        memcpy(destination, result->words, result->layout.size);
        memcpy(result->words, old, result->layout.size);
    }
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    for (size_t i = 0; i < result->layout.count; i++)
    {
        if (roots[i])
        {
            ptrdel(D2H(roots[i]));
        }
    }
    result->roots = 0;
    return 1;
}


/* EN/RU: See managed_ref.h. / См. managed_ref.h. */
int dvm_styx_ref_commit(const char *refs, void *const *objects, DVM_StyxValueFrame *updates,
    void *destination, DVM_StyxValueFrame *result)
{
    return ref_commit(refs, objects, updates, NULL, destination, result);
}

#include "managed_ref_delta.inc"


#include "managed_ref_updates.inc"
