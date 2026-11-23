/* Copyright (c) 2026 bhgv
 * EN: Owner-local typed FIFOs that can be lent to a single scoped RPC.
 * RU: Локальные типизированные FIFO, доступные одному контекстному RPC.
 */
#include "channel_private.h"
#include "managed_values.h"
#include "flashvm/userdata.h"
#include <stdlib.h>
#include <string.h>

#define LOCAL_CHANNEL_KEY 1U /* Private key before publication assigns a call key. */

typedef struct LocalChannel
{
    StyxRpcChannelQueue queue; /* The local queue, including when published on a reverse PID. */
    size_t sizes[FVM_STYXRPC_CHANNEL_QUEUE_SLOTS]; /* Length of each encoded element. */
    uint32_t keys[FVM_STYXRPC_CHANNEL_QUEUE_SLOTS][STYXRPC_CHANNEL_SLOTS]; /* Typed leaves of each physical slot. */
    int reading;               /* A local receive holds the current slot through GC. */
    unsigned writing;          /* Active encoders may run reentrant script getters. */
    int closed;                /* Explicit close invalidates the object and aliases. */
    int lent;                  /* Only the reverse dispatcher may use the arena. */
    unsigned references;       /* Userdata and optional PID publication each own one. */
} LocalChannel;


/* [GC:] EN: Drop the userdata or publication reference to this native arena.
 * RU: Снять ссылку userdata либо публикации на этот буфер. */
void fvm_styxrpc_local_channel_release(void *state)
{
    LocalChannel *local = state;
    if (!--local->references)
    {
        free(local);
    }
}


/* EN/RU: See channel_private.h. / См. channel_private.h. */
StyxRpcChannelQueue *fvm_styxrpc_local_channel_queue(FVM_Context *context, const FVM_Value *value)
{
    LocalChannel *local = value->type == FVM_VALUE_OBJECT ?
        fvm_styxrpc_local_channel_lookup(context, value->object) : NULL;
    return local && !local->closed && !local->reading && !local->lent ? &local->queue : NULL;
}


