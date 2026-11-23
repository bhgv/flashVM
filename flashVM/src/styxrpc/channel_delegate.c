/* Copyright (c) 2026 bhgv
 * EN: Bind exact host-approved reverse PID and module grants to an active AS channel lease.
 * RU: Привязка одобренных хостом полномочий встречного PID и модуля к живой аренде AS.
 */
#include "channel_private.h"
#include "managed_values.h"
#include "flashvm/userdata.h"
#include "styxrpc/channel_group.h"
#include <stdlib.h>


/* EN: Own a PID and the native queue pin independently of either script object.
 * RU: Владеть PID и ссылкой на очередь независимо от объектов AS. */
typedef struct FVM_StyxLocalLoan
{
    struct FVM_StyxLocalLoan *next;  /* Next publication of this result family. */
    StyxRpcServer *reverse;          /* Borrowed server on the same reverse mount. */
    StyxRpcChannelOutput output;     /* Resource with exclusive use of the FIFO arenas. */
    void *owner;                     /* Retained local arena until return. */
    uint32_t pid;                    /* Published route, zero during construction. */
} FVM_StyxLocalLoan;


/* [GC:] EN: Revoke loan before restoring its queue and freeing its pin.
 * RU: Отозвать loan до возврата очереди и освобождения ссылки. */
static void retire_loan(FVM_StyxLocalLoan *loan)
{
    if (loan->pid)
    {
        styxrpc_program_server_revoke(loan->reverse, loan->pid);
    }
    if (loan->output.release)
    {
        loan->output.release(loan->output.resource.context);
    }
    if (loan->owner)
    {
        fvm_styxrpc_local_channel_return(loan->owner);
    }
    free(loan);
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_channel_retire_loans(FVM_StyxChannelSet *set)
{
    while (set->loans)
    {
        FVM_StyxLocalLoan *loan = set->loans;
        set->loans = loan->next;
        retire_loan(loan);
    }
}


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
    fvm_styxrpc_channel_set_release(grant->set, 1);
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
    if (!grant || set->references == (size_t)-1)
    {
        free(grant);
        return FVM_OUT_OF_MEMORY;
    }
    ++set->references;
    grant->set = set;
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
        fvm_styxrpc_channel_set_release(set, 0);
        free(grant);
    }
    return status;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_delegate_local(FVM_Context *context, const FVM_Value *anchor,
    const FVM_Value *value, StyxRpcServer *reverse, FVM_Value *out)
{
    FVM_StyxChannelSet *set = lease_set(context, anchor);
    FVM_StyxLocalLoan *loan;
    StyxRpcChannelQueue *queue;
    StyxRpcChannelRoute route = {0};
    FVM_Result status;
    if (!set || !reverse || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    loan = calloc(1, sizeof(*loan));
    if (!loan)
    {
        return FVM_OUT_OF_MEMORY;
    }
    loan->reverse = reverse;
    queue = fvm_styxrpc_local_channel_borrow(context, value, &loan->owner);
    if (!queue || styxrpc_channel_group_reverse_borrow(queue, &loan->output) != STYXRPC_OK)
    {
        retire_loan(loan);
        return FVM_INVALID_ARGUMENT;
    }
    route.kind = STYXRPC_ROUTE_PID;
    route.channel = queue->grant;
    route.unbuffered = loan->output.unbuffered;
    loan->pid = styxrpc_program_server_grant(reverse, &loan->output.resource);
    route.pid = loan->pid;
    status = loan->pid ? fvm_styxrpc_channel_delegate_pid(context, anchor, &route, out) : FVM_ERROR;
    if (status != FVM_OK)
    {
        retire_loan(loan);
        return status;
    }
    loan->next = set->loans;
    set->loans = loan;
    return FVM_OK;
}


/* [GC:] EN: Publish args[1] on the host-authorized reverse server using args[0]
 * as the lease anchor. Return the opaque PID value in out.
 * RU: Опубликовать args[1] на выданном хостом встречном сервере под арендой args[0].
 * Вернуть непрозрачное значение PID в out. */
static FVM_Result publish_local(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *reverse)
{
    FVM_Result status;
    (void)self;
    if (argc != 2)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_styxrpc_channel_delegate_local(context, args, args + 1, reverse, out);
    if (status == FVM_INVALID_ARGUMENT || status == FVM_ERROR)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC local channel could not be published");
    }
    return status;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_publisher(FVM_Context *context, const char *global, StyxRpcServer *reverse)
{
    return context && global && *global && reverse ?
        fvm_register_function(context, global, publish_local, reverse) : FVM_INVALID_ARGUMENT;
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
