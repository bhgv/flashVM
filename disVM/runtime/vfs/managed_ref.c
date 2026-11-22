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
#include "bigtable_owner.h"
#include "table.h"
#include "../../native/terminal/scalar_record.h"
#include "thread_port.h"
#include "disvm/refcount.h"
#include <stdlib.h>
#include <string.h>

#include "managed_table.inc"


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


/* EN: Retain an object independently of its enclosing value during traversal.
 * RU: Удержать объект независимо от содержащего его значения на время обхода. */
static int pin_ref_object(void *object);


/* EN/RU: See managed_ref.h. / См. managed_ref.h. */
void dvm_styx_ref_release(DVM_StyxRefTable *table, size_t first)
{
    while (table->count > first)
    {
        void *object = table->objects[--table->count];
        table->objects[table->count] = NULL;
        if (object)
        {
            ptrdel(D2H(object));
            destroy(object);
        }
    }
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
    if (index >= (table->limit ? table->limit : STYX_CALL_LEGACY_REF_KEYS))
    {
        return 0;
    }
    if (*shape == 'h')
    {
        void *value = table->result ? table_read(node->payload + 1, node->size - 1) : NULL;
        if (!value || !pin_ref_object(value))
        {
            if (value)
            {
                destroy(value);
            }
            return 0;
        }
        table->objects[table->count++] = value;
        *result = value;
        return 1;
    }
    char fields[STYX_SHAPE_MAX + 1];
    if (!(table->result ? styxmanaged_result_ref_fields(shape, fields) : styxmanaged_ref_fields(shape, fields)))
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
    if (table->result && !pin_ref_object(allocation.object))
    {
        destroy(allocation.object);
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
static int root_ref_object(void *object)
{
    ptradd(D2H(object));
    Setmark(D2H(object));
    return 1;
}


static int pin_ref_object(void *object)
{
    if (dvm_refcount_retain_u(&D2H(object)->ref) != DVM_REFCOUNT_LIVE)
    {
        return 0;
    }
    if (!kprotect(root_ref_object, object))
    {
        destroy(object);
        return 0;
    }
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
    int hybrid = *shape == 'h';
    if (!object || (hybrid ? !table->result || dvm_bigtable_native_kind(D2H(object)) != DVM_BIGTABLE_NATIVE_TABLE :
        !(table->result ? styxmanaged_result_ref_fields(shape, fields) : styxmanaged_ref_fields(shape, fields)) ||
        !styxmanaged_layout(fields, &snapshot.layout) || !ref_layout(D2H(object)->t, &snapshot.layout)))
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
    if (hybrid)
    {
        if (index >= table->limit || !pin_ref_object(object))
        {
            return 0;
        }
        table->objects[table->count++] = object;
        unsigned char *record = malloc(capacity);
        size_t size;
        int valid = record && table_write(object, record, capacity, &size) &&
            fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &wire_index, 1, record, size,
                bytes, capacity, used) == FVM_S_OK;
        free(record);
        return valid;
    }
    if (index >= (table->limit ? table->limit : STYX_CALL_LEGACY_REF_KEYS) || !dvm_styx_value_snapshot(object, &snapshot))
    {
        dvm_styx_value_frame_clear(&snapshot);
        return 0;
    }
    if (table->result)
    {
        if (!pin_ref_object(object))
        {
            dvm_styx_value_frame_clear(&snapshot);
            return 0;
        }
        table->objects[table->count++] = object;
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


#include "managed_ref_commit.inc"

#include "managed_ref_delta.inc"


#include "managed_ref_updates.inc"

#include "managed_graph_reply.inc"