/* EN/RU: See channel_private.h. / См. channel_private.h. */
StyxRpcChannelQueue *fvm_styxrpc_local_channel_borrow(FVM_Context *context, const FVM_Value *value, void **owner)
{
    LocalChannel *local = value && value->type == FVM_VALUE_OBJECT ?
        fvm_styxrpc_local_channel_lookup(context, value->object) : NULL;
    if (!local || local->closed || local->reading || local->writing || local->lent ||
        strchr(local->queue.grant.shape, 'c'))
    {
        return NULL;
    }
    ++local->references;
    local->lent = 1;
    *owner = local;
    return &local->queue;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_local_channel_return(void *owner)
{
    LocalChannel *local = owner;
    local->lent = 0;
    fvm_styxrpc_local_channel_release(local);
}


/* EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_local_slot(FVM_Context *context, FVM_Object *owner, size_t slot, FVM_StyxLocalRefs *refs, int store)
{
    LocalChannel *local = fvm_styxrpc_local_channel_lookup(context, owner);
    size_t index;
    for (index = 0; index < STYXRPC_CHANNEL_SLOTS; ++index)
    {
        if (store)
        {
            local->keys[slot][index] = refs->keys[index];
        }
        else
        {
            refs->keys[index] = local->keys[slot][index];
        }
        refs->objects[index] = fvm_styxrpc_local_edge(owner, slot, index, refs->objects[index], store);
    }
}


/* [GC:] EN: Encode a matching open local channel, rooting each new alias through staging.
 * RU: Кодировать подходящий открытый канал и удерживать каждый новый алиас при подготовке. */
static int encode_child(FVM_Context *context, FVM_GCRootScope *roots, void *state, const char *type,
    const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used)
{
    FVM_StyxLocalRefs *refs = state;
    StyxRpcChannelQueue *queue;
    StyxRpcChannelRoute route = {0};
    const char *end;
    size_t length, index;
    if (value->type == FVM_VALUE_NULL)
    {
        return fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    queue = fvm_styxrpc_local_channel_queue(context, value);
    end = fvm_styxrpc_type_end(type);
    if (!queue || !end)
    {
        return 0;
    }
    length = (size_t)(end - type) - 3;
    if (strlen(queue->grant.shape) != length + 2 || memcmp(queue->grant.shape + 1, type + 2, length))
    {
        return 0;
    }
    for (index = 0; index < STYXRPC_CHANNEL_SLOTS && refs->objects[index] &&
        refs->objects[index] != value->object; ++index)
    {
    }
    if (index == STYXRPC_CHANNEL_SLOTS || fvm_gc_roots_add(roots, value) != FVM_OK)
    {
        return 0;
    }
    refs->objects[index] = value->object;
    refs->keys[index] = (uint32_t)(index + 1);
    route.channel = queue->grant;
    route.channel.key = refs->keys[index];
    return styxrpc_channel_route_value(&route, bytes, capacity, used) == STYXRPC_OK;
}


/* EN: Require every staged nested object to still be open after any script getters.
 * RU: После геттеров проверить, что все подготовленные вложенные объекты открыты. */
static int still_open(FVM_Context *context, const FVM_StyxLocalRefs *refs)
{
    size_t index;
    for (index = 0; index < STYXRPC_CHANNEL_SLOTS && refs->objects[index]; ++index)
    {
        const LocalChannel *child = fvm_styxrpc_local_channel_lookup(context, refs->objects[index]);
        if (!child || child->closed)
        {
            return 0;
        }
    }
    return 1;
}


/* [GC:] EN: Decode a stored ordinary channel key using this message's traced identities.
 * RU: Декодировать сохранённый обычный ключ по удерживаемым объектам этого сообщения. */
static int decode_child(FVM_Context *context, FVM_GCRootScope *roots, void *state,
    const char *type, const FVM_SRecord *record, FVM_Value *value)
{
    const FVM_StyxLocalRefs *refs = state;
    size_t index;
    const char *end = fvm_styxrpc_type_end(type);
    unsigned char wire[48];
    fvm_value_undefined(value);
    if (record->tag == STYXRPC_VALUE_NIL && !record->size)
    {
        value->type = FVM_VALUE_NULL;
        return 1;
    }
    if (!end)
    {
        return 0;
    }
    for (index = 0; index < STYXRPC_CHANNEL_SLOTS && refs->objects[index]; ++index)
    {
        StyxRpcChannelRoute route = {0};
        FVM_SRecord exact;
        size_t length, consumed;
        const LocalChannel *child = fvm_styxrpc_local_channel_lookup(context, refs->objects[index]);
        size_t width = (size_t)(end - type) - 3;
        if (!child || strlen(child->queue.grant.shape) != width + 2 ||
            memcmp(type + 2, child->queue.grant.shape + 1, width))
        {
            continue;
        }
        route.channel = child->queue.grant;
        route.channel.key = refs->keys[index];
        if (styxrpc_channel_route_value(&route, wire, sizeof(wire), &length) == STYXRPC_OK &&
            fvm_srecord_read(wire, length, sizeof(wire), &exact, &consumed) == FVM_S_OK &&
            exact.tag == record->tag && exact.size == record->size && !memcmp(exact.payload, record->payload, exact.size))
        {
            value->type = FVM_VALUE_OBJECT;
            value->object = refs->objects[index];
            return fvm_gc_roots_add(roots, value) == FVM_OK;
        }
    }
    return 0;
}


/* [GC:] EN: Enqueue one verified typed value without yielding the script owner.
 * RU: Поставить проверенное значение в очередь без ожидания владельца. */
static FVM_Result try_send(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    LocalChannel *local = fvm_styxrpc_local_channel_lookup(context, self);
    FVM_GCRootScope *roots;
    unsigned char *buffer;
    StyxRpcGuard guard = {0};
    FVM_Result status;
    int encoded;
    FVM_StyxLocalRefs edges = {0};
    FVM_StyxRefs refs = {0};
    (void)unused;
    if (!local || argc != 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (local->reading || local->lent)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel is busy");
    }
    if (local->closed)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel is closed");
    }
    buffer = malloc(2U * STYXRPC_MANAGED_LIMIT);
    if (!buffer)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        free(buffer);
        return status;
    }
    refs.graph = 1;
    refs.encode_channel = encode_child;
    refs.channel_context = &edges;
    ++local->writing;
    encoded = fvm_styxrpc_channel_pack_refs(context, roots, &refs, local->queue.grant.shape, args,
        buffer, buffer + STYXRPC_MANAGED_LIMIT, &guard) && still_open(context, &edges);
    --local->writing;
    if (encoded && !local->closed && local->queue.count < local->queue.capacity)
    {
        size_t slot = (local->queue.head + local->queue.count) % local->queue.capacity;
        memcpy(local->queue.bytes + slot * local->queue.stride, guard.value, guard.size);
        local->sizes[slot] = guard.size;
        if (strchr(local->queue.grant.shape, 'c'))
        {
            fvm_styxrpc_local_slot(context, self, slot, &edges, 1);
        }
        ++local->queue.count;
        fvm_value_boolean(out, 1);
    }
    else
    {
        fvm_value_boolean(out, 0);
    }
    fvm_gc_roots_end(roots);
    free(buffer);
    if (local->closed)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel is closed");
    }
    return encoded ? FVM_OK : fvm_styxrpc_managed_fail(context, "Styx RPC channel value does not fit the schema");
}


