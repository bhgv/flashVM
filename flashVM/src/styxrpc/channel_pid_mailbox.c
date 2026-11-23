/* Copyright (c) 2026 bhgv
 * EN: Expose native Dis reverse PIDs as ActionScript channels tied to a hosted module result.
 * RU: Представление встречных PID Dis каналами AS под арендой хостового результата модуля.
 */
#include "channel_private.h"
#include "managed_values.h"
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include "flashvm/userdata.h"
#include <stdlib.h>
#include <string.h>

#define FRAME STYXRPC_MANAGED_LIMIT /* Complete mailbox element buffer. */


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_result_mailbox_release(void *state)
{
    fvm_styxrpc_channel_set_release(state, 1);
}


/* [GC:] EN: Dequeue and confirm all PID leaves before exposing typed values on the owner.
 * RU: Извлечь кадр и подтвердить все PID до выдачи типизированного значения владельцу. */
static FVM_Result receive(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    FVM_StyxChannelSet *set = fvm_styxrpc_result_mailbox_lookup(context, self);
    FVM_GCRootScope *roots;
    FVM_StyxChannelDecode decode = {0};
    FVM_StyxRefs refs = {0};
    StyxRpcChannelResult result = {0};
    FVM_Result status;
    StyxRpcStatus received;
    unsigned char *bytes;
    (void)args;
    (void)unused;
    if (argc || !set)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!styxrpc_channel_mailbox_active(set->pid_mailbox))
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC mailbox is closed");
    }
    bytes = malloc(FRAME);
    if (!bytes)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        received = styxrpc_channel_mailbox_receive(set->pid_mailbox, bytes, FRAME, &result.size);
        if (received == STYXRPC_MORE)
        {
            fvm_value_undefined(out);
        }
        else if (received != STYXRPC_OK ||
            styxrpc_channel_pid_import(set->reverse, set->pid_shape, bytes, result.size, &set->pid_routes) != STYXRPC_OK ||
            !styxrpc_channel_mailbox_active(set->pid_mailbox))
        {
            status = fvm_styxrpc_managed_fail(context, "Styx RPC mailbox message was refused");
        }
        else
        {
            decode.set = set;
            decode.delegated = 1;
            refs.graph = 1;
            refs.decode_channel = fvm_styxrpc_channel_decode;
            refs.channel_context = &decode;
            result.selected = 1;
            result.value = bytes;
            if (!fvm_styxrpc_channel_unpack_refs(context, roots, &refs, set->pid_shape, &result, out))
            {
                status = fvm_styxrpc_managed_fail(context, "Styx RPC mailbox message was refused");
            }
        }
        fvm_gc_roots_end(roots);
    }
    free(bytes);
    return status;
}


/* [GC:] EN: Queue admitted PID leaves back toward their issuing client.
 * RU: Поставить принятые ссылки PID в очередь для выдавшего их клиента. */
static FVM_Result send(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    FVM_StyxChannelSet *set = fvm_styxrpc_result_mailbox_lookup(context, self);
    FVM_StyxRefs refs = {0};
    StyxRpcGuard frame = {0};
    FVM_GCRootScope *roots;
    FVM_Result status;
    unsigned char *bytes;
    (void)unused;
    if (argc != 1 || !set)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!styxrpc_channel_mailbox_active(set->pid_mailbox))
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC mailbox is closed");
    }
    bytes = malloc(2U * FRAME);
    if (!bytes)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        StyxRpcStatus sent = STYXRPC_INVALID;
        refs.graph = 1;
        refs.encode_channel = fvm_styxrpc_channel_encode_grant;
        refs.channel_context = set;
        if (fvm_styxrpc_channel_pack_refs(context, roots, &refs, set->pid_shape, args, bytes, bytes + FRAME, &frame))
        {
            sent = styxrpc_channel_mailbox_send(set->pid_mailbox, frame.value, frame.size);
        }
        if (sent == STYXRPC_INVALID)
        {
            status = fvm_styxrpc_managed_fail(context, "Styx RPC mailbox value was refused");
        }
        else
        {
            fvm_value_boolean(out, sent == STYXRPC_OK);
        }
        fvm_gc_roots_end(roots);
    }
    free(bytes);
    return status;
}


