/* Copyright (c) 2026 bhgv
 * EN: Bind scoped incoming channel arguments to one AVM1 owner call.
 * RU: Привязка входящих аргументов-каналов к одному вызову у владельца AVM1.
 */
#include "channel_private.h"
#include "managed_values.h"
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include <string.h>

#define CHANNEL_LEAF_BYTES 48U /* Maximum temporary record for a current-bus key. */

typedef struct IncomingChannels
{
    FVM_StyxBorrowedChannels scope;           /* Reverse endpoint while owner is active. */
    unsigned wait_ms;                         /* Per-operation timeout copied into objects. */
    FVM_Value objects[STYXRPC_CHANNEL_SLOTS]; /* Rooted alias cache for this callback. */
} IncomingChannels;


typedef struct HandlerResult
{
    void *bytes;                    /* Copied result buffer owned by the SDK. */
    size_t capacity;                /* Byte capacity of the copied buffer. */
    size_t *used;                   /* Number of copied children bytes written. */
    StyxRpcChannelOutput *channel; /* Transferred channel resource for leased mode. */
    unsigned slots;                 /* Capacity of the leased queue. */
} HandlerResult;


/* EN: Match record to the canonical encoding of one already validated argument grant.
 * RU: Сопоставить record с каноническим кодированием проверенного полномочия аргумента. */
static int same_key(const StyxRpcChannelGrant *grant, const FVM_SRecord *record)
{
    StyxRpcChannelRoute route = {0};
    unsigned char wire[CHANNEL_LEAF_BYTES];
    FVM_SRecord exact;
    size_t size, consumed;
    route.kind = STYXRPC_ROUTE_CURRENT;
    route.channel = *grant;
    return styxrpc_channel_route_value(&route, wire, sizeof(wire), &size) == STYXRPC_OK &&
        fvm_srecord_read(wire, size, sizeof(wire), &exact, &consumed) == FVM_S_OK &&
        exact.tag == record->tag && exact.size == record->size &&
        !memcmp(exact.payload, record->payload, exact.size);
}


/* [GC:] EN: Decode a prevalidated channel leaf using the call's rooted identity cache.
 * RU: Декодировать проверенный лист канала через удерживаемый кэш объектов вызова. */
static int decode_channel(FVM_Context *context, FVM_GCRootScope *roots, void *state,
    const char *type, const FVM_SRecord *record, FVM_Value *out)
{
    IncomingChannels *incoming = state;
    const StyxRpcChannelCall *call = incoming->scope.call;
    size_t i;
    (void)type; /* The whole frame already verified each key's exact type, including aliases. */
    if (record->tag == STYXRPC_VALUE_NIL && !record->size)
    {
        fvm_value_undefined(out);
        out->type = FVM_VALUE_NULL;
        return 1;
    }
    for (i = 0; i < call->count; ++i)
    {
        if (!same_key(call->grants + i, record))
        {
            continue;
        }
        if (incoming->objects[i].type != FVM_VALUE_OBJECT)
        {
            FVM_Value value;
            if (fvm_styxrpc_channel_object(context, NULL, &incoming->scope, call->grants + i, 0,
                incoming->wait_ms, &value) != FVM_OK)
            {
                return 0;
            }
            if (fvm_gc_roots_add(roots, &value) != FVM_OK)
            {
                fvm_styxrpc_channel_disarm(fvm_styxrpc_channel_lookup(context, value.object), 0);
                return 0;
            }
            incoming->objects[i] = value;
        }
        *out = incoming->objects[i];
        return 1;
    }
    return 0;
}


/* [GC:] EN: Marshal the rooted return value to a copied result or a fresh channel.
 * RU: Преобразовать удерживаемый результат в копируемое значение либо новый канал. */
static int write_result(const FVM_StyxRpcChannelHandler *binding, FVM_GCRootScope *roots,
    const FVM_Value *value, const HandlerResult *result)
{
    if (result->channel)
    {
        return fvm_styxrpc_channel_output(binding->context, roots, &binding->schema,
            result->slots, value, result->channel);
    }
    return fvm_styxrpc_values_encode(binding->context, roots, NULL, binding->schema.result,
        value, 1, result->bytes, result->capacity, result->used);
}


/* [GC:] EN: Run the named AVM1 function once and revoke every argument object before returning.
 * RU: Однократно вызвать функцию AVM1 и отозвать все объекты аргументов перед возвратом. */
