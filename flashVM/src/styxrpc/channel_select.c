/* Copyright (c) 2026 bhgv
 * EN: Convert script guard arrays into one native ALT on the anchored UniBus.
 * RU: Преобразует массивы веток сценария в один нативный ALT на выбранной шине UniBus.
 */
#include "channel_private.h"
#include "managed_values.h"
#include "flashvm/execution.h"
#include <stdlib.h>
#include <string.h>

#define FRAME STYXRPC_MANAGED_LIMIT /* Native byte limit for one complete selection. */
#define SELECT_BUFFERS 6U /* Payload, typed element, operands, ALT scratch, request and reply. */

static const StyxRpcChannelMode blocking = STYXRPC_CHANNEL_ALT; /* Wait for a native branch. */
static const StyxRpcChannelMode immediate = STYXRPC_CHANNEL_TRY; /* Return idle if none is ready. */


/* [GC:] EN: Snapshot each dense guard and retain its channel and optional operand under roots.
 * Only the anchor's exact bus may accept the resulting selection.
 * RU: Снять копию каждой плотной ветки и удержать канал и необязательный операнд под roots.
 * Полученный выбор может принять только точная шина anchor. */
static FVM_Result snapshot(FVM_Context *context, FVM_GCRootScope *roots, const FVM_Value *list,
    const FVM_StyxRemoteChannel *anchor, StyxRpcSelection *selection,
    FVM_StyxRemoteChannel **channels, FVM_Value *operands)
{
    FVM_Value rows[STYXRPC_CHANNEL_SLOTS];
    unsigned i, count = list->type == FVM_VALUE_OBJECT ?
        fvm_styxrpc_array_snapshot(context, list->object, STYXRPC_CHANNEL_SLOTS, rows) : 0;
    if (!count)
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (i = 0; i < count; ++i)
    {
        FVM_Value fields[2];
        unsigned arity = rows[i].type == FVM_VALUE_OBJECT ?
            fvm_styxrpc_array_snapshot(context, rows[i].object, 2, fields) : 0;
        FVM_Result status;
        if (!arity || fields[0].type != FVM_VALUE_OBJECT)
        {
            return FVM_INVALID_ARGUMENT;
        }
        channels[i] = fvm_styxrpc_channel_lookup(context, fields[0].object);
        if (!channels[i] || !fvm_styxrpc_channels_share_bus(anchor, channels[i]))
        {
            return fvm_styxrpc_managed_fail(context, "Styx RPC select requires open channels on one bus");
        }
        status = fvm_gc_roots_add(roots, fields);
        if (status == FVM_OK && arity == 2)
        {
            status = fvm_gc_roots_add(roots, fields + 1);
            operands[i] = fields[1];
        }
        if (status != FVM_OK)
        {
            return status;
        }
        selection->guards[i].key = channels[i]->grant.key;
        selection->guards[i].receive = arity == 1;
    }
    selection->count = count;
    return FVM_OK;
}


/* EN: Outgoing CURRENT leaves have already been checked by the encoder. An incoming
 * CURRENT leaf may introduce a new key, which the selected receiver admits before decoding.
 * For external addresses only the exact staged PID grants are accepted by the wire policy.
 * RU: Исходящие поля CURRENT уже проверены кодировщиком. Входящее поле CURRENT может
 * выдать новый ключ, который получатель допускает перед декодированием.
 * Для отдельного адреса политика кадров допускает только точно подготовленный PID. */
static int allow(const void *state, const StyxRpcChannelRoute *route)
{
    return route->kind == STYXRPC_ROUTE_CURRENT || fvm_styxrpc_channel_send_allow(state, route);
}


/* [GC:] EN: Encode the send guards in one bounded arena and recheck all channels after getters.
 * Return a static error or NULL. The operands, children and receivers remain rooted by caller.
 * RU: Закодировать отправки в одном ограниченном буфере и повторно проверить каналы после геттеров.
 * Вернуть статическую ошибку либо NULL. Вызывающий удерживает операнды, потомков и получателей. */
