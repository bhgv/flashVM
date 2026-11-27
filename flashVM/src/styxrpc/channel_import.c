/* Copyright (c) 2026 bhgv
 * EN: Decode a scoped channel result into aliased owner-local FlashVM objects.
 * RU: Декодирование результата с каналами в локальные объекты FlashVM с алиасами.
 */
#include "module_import.h"
#include "channel_lend.h"
#include "managed_values.h"
#include "styxrpc/managed_module.h"
#include <stdlib.h>
#include <string.h>

/* [GC:] EN: Materialize the successful result under one shared owner-local lease.
 * RU: Создать успешный результат под общей арендой владельца. */
static int decode_result(FVM_Context *context, FVM_GCRootScope *roots, const StyxRpcManagedTarget *target,
    const StyxRpcLeasedReply *reply, FVM_Value *out)
{
    FVM_StyxChannelDecode decode = {0};
    FVM_StyxRefs refs = {0};
    unsigned count;
    FVM_Value value;
    int ok;
    if (target->schema.result[1] == ')' && reply->result.tag == STYXRPC_VALUE_NIL && !reply->result.size)
    {
        fvm_value_undefined(out);
        return 1;
    }
    if (reply->lease)
    {
        decode.set = fvm_styxrpc_channel_set_create(reply->lease, NULL, NULL, target->wait_ms);
        if (!decode.set)
        {
            return 0;
        }
    }
    decode.exported = 1;
    refs.graph = 1;
    refs.decode_channel = fvm_styxrpc_channel_decode;
    refs.channel_context = &decode;
    ok = reply->result.tag == STYXRPC_GRAPH_RESULT_TAG &&
        fvm_styxrpc_values_decode(context, roots, &refs, target->schema.result, &reply->result,
            &value, 1, &count) && count == 1;
    if (ok)
    {
        *out = value;
    }
    else if (decode.set)
    {
        fvm_styxrpc_channel_set_disarm(context, decode.set);
    }
    fvm_styxrpc_channel_set_release(decode.set, 0);
    return ok;
}


/* [GC:] EN: Invoke one verified function and preserve the exact fid in its channel objects.
 * RU: Выполнить проверенную функцию и удержать точный fid в объектах её каналов. */
static FVM_Result invoke_scoped(FVM_Context *context, struct StyxFid *directory, const StyxRpcManagedTarget *descriptor,
    StyxRpcArgumentBus *bus, unsigned argc, const FVM_Value *argv, FVM_Value *out)
{
    const StyxRpcManagedTarget target = *descriptor;
    FVM_GCRootScope *roots;
    FVM_StyxLentChannels channels = {0};
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
        refs.encode_channel = fvm_styxrpc_lend_argument;
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
    else if (arguments && !fvm_styxrpc_lend_prepare(context, roots, &channels))
    {
        error = "Styx RPC channel argument scope could not be published";
    }
    else
    {
        StyxRpcOutcome outcome = channels.scope ?
            styxrpc_argument_scope_invoke_at(directory, channels.scope, &target, buffer, size,
                buffer + STYXRPC_MANAGED_LIMIT, STYXRPC_MANAGED_LIMIT, &reply) :
            styxrpc_channel_invoke_at(directory, &target, buffer, size, buffer + STYXRPC_MANAGED_LIMIT,
                STYXRPC_MANAGED_LIMIT, &reply);
        channels.scope = NULL;
        if (arguments && outcome != STYXRPC_CALL_NOT_SENT && !fvm_styxrpc_lend_restore(context, &channels))
        {
            styxrpc_channel_lease_release(outcome == STYXRPC_CALL_OK ? reply.lease : NULL, 0);
            outcome = STYXRPC_CALL_UNCERTAIN;
        }
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
    fvm_styxrpc_lend_clear(&channels);
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
    return invoke_scoped(context, NULL, state, NULL, argc, argv, out);
}


/* [GC:] EN: Call a descriptor that can lend owner-local argument channels.
 * RU: Вызвать описатель, передающий локальные каналы владельца как аргументы. */
FVM_Result fvm_styxrpc_invoke_channels(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *argv, FVM_Value *out, void *state)
{
    const FVM_StyxRpcChannelArguments *binding = state;
    (void)self;
    return fvm_styxrpc_channels_at(context, NULL, binding, argc, argv, out);
}


/* EN/RU: See module_import.h. / См. module_import.h. */
FVM_Result fvm_styxrpc_channels_at(FVM_Context *context, struct StyxFid *directory,
    const FVM_StyxRpcChannelArguments *binding, unsigned argc, const FVM_Value *argv, FVM_Value *out)
{
    return invoke_scoped(context, directory, binding->target, binding->bus, argc, argv, out);
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
    return fvm_register_function(context, global, fvm_styxrpc_invoke_channels, binding);
}
