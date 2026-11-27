/* Copyright (c) 2026 bhgv
 * EN: Publish a native reverse mailbox whose typed messages are consumed on the FlashVM owner.
 * RU: Публикация встречной очереди с обработкой типизированных сообщений у владельца FlashVM.
 */
#include "channel_private.h"
#include "managed_values.h"
#include "wire_wait.h"
#include <stdlib.h>
#include <string.h>

#define FRAME STYXRPC_MANAGED_LIMIT /* Complete typed element buffer. */


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_channel_mailbox_close(FVM_StyxPidGrant *grant, int deferred)
{
    FVM_StyxChannelSet *set = grant->set;
    if (grant->route.pid)
    {
        styxrpc_program_server_revoke(grant->mailbox_server, grant->route.pid);
        grant->route.pid = 0;
    }
    grant->mailbox_output.release(grant->mailbox_output.resource.context);
    grant->mailbox = NULL;
    grant->set = NULL;
    fvm_styxrpc_channel_set_release(set, deferred);
}


/* EN: Revoke this mailbox on the FlashVM owner. Repeated closes are harmless.
 * RU: Отозвать очередь в потоке владельца FlashVM. Повторное закрытие допустимо. */
static FVM_Result close_mailbox(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    FVM_StyxPidGrant *grant = fvm_styxrpc_pid_lookup(context, self);
    (void)args;
    (void)unused;
    if (argc || !grant || !grant->mailbox_server)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (grant->mailbox)
    {
        fvm_styxrpc_channel_mailbox_close(grant, 0);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* [GC:] EN: Dequeue one copied frame and verify its tokens on the original lease.
 * RU: Извлечь один кадр и подтвердить токены на исходной аренде. */
static FVM_Result receive(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    FVM_StyxPidGrant *grant = fvm_styxrpc_pid_lookup(context, self);
    FVM_GCRootScope *roots;
    FVM_Result status;
    FVM_Worker *worker = NULL;
    unsigned char *bytes;
    size_t size;
    StyxRpcStatus received;
    (void)args;
    (void)unused;
    if (argc || !grant || !grant->mailbox_server)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!grant->mailbox)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC mailbox is closed");
    }
    fvm_worker_current(context, &worker);
    if (fvm_styxrpc_worker_stopped(worker))
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC mailbox receive cancelled");
    }
    bytes = malloc(FRAME);
    if (!bytes)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        received = styxrpc_channel_mailbox_receive(grant->mailbox, bytes, FRAME, &size);
        if (received == STYXRPC_MORE)
        {
            fvm_value_undefined(out);
        }
        else if (received != STYXRPC_OK ||
            fvm_styxrpc_channel_decode_message(context, roots, grant->set,
                grant->route.channel.shape, bytes, size, out) != FVM_OK)
        {
            status = fvm_styxrpc_managed_fail(context, "Styx RPC mailbox message was refused");
        }
        fvm_gc_roots_end(roots);
    }
    free(bytes);
    return status;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
