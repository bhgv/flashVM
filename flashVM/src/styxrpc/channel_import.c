/* Copyright (c) 2026 bhgv
 * EN: Decode a scoped channel result into aliased owner-local FlashVM objects.
 * RU: Декодирование результата с каналами в локальные объекты FlashVM с алиасами.
 */
#include "channel_private.h"
#include "managed_values.h"
#include "styxrpc/managed_module.h"
#include <stdlib.h>
#include <string.h>

#define CHANNEL_LEAF_BYTES 48U /* Bounded encoded module token and channel key. */

typedef struct ResultChannels
{
    const StyxRpcLeasedReply *reply; /* Borrowed while decoding a single result. */
    unsigned wait_ms;                /* Copied into each resulting channel object. */
    FVM_Value objects[STYXRPC_CHANNEL_SLOTS]; /* Rooted identity cache for repeated routes. */
} ResultChannels;


typedef struct ArgumentChannels
{
    StyxRpcArgumentScope *scope; /* Borrowed staged reverse publication. */
    FVM_Value objects[STYXRPC_CHANNEL_SLOTS]; /* Rooted argument identities through retirement. */
    size_t count;                /* Distinct queues already encoded. */
} ArgumentChannels;


/* [GC:] EN: Encode an exact local queue or nil and pin each queue for the complete call.
 * RU: Закодировать локальную очередь либо nil и удержать очередь на весь вызов. */
static int encode_channel(FVM_Context *context, FVM_GCRootScope *roots, void *state,
    const char *type, const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used)
{
    ArgumentChannels *channels = state;
    StyxRpcChannelQueue *queue;
    const char *end;
    size_t size, index;
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
    size = (size_t)(end - type) - 3;
    if (strlen(queue->grant.shape) != size + 2 || memcmp(type + 2, queue->grant.shape + 1, size))
    {
        return 0;
    }
    for (index = 0; index < channels->count && channels->objects[index].object != value->object; ++index)
    {
    }
    if (index == channels->count)
    {
        if (index == STYXRPC_CHANNEL_SLOTS || fvm_gc_roots_add(roots, value) != FVM_OK)
        {
            return 0;
        }
        channels->objects[channels->count++] = *value;
    }
    return styxrpc_argument_scope_value(channels->scope, queue, bytes, capacity, used) == STYXRPC_OK;
}


/* EN: Recheck that argument getters did not close any borrowed local channel.
 * RU: Проверить, что геттеры не закрыли заимствованные локальные каналы. */
static int check_channels(FVM_Context *context, const ArgumentChannels *channels)
{
    size_t i;
    for (i = 0; i < channels->count; ++i)
    {
        if (!fvm_styxrpc_local_channel_queue(context, channels->objects + i))
        {
            return 0;
        }
    }
    return 1;
}


/* [GC:] EN: Match a channel leaf to the prevalidated grants and root its unique object.
 * RU: Сопоставить канал с проверенными полномочиями и удержать его единственный объект. */
static int decode_channel(FVM_Context *context, FVM_GCRootScope *roots, void *state,
    const char *type, const FVM_SRecord *record, FVM_Value *out)
{
    ResultChannels *channels = state;
    const char *end = fvm_styxrpc_type_end(type);
    size_t length, i;
    if (record->tag == STYXRPC_VALUE_NIL && !record->size)
    {
        fvm_value_undefined(out);
        out->type = FVM_VALUE_NULL;
        return 1;
    }
    if (!end || !channels->reply->lease)
    {
        return 0;
    }
    length = (size_t)(end - type) - 3;
    for (i = 0; i < channels->reply->routes.count; ++i)
    {
        const StyxRpcChannelRoute *route = channels->reply->routes.entries + i;
        unsigned char wire[CHANNEL_LEAF_BYTES];
        size_t size, consumed;
        FVM_SRecord exact;
        if (strlen(route->channel.shape) != length + 2 || memcmp(type + 2, route->channel.shape + 1, length) ||
            styxrpc_channel_export_value(route->token, route->channel.key, route->unbuffered,
                wire, sizeof(wire), &size) != STYXRPC_OK ||
            fvm_srecord_read(wire, size, sizeof(wire), &exact, &consumed) != FVM_S_OK ||
            exact.tag != record->tag || exact.size != record->size || memcmp(exact.payload, record->payload, exact.size))
        {
            continue;
        }
        if (channels->objects[i].type != FVM_VALUE_OBJECT &&
            (fvm_styxrpc_channel_object(context, channels->reply->lease, &route->channel, i,
                channels->wait_ms, channels->objects + i) != FVM_OK ||
                fvm_gc_roots_add(roots, channels->objects + i) != FVM_OK))
        {
            return 0;
        }
        *out = channels->objects[i];
        return 1;
    }
    return 0;
}


/* [GC:] EN: Materialize the successful result under one alias cache and GC scope.
 * RU: Создать успешный результат с одним кэшем алиасов в области корней GC. */
static int decode_result(FVM_Context *context, FVM_GCRootScope *roots, const StyxRpcManagedTarget *target,
    const StyxRpcLeasedReply *reply, FVM_Value *out)
{
    ResultChannels channels = {0};
    FVM_StyxRefs refs = {0};
    unsigned count, i;
    FVM_Value value;
    if (target->schema.result[1] == ')' && reply->result.tag == STYXRPC_VALUE_NIL && !reply->result.size)
    {
        fvm_value_undefined(out);
        return 1;
    }
    channels.reply = reply;
    channels.wait_ms = target->wait_ms;
    refs.graph = 1;
    refs.decode_channel = decode_channel;
    refs.channel_context = &channels;
    if (reply->result.tag == STYXRPC_GRAPH_RESULT_TAG &&
        fvm_styxrpc_values_decode(context, roots, &refs, target->schema.result, &reply->result,
            &value, 1, &count) && count == 1)
    {
        *out = value;
        return 1;
    }
    for (i = 0; i < reply->routes.count; ++i)
    {
        if (channels.objects[i].type == FVM_VALUE_OBJECT)
        {
            fvm_styxrpc_channel_disarm(fvm_styxrpc_channel_lookup(context, channels.objects[i].object), 0);
        }
    }
    return 0;
}