static const char *pack(FVM_Context *context, FVM_GCRootScope *roots, StyxRpcSelection *selection,
    FVM_StyxChannelSend *send, FVM_StyxRemoteChannel **channels, const FVM_Value *operands,
    unsigned char *buffers)
{
    FVM_StyxRefs refs = {0};
    unsigned char *arena = buffers + 2U * FRAME;
    size_t i, used = 0;
    refs.graph = 1;
    refs.encode_channel = fvm_styxrpc_channel_encode;
    refs.channel_context = send;
    for (i = 0; i < selection->count; ++i)
    {
        StyxRpcGuard *guard = selection->guards + i;
        if (!guard->receive)
        {
            if (!fvm_styxrpc_channel_pack_refs(context, roots, &refs, channels[i]->grant.shape,
                operands + i, buffers, buffers + FRAME, guard) || guard->size > FRAME - used)
            {
                return "Styx RPC channel value does not fit the schema or selection frame";
            }
            memcpy(arena + used, guard->value, guard->size);
            guard->value = arena + used;
            used += guard->size;
        }
    }
    for (i = 0; i < selection->count; ++i)
    {
        if (!fvm_styxrpc_channels_share_bus(send->receiver, channels[i]))
        {
            return "Styx RPC select requires open channels on one bus";
        }
    }
    return NULL;
}


/* [GC:] EN: Report the native ordinal and received value as a rooted pair, or undefined for idle.
 * RU: Вернуть номер нативной ветки и принятое значение в массиве либо undefined при простое. */
static FVM_Result publish(FVM_Context *context, FVM_GCRootScope *roots, const StyxRpcSelection *selection,
    FVM_StyxRemoteChannel **channels, const StyxRpcChannelResult *result, FVM_Value *out)
{
    FVM_Value fields[2];
    FVM_Result status;
    if (!result->selected)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    fvm_value_number(fields, result->selected);
    fvm_value_undefined(fields + 1);
    if (selection->guards[result->selected - 1].receive &&
        !fvm_styxrpc_channel_unpack_remote(context, roots, channels[result->selected - 1], result, fields + 1))
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel result could not be decoded");
    }
    status = fvm_gc_roots_add(roots, fields + 1);
    return status == FVM_OK ? fvm_exec_literal_array(context, 2, fields, out) : status;
}


/* [GC:] EN: Execute one ordered guard list on self using the native selection mode in state.
 * Operands are borrowed during the owner call. Decode only the confirmed receiving branch.
 * RU: Выполнить на self один список веток в нативном режиме state.
 * Операнды заимствуются на время вызова владельца. Декодируется подтверждённая ветка приёма. */
static FVM_Result choose(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRemoteChannel *anchor = fvm_styxrpc_channel_lookup(context, self);
    FVM_GCRootScope *roots;
    FVM_StyxRemoteChannel *channels[STYXRPC_CHANNEL_SLOTS];
    FVM_Value operands[STYXRPC_CHANNEL_SLOTS];
    StyxRpcSelection selection = {0};
    StyxRpcChannelResult result;
    FVM_StyxChannelSend send = {0};
    StyxRpcChannelPolicy policy;
    FVM_Result status;
    unsigned char *buffers;
    const char *error;
    if (!anchor || argc != 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!anchor->set || (anchor->set->pid_mailbox && !styxrpc_channel_mailbox_active(anchor->set->pid_mailbox)))
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel is closed");
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    selection.mode = *(const StyxRpcChannelMode *)state;
    send.receiver = anchor;
    policy.allow = allow;
    policy.context = &send;
    status = snapshot(context, roots, args, anchor, &selection, channels, operands);
    if (status == FVM_OK)
    {
        buffers = malloc(SELECT_BUFFERS * FRAME);
        if (buffers)
        {
            error = pack(context, roots, &selection, &send, channels, operands, buffers);
            if (!error)
            {
                selection.policy = send.delegated.count ? &policy : NULL;
                error = fvm_styxrpc_channel_run(context, anchor, &selection, buffers + 3U * FRAME, &result);
            }
            status = error ? fvm_styxrpc_managed_fail(context, error) :
                publish(context, roots, &selection, channels, &result, out);
            free(buffers);
        }
        else
        {
            status = FVM_OUT_OF_MEMORY;
        }
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
FVM_Result fvm_styxrpc_channel_install_select(FVM_Context *context, FVM_Object *owner)
{
    FVM_Result status = fvm_object_define_method_rooted(context, owner, "select", choose, (void *)&blocking, NULL);
    return status == FVM_OK ?
        fvm_object_define_method_rooted(context, owner, "trySelect", choose, (void *)&immediate, NULL) : status;
}
