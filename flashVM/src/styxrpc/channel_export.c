/* Copyright (c) 2026 bhgv
 * EN: Issue native UniBus FIFO and rendezvous channels from a FlashVM initializer.
 * RU: Выдача нативных каналов UniBus FIFO и rendezvous из инициализатора FlashVM.
 */
#include "channel_private.h"
#include "managed_values.h"
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include <string.h>


/* EN/RU: See channel_private.h. / См. channel_private.h. */
int fvm_styxrpc_single_channel_result(const StyxRpcSchema *schema)
{
    const char *end;
    return schema->result[1] == 'c' &&
        (end = fvm_styxrpc_type_end(schema->result + 1)) && !strcmp(end, ")");
}


/* [GC:] EN: Run the initializer under GC roots and transfer the exact resource on success.
 * RU: Выполнить инициализатор под корнями GC и передать ресурс при успехе. */
static StyxRpcStatus begin(void *state, const StyxRpcChannelCall *call, StyxRpcChannelOutput *output)
{
    const FVM_StyxRpcChannelExport *binding = state;
    FVM_Context *context = binding->context;
    FVM_GCRootScope *roots;
    FVM_Value args[FVM_STYXRPC_LIST_ITEMS], seed;
    unsigned count;
    int ok;
    if (fvm_exception_get(context, &seed) != FVM_NOT_FOUND ||
        fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    ok = fvm_styxrpc_values_decode(context, roots, NULL, binding->schema.arguments, &call->arguments,
            args, FVM_STYXRPC_LIST_ITEMS, &count) &&
        fvm_avm1_call(context, binding->initializer, NULL, count, args, &seed) == FVM_OK &&
        fvm_gc_roots_add(roots, &seed) == FVM_OK &&
        fvm_styxrpc_channel_output(context, roots, &binding->schema, binding->capacity, &seed, output);
    fvm_gc_roots_end(roots);
    if (!ok)
    {
        fvm_exception_clear(context);
    }
    return ok ? STYXRPC_OK : STYXRPC_REMOTE_ERROR;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_export(FVM_StyxRpcChannelExport *binding,
    const char *name, uint32_t signature, StyxRpcChannelExport *exported)
{
    size_t size;
    unsigned char offer[STYXRPC_MANAGED_PROFILE_BYTES];
    if (!binding || !binding->context || !binding->initializer || !*binding->initializer || !exported ||
        binding->capacity > FVM_STYXRPC_CHANNEL_QUEUE_SLOTS || !fvm_styxrpc_channel_supported(&binding->schema) ||
        !fvm_styxrpc_single_channel_result(&binding->schema) ||
        styxrpc_channel_profile(name, signature, &binding->schema, offer, sizeof(offer), &size) != STYXRPC_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    exported->name = name;
    exported->signature = signature;
    exported->schema = binding->schema;
    exported->begin = begin;
    exported->context = binding;
    return FVM_OK;
}