/* [GC:] EN: Decode before consuming and distinguish an empty FIFO from a received nil.
 * RU: Декодировать до удаления и различать пустую очередь и полученный nil. */
static FVM_Result try_receive(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    LocalChannel *local = fvm_styxrpc_local_channel_lookup(context, self);
    FVM_GCRootScope *roots;
    FVM_Result status;
    StyxRpcChannelResult result = {0};
    const unsigned char *bytes;
    int64_t number;
    int decoded;
    FVM_StyxLocalRefs edges = {0};
    FVM_StyxRefs refs = {0};
    int nested;
    (void)args;
    (void)unused;
    if (!local || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (local->reading || local->lent)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel is busy");
    }
    if (local->closed)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel is closed");
    }
    if (!local->queue.count)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    bytes = local->queue.bytes + local->queue.head * local->queue.stride;
    result.selected = 1;
    result.value = bytes;
    result.size = local->sizes[local->queue.head];
    if (!strcmp(local->queue.grant.shape, "(i)"))
    {
        if (styxrpc_value_get_integer(STYXRPC_VALUE_INT, bytes, result.size, &number) != STYXRPC_OK)
        {
            return FVM_ERROR;
        }
        result.integer = (int32_t)number;
    }
    local->reading = 1;
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        local->reading = 0;
        return status;
    }
    nested = strchr(local->queue.grant.shape, 'c') != NULL;
    if (nested)
    {
        fvm_styxrpc_local_slot(context, self, local->queue.head, &edges, 0);
        refs.graph = 1;
        refs.decode_channel = decode_child;
        refs.channel_context = &edges;
    }
    decoded = fvm_styxrpc_channel_unpack_refs(context, roots, nested ? &refs : NULL, local->queue.grant.shape, &result, out);
    fvm_gc_roots_end(roots);
    local->reading = 0;
    if (!decoded)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel result could not be decoded");
    }
    if (nested)
    {
        memset(&edges, 0, sizeof(edges));
        fvm_styxrpc_local_slot(context, self, local->queue.head, &edges, 1);
    }
    local->queue.head = (local->queue.head + 1) % local->queue.capacity;
    --local->queue.count;
    return FVM_OK;
}


/* EN: Disarm this identity; storage remains alive under its GC roots.
 * RU: Отключить идентификатор, сохраняя память под его корнями GC. */
