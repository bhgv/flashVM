/* Copyright (c) 2026 bhgv
 * EN: Bind scoped incoming channel arguments to one AVM1 owner call.
 * RU: Привязка входящих аргументов-каналов к одному вызову у владельца AVM1.
 */
#include "channel_private.h"
#include "managed_values.h"
#include "graph_refs.h"
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include <string.h>

typedef struct HandlerResult
{
    void *bytes;                    /* Copied result buffer owned by the SDK. */
    size_t capacity;                /* Byte capacity of the copied buffer. */
    size_t *used;                   /* Number of copied children bytes written. */
    StyxRpcChannelOutput *channel; /* Transferred channel resource for leased mode. */
    unsigned slots;                 /* Capacity of the leased queue. */
} HandlerResult;


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


/* [GC:] [ASINC] EN: Run the named AVM1 function once and revoke every argument object before returning.
 * RU: Однократно вызвать функцию AVM1 и отозвать все объекты аргументов перед возвратом. */
static StyxRpcStatus invoke(const FVM_StyxRpcChannelHandler *binding, const StyxRpcChannelCall *call,
    const HandlerResult *result)
{
    FVM_Context *context = binding->context;
    StyxRpcChannelCall scoped = *call;
    FVM_StyxChannelDecode decode = {0};
    FVM_StyxRefs refs = {0};
    FVM_GCRootScope *roots;
    FVM_Value args[FVM_STYXRPC_LIST_ITEMS], value;
    unsigned count;
    int ok;
    if (fvm_exception_get(context, &value) != FVM_NOT_FOUND || fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    decode.set = fvm_styxrpc_channel_set_create(NULL, binding->reverse, &scoped, binding->wait_ms);
    refs.graph = 1;
    refs.decode_channel = fvm_styxrpc_channel_decode;
    refs.channel_context = &decode;
    ok = decode.set && (!strpbrk(binding->schema.arguments, "rh") || fvm_styxrpc_graph_init(&refs)) &&
        fvm_styxrpc_values_decode(context, roots, &refs, binding->schema.arguments, &call->arguments,
            args, FVM_STYXRPC_LIST_ITEMS, &count);
    refs.originals = refs.count;
    ok = ok && fvm_avm1_call(context, binding->function, NULL, count, args, &value) == FVM_OK &&
        (binding->schema.result[1] == ')' ||
            (fvm_gc_roots_add(roots, &value) == FVM_OK &&
             write_result(binding, roots, &value, result))) &&
        (!result->channel || fvm_styxrpc_channel_updates(context, roots, &refs, &binding->schema,
            &call->arguments, result->channel));
    if (decode.set)
    {
        fvm_styxrpc_channel_set_disarm(context, decode.set);
        fvm_styxrpc_channel_set_release(decode.set, 0);
    }
    if (!ok)
    {
        fvm_exception_clear(context);
    }
    fvm_styxrpc_graph_clear(&refs);
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
    if (!function || !checked_binding(binding, name, signature) ||
        strpbrk(binding->schema.arguments, "rh") || strpbrk(binding->schema.result, "rhc"))
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