static StyxRpcStatus invoke(const FVM_StyxRpcChannelHandler *binding, const StyxRpcChannelCall *call,
    const HandlerResult *result)
{
    FVM_Context *context = binding->context;
    IncomingChannels incoming = {0};
    FVM_StyxRefs refs = {0};
    FVM_GCRootScope *roots;
    FVM_Value args[FVM_STYXRPC_LIST_ITEMS], value;
    unsigned count;
    size_t i;
    int ok;
    if (fvm_exception_get(context, &value) != FVM_NOT_FOUND || fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    incoming.scope.reverse = binding->reverse;
    incoming.scope.call = call;
    incoming.wait_ms = binding->wait_ms;
    refs.graph = 1;
    refs.decode_channel = decode_channel;
    refs.channel_context = &incoming;
    ok = fvm_styxrpc_values_decode(context, roots, &refs, binding->schema.arguments, &call->arguments,
            args, FVM_STYXRPC_LIST_ITEMS, &count) &&
        fvm_avm1_call(context, binding->function, NULL, count, args, &value) == FVM_OK &&
        (binding->schema.result[1] == ')' ||
            (fvm_gc_roots_add(roots, &value) == FVM_OK &&
             write_result(binding, roots, &value, result)));
    for (i = 0; i < call->count; ++i)
    {
        if (incoming.objects[i].type == FVM_VALUE_OBJECT)
        {
            fvm_styxrpc_channel_disarm(fvm_styxrpc_channel_lookup(context, incoming.objects[i].object), 0);
        }
    }
    if (!ok)
    {
        fvm_exception_clear(context);
    }
    fvm_gc_roots_end(roots);
    return ok ? STYXRPC_OK : STYXRPC_REMOTE_ERROR;
}


/* [GC:] EN: Run with a caller-owned copied result buffer in bytes.
 * RU: Запустить вызов с буфером копируемого результата в bytes. */
static StyxRpcStatus dispatch(void *state, const StyxRpcChannelCall *call,
    void *bytes, size_t capacity, size_t *used)
{
    HandlerResult result = {0};
    result.bytes = bytes;
    result.capacity = capacity;
    result.used = used;
    *used = 0;
    return invoke(state, call, &result);
}


/* [GC:] EN: Run with a result resource that the dispatcher can take after return.
 * RU: Запустить вызов с ресурсом результата, передаваемым диспетчеру после возврата. */
static StyxRpcStatus dispatch_channel(void *state, const StyxRpcChannelCall *call, StyxRpcChannelOutput *output)
{
    const FVM_StyxRpcChannelHandlerExport *binding = state;
    HandlerResult result = {0};
    result.channel = output;
    result.slots = binding->capacity;
    return invoke(&binding->handler, call, &result);
}


/* EN: Check owner, route, channel inputs and the compiled profile shared by both bindings.
 * RU: Проверить владельца, маршрут, каналы аргументов и общий компилированный профиль. */
static int checked_binding(const FVM_StyxRpcChannelHandler *binding, const char *name, uint32_t signature)
{
    unsigned char offer[STYXRPC_MANAGED_PROFILE_BYTES];
    size_t size;
    return binding && binding->context && binding->function && *binding->function && binding->reverse &&
        binding->wait_ms && binding->wait_ms <= STYXRPC_SCALAR_WAIT_MAX_MS &&
        fvm_styxrpc_channel_signature(&binding->schema) && strchr(binding->schema.arguments, 'c') &&
        styxrpc_channel_profile(name, signature, &binding->schema, offer, sizeof(offer), &size) == STYXRPC_OK;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_handler(FVM_StyxRpcChannelHandler *binding,
    const char *name, uint32_t signature, StyxRpcChannelFunction *function)
{
    if (!function || !checked_binding(binding, name, signature) || strchr(binding->schema.result, 'c'))
    {
        return FVM_INVALID_ARGUMENT;
    }
    function->name = name;
    function->signature = signature;
    function->schema = binding->schema;
    function->handler = dispatch;
    function->context = binding;
    return FVM_OK;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_handler_export(FVM_StyxRpcChannelHandlerExport *binding,
    const char *name, uint32_t signature, StyxRpcChannelExport *exported)
{
    if (!binding || !exported || binding->capacity > FVM_STYXRPC_CHANNEL_QUEUE_SLOTS ||
        !checked_binding(&binding->handler, name, signature) || !fvm_styxrpc_single_channel_result(&binding->handler.schema))
    {
        return FVM_INVALID_ARGUMENT;
    }
    exported->name = name;
    exported->signature = signature;
    exported->schema = binding->handler.schema;
    exported->begin = dispatch_channel;
    exported->context = binding;
    return FVM_OK;
}