/* [GC:] EN: Create a rooted mailbox object which retains its native family.
 * RU: Создать удерживаемый объект очереди со ссылкой на нативное семейство. */
static int make_object(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxChannelSet *set, FVM_Value *value)
{
    FVM_Object *object;
    if (fvm_object_create(context, &object) != FVM_OK)
    {
        return 0;
    }
    fvm_value_undefined(value);
    value->type = FVM_VALUE_OBJECT;
    value->object = object;
    if (fvm_gc_roots_add(roots, value) != FVM_OK ||
        fvm_object_define_method_rooted(context, object, "tryReceive", receive, NULL, NULL) != FVM_OK ||
        fvm_object_define_method_rooted(context, object, "trySend", send, NULL, NULL) != FVM_OK ||
        fvm_userdata_attach(context, object, set, fvm_styxrpc_result_mailbox_release) != FVM_OK)
    {
        return 0;
    }
    ++set->references;
    return 1;
}


/* [GC:] [ASINC] EN: Give the initializer a new mailbox before returning its separate resource.
 * RU: Передать инициализатору новую очередь до выдачи её отдельного ресурса. */
static StyxRpcStatus begin(void *state, const StyxRpcChannelCall *call, StyxRpcChannelOutput *output)
{
    const FVM_StyxRpcPidMailboxExport *binding = state;
    FVM_Context *context = binding->context;
    FVM_StyxChannelSet *set;
    FVM_GCRootScope *roots;
    FVM_Value args[FVM_STYXRPC_LIST_ITEMS + 1], ignored;
    StyxRpcChannelOutput staged = {0};
    unsigned count;
    int ok;
    if (fvm_exception_get(context, &ignored) != FVM_NOT_FOUND || fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    set = fvm_styxrpc_channel_set_create(NULL, binding->reverse, NULL, binding->wait_ms);
    ok = set && fvm_styxrpc_channel_element_shape(binding->schema.result + 1, set->pid_shape) &&
        styxrpc_channel_mailbox_result_create(set->pid_shape, binding->slots, &set->pid_mailbox, &staged) == STYXRPC_OK &&
        fvm_styxrpc_values_decode(context, roots, NULL, binding->schema.arguments, &call->arguments,
            args + 1, FVM_STYXRPC_LIST_ITEMS, &count) &&
        make_object(context, roots, set, args) &&
        fvm_avm1_call(context, binding->initializer, NULL, count + 1, args, &ignored) == FVM_OK;
    if (ok)
    {
        *output = staged;
    }
    else
    {
        if (staged.release)
        {
            staged.release(staged.resource.context);
        }
        fvm_exception_clear(context);
    }
    fvm_styxrpc_channel_set_release(set, 0);
    fvm_gc_roots_end(roots);
    return ok ? STYXRPC_OK : STYXRPC_REMOTE_ERROR;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_pid_mailbox_export(FVM_StyxRpcPidMailboxExport *binding,
    const char *name, uint32_t signature, StyxRpcChannelExport *exported)
{
    unsigned char offer[STYXRPC_MANAGED_PROFILE_BYTES];
    char shape[STYXRPC_MANAGED_SHAPE];
    size_t used;
    if (!binding || !binding->context || !binding->initializer || !*binding->initializer || !binding->reverse ||
        !binding->slots || binding->slots > STYXRPC_CHANNEL_GROUP_CAPACITY ||
        !binding->wait_ms || binding->wait_ms > STYXRPC_SCALAR_WAIT_MAX_MS || !exported ||
        !fvm_styxrpc_channel_supported(&binding->schema) || !fvm_styxrpc_single_channel_result(&binding->schema) ||
        !fvm_styxrpc_channel_element_shape(binding->schema.result + 1, shape) || !strchr(shape, 'c') ||
        styxrpc_channel_profile(name, signature, &binding->schema, offer, sizeof(offer), &used) != STYXRPC_OK)
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
