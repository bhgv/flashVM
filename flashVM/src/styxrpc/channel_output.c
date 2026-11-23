/* Copyright (c) 2026 bhgv
 * EN: Materialize ActionScript channel seeds as one portable module-owned queue graph.
 * RU: Материализация начальных данных ActionScript в переносимый граф очередей модуля.
 */
#include "channel_private.h"
#include "managed_values.h"
#include "styxrpc/channel_group.h"
#include <stdlib.h>
#include <string.h>

#define FRAME STYXRPC_MANAGED_LIMIT /* Maximum supported element staging length. */

typedef struct ChannelSeeds
{
    StyxRpcChannelQueue queues[STYXRPC_CHANNEL_SLOTS]; /* Staged capacity and encoded frames. */
    size_t sizes[STYXRPC_CHANNEL_SLOTS][FVM_STYXRPC_CHANNEL_QUEUE_SLOTS]; /* Per-queue frame lengths. */
    FVM_Object *identities[STYXRPC_CHANNEL_SLOTS]; /* Rooted nested descriptors used for aliasing. */
    size_t count; /* Reserved entries, including the root channel. */
} ChannelSeeds;


/* EN: Store one compiled channel's element shape as a parenthesized type list.
 * RU: Записать тип элемента компилированного канала в виде списка в скобках. */
static int element_shape(const char *type, char *shape)
{
    const char *end = fvm_styxrpc_type_end(type);
    size_t length;
    if (!end || *type != 'c')
    {
        return 0;
    }
    length = (size_t)(end - type) - 3;
    if (length > STYXRPC_MANAGED_SHAPE - 3)
    {
        return 0;
    }
    shape[0] = '(';
    memcpy(shape + 1, type + 2, length);
    shape[length + 1] = ')';
    shape[length + 2] = 0;
    return 1;
}


/* [GC:] EN: Read only a descriptor's own indexed property and root the value.
 * RU: Прочитать собственное свойство по индексу и добавить значение под корни. */
static int item(FVM_Context *context, FVM_GCRootScope *roots, const FVM_Value *array, size_t index, FVM_Value *out)
{
    char name[] = {'0', 0};
    int present;
    name[0] = (char)('0' + index);
    return array->type == FVM_VALUE_OBJECT &&
        fvm_object_has_own(context, array->object, name, &present) == FVM_OK && present &&
        fvm_object_get(context, array->object, name, out) == FVM_OK &&
        fvm_gc_roots_add(roots, out) == FVM_OK;
}


/* EN: Read an exact short list length or a bounded FIFO capacity.
 * RU: Прочитать точную длину короткого списка либо ограниченную ёмкость FIFO. */
static int bounded(const FVM_Value *value, size_t limit, size_t *size)
{
    if (value->type != FVM_VALUE_NUMBER || !(value->number >= 0 && value->number <= (double)limit))
    {
        return 0;
    }
    *size = (size_t)value->number;
    return (double)*size == value->number;
}


/* EN: Reserve one queue slot and allocate its temporary seed frame storage.
 * RU: Зарезервировать одну очередь и выделить временную память начальных кадров. */
static int prepare(ChannelSeeds *seeds, size_t index, const char *shape, size_t capacity, size_t items)
{
    StyxRpcChannelQueue *queue = seeds->queues + index;
    queue->grant.key = (uint32_t)(index + 1);
    memcpy(queue->grant.shape, shape, strlen(shape) + 1);
    queue->capacity = capacity;
    queue->stride = FRAME;
    queue->sizes = seeds->sizes[index];
    if (items)
    {
        queue->bytes = malloc(items * (size_t)FRAME);
        if (!queue->bytes)
        {
            return 0;
        }
    }
    return 1;
}


/* [GC:] EN: Encode a nil or nested descriptor into an owner-issued channel key.
 * RU: Закодировать nil или вложенный описатель ключом канала этого владельца. */
static int nested(FVM_Context *context, FVM_GCRootScope *roots, void *state, const char *type,
    const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used);


/* [GC:] EN: Append one verified seed frame; nested descriptors can reserve more queues.
 * RU: Добавить один проверенный начальный кадр. Вложенные описатели могут выделять очереди. */
