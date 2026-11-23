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
    StyxRpcChannelQueue queue; /* Native queue lent only while the script owner waits. */
    size_t sizes[FVM_STYXRPC_CHANNEL_QUEUE_SLOTS]; /* Length of each encoded element. */
    int closed;                /* Explicit close invalidates the object and aliases. */
} LocalChannel;


/* [GC:] EN: Free the owner-local queue; an active call roots every borrowed object.
 * RU: Освободить очередь владельца. Активный вызов удерживает заимствованные объекты. */
void fvm_styxrpc_local_channel_release(void *state)
{
    free(state);
}


/* EN/RU: See channel_private.h. / См. channel_private.h. */
StyxRpcChannelQueue *fvm_styxrpc_local_channel_queue(FVM_Context *context, const FVM_Value *value)
{
    LocalChannel *local = value->type == FVM_VALUE_OBJECT ?
        fvm_styxrpc_local_channel_lookup(context, value->object) : NULL;
    return local && !local->closed ? &local->queue : NULL;
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
    (void)unused;
    if (!local || argc != 1)
    {
        return FVM_INVALID_ARGUMENT;
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
    encoded = fvm_styxrpc_channel_pack(context, roots, local->queue.grant.shape, args,
        buffer, buffer + STYXRPC_MANAGED_LIMIT, &guard);
    if (encoded && !local->closed && local->queue.count < local->queue.capacity)
    {
        size_t slot = (local->queue.head + local->queue.count) % local->queue.capacity;
        memcpy(local->queue.bytes + slot * local->queue.stride, guard.value, guard.size);
        local->sizes[slot] = guard.size;
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
    (void)args;
    (void)unused;
    if (!local || argc)
    {
        return FVM_INVALID_ARGUMENT;
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
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    decoded = fvm_styxrpc_channel_unpack(context, roots, local->queue.grant.shape, &result, out);
    fvm_gc_roots_end(roots);
    if (!decoded)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel result could not be decoded");
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
    return end && end == shape + size + 1 && !strpbrk(shape, "rhc");
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
