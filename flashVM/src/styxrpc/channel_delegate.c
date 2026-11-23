/* Copyright (c) 2026 bhgv
 * EN: Bind exact host-approved reverse PID and module grants to an active AS channel lease.
 * RU: Привязка одобренных хостом полномочий встречного PID и модуля к живой аренде AS.
 */
#include "channel_private.h"
#include "flashvm/userdata.h"
#include <stdlib.h>


/* EN: Recover an active result lease anchored by an exact owner-local channel.
 * RU: Получить действующую аренду результата по точному локальному каналу владельца. */
static FVM_StyxChannelSet *lease_set(FVM_Context *context, const FVM_Value *anchor)
{
    FVM_StyxRemoteChannel *channel = anchor && anchor->type == FVM_VALUE_OBJECT ?
        fvm_styxrpc_channel_lookup(context, anchor->object) : NULL;
    return channel && channel->set && channel->set->lease ? channel->set : NULL;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_pid_release(void *state)
{
    FVM_StyxPidGrant *grant = state;
    styxrpc_channel_lease_release(grant->lease, 1);
    free(grant);
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_delegate_pid(FVM_Context *context, const FVM_Value *anchor,
    const StyxRpcChannelRoute *route, FVM_Value *out)
{
    FVM_StyxChannelSet *set = lease_set(context, anchor);
    FVM_StyxPidGrant *grant;
    FVM_GCRootScope *roots;
    FVM_Value value;
    FVM_Object *object;
    FVM_Result status;
    unsigned char bytes[48];
    size_t size;
    if (!set || !route || route->kind != STYXRPC_ROUTE_PID || !out ||
        styxrpc_channel_route_value(route, bytes, sizeof(bytes), &size) != STYXRPC_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    grant = malloc(sizeof(*grant));
    if (!grant || !styxrpc_channel_lease_retain(set->lease))
    {
        free(grant);
        return FVM_OUT_OF_MEMORY;
    }
    grant->lease = set->lease;
    grant->route = *route;
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        status = fvm_gc_roots_add(roots, anchor);
        if (status == FVM_OK)
        {
            status = fvm_object_create(context, &object);
        }
        if (status == FVM_OK)
        {
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = object;
            status = fvm_userdata_attach(context, object, grant, fvm_styxrpc_pid_release);
            if (status == FVM_OK)
            {
                *out = value;
            }
        }
        fvm_gc_roots_end(roots);
    }
    if (status != FVM_OK)
    {
        styxrpc_channel_lease_release(grant->lease, 0);
        free(grant);
    }
    return status;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_admit_module(FVM_Context *context, const FVM_Value *anchor,
    const StyxRpcChannelRoute *route, FVM_Value *out)
{
    FVM_StyxChannelSet *set = lease_set(context, anchor);
    FVM_GCRootScope *roots;
    FVM_Value value;
    FVM_Result status;
    size_t index;
    if (!set || !out ||
        styxrpc_channel_lease_admit_module(set->lease, route, &index) != STYXRPC_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    ++set->references;
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        status = fvm_gc_roots_add(roots, anchor);
        if (status == FVM_OK)
        {
            const StyxRpcChannelRoute *accepted = styxrpc_channel_lease_route(set->lease, index);
            if (fvm_styxrpc_channel_materialize(context, roots, set, index, &accepted->channel, &value))
            {
                *out = value;
            }
            else
            {
                status = FVM_OUT_OF_MEMORY;
            }
        }
        fvm_gc_roots_end(roots);
    }
    fvm_styxrpc_channel_set_release(set, 0);
    return status;
}