static int append(FVM_Context *context, FVM_GCRootScope *roots, ChannelSeeds *seeds, size_t index, const FVM_Value *value)
{
    StyxRpcChannelQueue *queue = seeds->queues + index;
    unsigned char *scratch = malloc(FRAME);
    unsigned char *target = queue->bytes + queue->count * queue->stride;
    StyxRpcGuard frame = {0};
    FVM_StyxRefs refs = {0};
    int ok;
    refs.graph = 1;
    refs.encode_channel = nested;
    refs.channel_context = seeds;
    ok = scratch && fvm_styxrpc_channel_pack_refs(context, roots, &refs,
        queue->grant.shape, value, scratch, target, &frame);
    if (ok)
    {
        if (frame.value != target)
        {
            memcpy(target, frame.value, frame.size);
        }
        queue->sizes[queue->count++] = frame.size;
    }
    free(scratch);
    return ok;
}


/* [GC:] EN: Read and materialize the dense seeds of a new [capacity, items] descriptor.
 * RU: Прочитать и материализовать плотный список значений описателя [ёмкость, элементы]. */
static int prepare_nested(FVM_Context *context, FVM_GCRootScope *roots, ChannelSeeds *seeds,
    const FVM_Value *value, const char *shape, size_t index)
{
    FVM_Value slots, items, length, element;
    size_t capacity, count, arity, i;
    if (fvm_gc_roots_add(roots, value) != FVM_OK ||
        fvm_object_get(context, value->object, "length", &length) != FVM_OK ||
        !bounded(&length, 2, &arity) || arity != 2 ||
        !item(context, roots, value, 0, &slots) ||
        !bounded(&slots, FVM_STYXRPC_CHANNEL_QUEUE_SLOTS, &capacity) ||
        !item(context, roots, value, 1, &items) || items.type != FVM_VALUE_OBJECT ||
        fvm_object_get(context, items.object, "length", &length) != FVM_OK ||
        !bounded(&length, capacity, &count) || !prepare(seeds, index, shape, capacity, count))
    {
        return 0;
    }
    for (i = 0; i < count; ++i)
    {
        if (!item(context, roots, &items, i, &element) || !append(context, roots, seeds, index, &element))
        {
            return 0;
        }
    }
    return 1;
}


/* [GC:] EN: Add one nested seed exactly once or reuse a matching descriptor and type.
 * RU: Однократно добавить вложенный канал либо использовать описатель того же типа. */
static int nested(FVM_Context *context, FVM_GCRootScope *roots, void *state, const char *type,
    const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used)
{
    ChannelSeeds *seeds = state;
    StyxRpcChannelRoute route = {0};
    char shape[STYXRPC_MANAGED_SHAPE];
    size_t index;
    if (value->type == FVM_VALUE_NULL)
    {
        return fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    if (value->type != FVM_VALUE_OBJECT || !element_shape(type, shape))
    {
        return 0;
    }
    for (index = 1; index < seeds->count && seeds->identities[index] != value->object; ++index)
    {
    }
    if (index == seeds->count)
    {
        if (index == STYXRPC_CHANNEL_SLOTS)
        {
            return 0;
        }
        seeds->identities[index] = value->object;
        ++seeds->count;
        if (!prepare_nested(context, roots, seeds, value, shape, index))
        {
            return 0;
        }
    }
    if (strcmp(shape, seeds->queues[index].grant.shape))
    {
        return 0;
    }
    route.kind = STYXRPC_ROUTE_CURRENT;
    route.channel = seeds->queues[index].grant;
    route.unbuffered = !seeds->queues[index].capacity;
    return styxrpc_channel_route_value(&route, bytes, capacity, used) == STYXRPC_OK;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
int fvm_styxrpc_channel_output(FVM_Context *context, FVM_GCRootScope *roots,
    const StyxRpcSchema *schema, unsigned capacity, const FVM_Value *seed, StyxRpcChannelOutput *output)
{
    ChannelSeeds seeds = {0};
    char shape[STYXRPC_MANAGED_SHAPE];
    size_t count = seed->type != FVM_VALUE_UNDEFINED;
    size_t index;
    int ok;
    seeds.count = 1;
    ok = count <= capacity && element_shape(schema->result + 1, shape) &&
        prepare(&seeds, 0, shape, capacity, count) &&
        (!count || append(context, roots, &seeds, 0, seed)) &&
        styxrpc_channel_group_output(seeds.queues, seeds.count, 0, output) == STYXRPC_OK;
    for (index = 0; index < seeds.count; ++index)
    {
        free(seeds.queues[index].bytes);
    }
    return ok;
}