static FVM_Result close_local(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    LocalChannel *local = fvm_styxrpc_local_channel_lookup(context, self);
    (void)args;
    (void)unused;
    if (!local || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (local->reading || local->lent)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel is busy");
    }
    if (strchr(local->queue.grant.shape, 'c'))
    {
        FVM_StyxLocalRefs edges = {0};
        size_t slot;
        for (slot = 0; slot < local->queue.capacity; ++slot)
        {
            fvm_styxrpc_local_slot(context, self, slot, &edges, 1);
        }
    }
    local->closed = 1;
    fvm_value_undefined(out);
    return FVM_OK;
}


/* EN: Check a complete bounded value element and an exact native slot count.
 * RU: Проверить полный ограниченный тип элемента и точное число слотов. */
static int local_shape(const FVM_Value *args, char *shape, unsigned *slots)
{
    const char *end;
    size_t size;
    if (args[0].type != FVM_VALUE_STRING || args[1].type != FVM_VALUE_NUMBER ||
        !(args[1].number >= 0 && args[1].number <= FVM_STYXRPC_CHANNEL_QUEUE_SLOTS))
    {
        return 0;
    }
    *slots = (unsigned)args[1].number;
    size = fvm_value_string_size(args);
    if ((double)*slots != args[1].number || !size || size > STYXRPC_MANAGED_SHAPE - 3 ||
        memchr(args[0].string, 0, size))
    {
        return 0;
    }
    shape[0] = '(';
    memcpy(shape + 1, args[0].string, size);
    shape[size + 1] = ')';
    shape[size + 2] = 0;
    end = fvm_styxrpc_type_end(shape + 1);
    return end && end == shape + size + 1 && !strpbrk(shape, "rh");
}


/* [GC:] EN: Allocate a new typed FIFO and attach its lifetime to one script object.
 * RU: Выделить новый типизированный FIFO и передать владение объекту AS. */
static FVM_Result create(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    LocalChannel *local;
    FVM_GCRootScope *roots;
    FVM_Object *object;
    FVM_Value value;
    FVM_Result status;
    unsigned slots;
    char shape[STYXRPC_MANAGED_SHAPE] = {0};
    (void)self;
    (void)unused;
    if (argc != 2 || !local_shape(args, shape, &slots))
    {
        return FVM_INVALID_ARGUMENT;
    }
    local = calloc(1, sizeof(*local) + slots * (size_t)STYXRPC_MANAGED_LIMIT);
    if (!local)
    {
        return FVM_OUT_OF_MEMORY;
    }
    local->queue.grant.key = LOCAL_CHANNEL_KEY;
    local->references = 1;
    memcpy(local->queue.grant.shape, shape, sizeof(shape));
    local->queue.bytes = (unsigned char *)(local + 1);
    local->queue.sizes = local->sizes;
    local->queue.capacity = slots;
    local->queue.stride = STYXRPC_MANAGED_LIMIT;
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        status = fvm_object_create(context, &object);
        if (status == FVM_OK)
        {
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = object;
            status = fvm_gc_roots_add(roots, &value);
        }
        if (status == FVM_OK)
        {
            status = fvm_object_define_method_rooted(context, object, "trySend", try_send, NULL, NULL);
        }
        if (status == FVM_OK)
        {
            status = fvm_object_define_method_rooted(context, object, "tryReceive", try_receive, NULL, NULL);
        }
        if (status == FVM_OK)
        {
            status = fvm_object_define_method_rooted(context, object, "close", close_local, NULL, NULL);
        }
        if (status == FVM_OK && strchr(shape, 'c') && slots)
        {
            status = fvm_styxrpc_local_edges_init(context, object, slots);
        }
        if (status == FVM_OK)
        {
            status = fvm_userdata_attach(context, object, local, fvm_styxrpc_local_channel_release);
        }
        if (status == FVM_OK)
        {
            *out = value;
        }
        fvm_gc_roots_end(roots);
    }
    if (status != FVM_OK)
    {
        free(local);
    }
    return status;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_factory(FVM_Context *context, const char *global)
{
    return context && global && *global ? fvm_register_function(context, global, create, NULL) :
        FVM_INVALID_ARGUMENT;
}