/* [GC:] EN: Invoke one verified function and preserve the exact fid in its channel objects.
 * RU: Выполнить проверенную функцию и удержать точный fid в объектах её каналов. */
static FVM_Result invoke_scoped(FVM_Context *context, const StyxRpcManagedTarget *descriptor,
    StyxRpcArgumentBus *bus, unsigned argc, const FVM_Value *argv, FVM_Value *out)
{
    const StyxRpcManagedTarget target = *descriptor;
    FVM_GCRootScope *roots;
    ArgumentChannels channels = {0};
    FVM_StyxRefs refs = {0};
    unsigned char *buffer;
    StyxRpcLeasedReply reply;
    size_t size;
    const char *error = NULL;
    FVM_Result status;
    int arguments;
    if (!fvm_styxrpc_channel_signature(&target.schema))
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel schema is unsupported");
    }
    arguments = strchr(target.schema.arguments, 'c') != NULL;
    if (arguments && !bus)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel arguments require a reverse bus");
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
    if (arguments)
    {
        channels.scope = styxrpc_argument_scope_open(bus, target.pid);
        refs.graph = 1;
        refs.encode_channel = encode_channel;
        refs.channel_context = &channels;
    }
    if (arguments && !channels.scope)
    {
        error = "Styx RPC channel argument scope is unavailable";
    }
    else if (!fvm_styxrpc_values_encode(context, roots, &refs, target.schema.arguments, argv, argc,
        buffer, STYXRPC_MANAGED_LIMIT, &size))
    {
        error = "Styx RPC channel arguments do not fit the schema";
    }
    else if (arguments && !check_channels(context, &channels))
    {
        error = "Styx RPC channel argument scope could not be published";
    }
    else
    {
        StyxRpcOutcome outcome = channels.scope ?
            styxrpc_argument_scope_invoke(channels.scope, &target, buffer, size,
                buffer + STYXRPC_MANAGED_LIMIT, STYXRPC_MANAGED_LIMIT, &reply) :
            styxrpc_channel_invoke(&target, buffer, size, buffer + STYXRPC_MANAGED_LIMIT, STYXRPC_MANAGED_LIMIT, &reply);
        channels.scope = NULL;
        switch (outcome)
        {
        case STYXRPC_CALL_OK:
            if (!decode_result(context, roots, &target, &reply, out))
            {
                error = "Styx RPC channel result could not be decoded";
            }
            styxrpc_channel_lease_release(reply.lease, 0);
            break;
        case STYXRPC_CALL_NOT_SENT:
            error = "Styx RPC call was not sent";
            break;
        case STYXRPC_CALL_REMOTE_ERROR:
            error = "Styx RPC remote function failed";
            break;
        default:
            error = "Styx RPC outcome is uncertain";
            break;
        }
    }
    styxrpc_argument_scope_close(channels.scope);
    fvm_gc_roots_end(roots);
    free(buffer);
    return error ? fvm_styxrpc_managed_fail(context, error) : FVM_OK;
}


/* [GC:] EN: Call a result-channel-only descriptor without a reverse argument bus.
 * RU: Вызвать описатель с каналами результата без встречной шины аргументов. */
static FVM_Result invoke(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *argv, FVM_Value *out, void *state)
{
    (void)self;
    return invoke_scoped(context, state, NULL, argc, argv, out);
}


/* [GC:] EN: Call a descriptor that can lend owner-local argument channels.
 * RU: Вызвать описатель, передающий локальные каналы владельца как аргументы. */
static FVM_Result invoke_arguments(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *argv, FVM_Value *out, void *state)
{
    const FVM_StyxRpcChannelArguments *binding = state;
    (void)self;
    return invoke_scoped(context, binding->target, binding->bus, argc, argv, out);
}


/* EN: Check an exact compiled offer and its mandatory caller scope before registration.
 * RU: Проверить точное компилированное предложение и обязательный контекст. */
static int checked_target(const StyxRpcManagedTarget *target)
{
    unsigned char offer[STYXRPC_MANAGED_PROFILE_BYTES];
    size_t size;
    return target && target->session && target->pid && target->module && *target->module &&
        target->wait_ms && target->wait_ms <= STYXRPC_SCALAR_WAIT_MAX_MS &&
        fvm_styxrpc_channel_signature(&target->schema) &&
        styxrpc_channel_profile(target->name, target->signature, &target->schema, offer, sizeof(offer), &size) == STYXRPC_OK;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_import(FVM_Context *context, const char *global, StyxRpcManagedTarget *target)
{
    if (!context || !global || !*global || !checked_target(target) || strchr(target->schema.arguments, 'c'))
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_register_function(context, global, invoke, target);
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_import_arguments(FVM_Context *context, const char *global,
    FVM_StyxRpcChannelArguments *binding)
{
    if (!context || !global || !*global || !binding || !binding->bus || !checked_target(binding->target) ||
        !strchr(binding->target->schema.arguments, 'c'))
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_register_function(context, global, invoke_arguments, binding);
}