int fvm_styxrpc_channel_encode_grant(FVM_Context *context, FVM_GCRootScope *roots, void *state,
    const char *type, const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used)
{
    FVM_StyxChannelSet *set = state;
    FVM_StyxRemoteChannel *channel;
    const StyxRpcChannelRoute *route;
    const char *end;
    size_t width, index;
    (void)roots;
    if (value->type == FVM_VALUE_NULL)
    {
        return fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    channel = value->type == FVM_VALUE_OBJECT ? fvm_styxrpc_channel_lookup(context, value->object) : NULL;
    end = fvm_styxrpc_type_end(type);
    if (!channel || channel->set != set || !end)
    {
        return 0;
    }
    route = fvm_styxrpc_channel_set_route(set, channel->index);
    if (route && route->kind == STYXRPC_ROUTE_PID)
    {
        for (index = 0; index < channel->index; ++index)
        {
            if (set->pid_routes.entries[index].pid == route->pid)
            {
                return 0;
            }
        }
    }
    width = (size_t)(end - type) - 3;
    return route && strlen(route->channel.shape) == width + 2 &&
        !memcmp(route->channel.shape + 1, type + 2, width) &&
        styxrpc_channel_route_value(route, bytes, capacity, used) == STYXRPC_OK;
}


/* [GC:] EN: Queue one typed value toward the peer. Return false on full.
 * RU: Поставить типизированное значение в очередь партнёра. Вернуть false при заполнении. */
static FVM_Result send(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    FVM_StyxPidGrant *grant = fvm_styxrpc_pid_lookup(context, self);
    FVM_StyxChannelSet *set;
    FVM_GCRootScope *roots;
    FVM_StyxRefs refs = {0};
    StyxRpcGuard frame = {0};
    FVM_Result status;
    unsigned char *scratch;
    (void)unused;
    if (argc != 1 || !grant || !grant->mailbox_server)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!grant->mailbox)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC mailbox is closed");
    }
    set = grant->set;
    scratch = set->references != (size_t)-1 ? malloc(2U * FRAME) : NULL;
    if (!scratch)
    {
        return FVM_OUT_OF_MEMORY;
    }
    ++set->references;
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        refs.graph = 1;
        refs.encode_channel = fvm_styxrpc_channel_encode_grant;
        refs.channel_context = set;
        if (!fvm_styxrpc_channel_pack_refs(context, roots, &refs, grant->route.channel.shape,
            args, scratch, scratch + FRAME, &frame) || !grant->mailbox)
        {
            status = fvm_styxrpc_managed_fail(context, "Styx RPC mailbox value was refused");
        }
        else
        {
            const StyxRpcStatus sent = styxrpc_channel_mailbox_send(grant->mailbox, frame.value, frame.size);
            if (sent == STYXRPC_INVALID)
            {
                status = fvm_styxrpc_managed_fail(context, "Styx RPC mailbox value was refused");
            }
            else
            {
                fvm_value_boolean(out, sent == STYXRPC_OK);
            }
        }
        fvm_gc_roots_end(roots);
    }
    fvm_styxrpc_channel_set_release(set, 0);
    free(scratch);
    return status;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_mailbox(FVM_Context *context, const FVM_Value *anchor,
    const char *shape, unsigned slots, StyxRpcServer *reverse, FVM_Value *out)
{
    FVM_StyxChannelSet *set = fvm_styxrpc_channel_lease_set(context, anchor);
    FVM_StyxPidGrant *grant = NULL;
    FVM_GCRootScope *roots;
    FVM_Value value;
    StyxRpcChannelMailbox *mailbox;
    StyxRpcChannelOutput resource;
    FVM_Result status;
    const char *end = shape && *shape == '(' ? fvm_styxrpc_type_end(shape + 1) : NULL;
    if (!set || !reverse || !out || !end || *end != ')' || end[1] ||
        styxrpc_channel_mailbox_create(shape, slots, &mailbox, &resource) != STYXRPC_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        status = fvm_gc_roots_add(roots, anchor);
        if (status == FVM_OK)
        {
            status = fvm_styxrpc_channel_create_pid(context, set, &grant, &value);
        }
        if (status == FVM_OK)
        {
            grant->mailbox = mailbox;
            grant->mailbox_server = reverse;
            grant->mailbox_output = resource;
            grant->route.kind = STYXRPC_ROUTE_PID;
            grant->route.channel.key = resource.key;
            strcpy(grant->route.channel.shape, shape);
            status = fvm_gc_roots_add(roots, &value);
        }
        if (status == FVM_OK)
        {
            status = fvm_object_define_method_rooted(context, value.object, "tryReceive", receive, NULL, NULL);
        }
        if (status == FVM_OK)
        {
            status = fvm_object_define_method_rooted(context, value.object, "trySend", send, NULL, NULL);
        }
        if (status == FVM_OK)
        {
            status = fvm_object_define_method_rooted(context, value.object, "close", close_mailbox, NULL, NULL);
        }
        if (status == FVM_OK)
        {
            grant->route.pid = styxrpc_program_server_grant(reverse, &resource.resource);
            status = grant->route.pid ? FVM_OK : FVM_ERROR;
        }
        if (status == FVM_OK)
        {
            *out = value;
        }
        fvm_gc_roots_end(roots);
    }
    if (status != FVM_OK)
    {
        if (grant)
        {
            fvm_styxrpc_channel_mailbox_close(grant, 0);
        }
        else
        {
            resource.release(resource.resource.context);
        }
    }
    return status;
}


/* [GC:] EN: Validate AS arguments and publish on reverse, borrowed from the host.
 * RU: Проверить аргументы AS и опубликовать очередь на reverse, выданном хостом. */
static FVM_Result create(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *reverse)
{
    unsigned slots;
    FVM_Result status;
    (void)self;
    if (argc != 3 || args[1].type != FVM_VALUE_STRING || args[2].type != FVM_VALUE_NUMBER ||
        !(args[2].number >= 1 && args[2].number <= STYXRPC_CHANNEL_GROUP_CAPACITY) ||
        memchr(args[1].string, 0, fvm_value_string_size(args + 1)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    slots = (unsigned)args[2].number;
    if ((double)slots != args[2].number)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_styxrpc_channel_mailbox(context, args, args[1].string, slots, reverse, out);
    return status == FVM_INVALID_ARGUMENT || status == FVM_ERROR ?
        fvm_styxrpc_managed_fail(context, "Styx RPC mailbox could not be published") : status;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_mailbox_factory(FVM_Context *context, const char *global, StyxRpcServer *reverse)
{
    return context && global && *global && reverse ?
        fvm_register_function(context, global, create, reverse) : FVM_INVALID_ARGUMENT;
}
