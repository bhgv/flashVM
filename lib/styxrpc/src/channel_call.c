/* Copyright (c) 2026 bhgv
 * EN: Scoped Dis channel profiles and call-local admission without VM types.
 * RU: Контекстные профили каналов Dis и допуск в пределах вызова без типов VM.
 */
#include "styxrpc/channel_call.h"
#include "call_record.h"
#include "call_scope.h"
#include "scalar_record.h"
#include "scalar_verify.h"
#include <string.h>


/* EN: Verify one scoped channel profile, with at least one channel in either direction.
 * RU: Проверить контекстный профиль хотя бы с одним каналом аргумента либо результата. */
static int checked(const StyxRpcSchema *schema, StyxCallShape *shape)
{
    if (!schema)
    {
        return 0;
    }
    memcpy(shape->arguments, schema->arguments, sizeof(shape->arguments));
    memcpy(shape->result, schema->result, sizeof(shape->result));
    return styxshapevalid(shape) && (strchr(shape->arguments, 'c') || strchr(shape->result, 'c'));
}


/* EN/RU: See channel_call.h. / См. channel_call.h. */
StyxRpcStatus styxrpc_channel_profile(const char *name, uint32_t signature, const StyxRpcSchema *schema,
    void *bytes, size_t capacity, size_t *used)
{
    StyxCallShape shape;
    unsigned char direct[STYX_SHAPED_OFFER_MAX];
    size_t size, parsed;
    FVM_SRecord offer;
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    return bytes && styxscalar_name_length(name) && *name != '.' && !strchr(name, '/') &&
        checked(schema, &shape) && styxshaperequest(signature, &shape) == STYX_MANAGED_REQUEST_TAG &&
        styxshapedoffer(name, signature, &shape, direct, sizeof(direct), &size) &&
        fvm_srecord_read(direct, size, sizeof(direct), &offer, &parsed) == FVM_S_OK &&
        fvm_srecord_write(STYX_SCOPED_RECORD_OFFER_TAG, offer.payload, offer.size,
            bytes, capacity, used) == FVM_S_OK ? STYXRPC_OK : STYXRPC_INVALID;
}


/* EN: Index a channel's shape, requiring aliases to agree on the exact element type.
 * RU: Запомнить форму канала и сверить точный тип элемента у каждого псевдонима. */
static int admit(const void *context, const char *type, uint32_t key)
{
    StyxRpcChannelCall *call = (StyxRpcChannelCall *)context;
    const char *end = styxshape_end(type);
    if (!end || *type != 'c')
    {
        return 0;
    }
    const size_t length = (size_t)(end - type) - 3;
    if (length + 3 > STYXRPC_MANAGED_SHAPE || memchr(type + 2, 'c', length) || memchr(type + 2, 'h', length))
    {
        return 0;
    }
    for (size_t i = 0; i < call->count; ++i)
    {
        const StyxRpcChannelGrant *old = &call->grants[i];
        if (old->key == key)
        {
            return strlen(old->shape) == length + 2 && !memcmp(old->shape + 1, type + 2, length);
        }
    }
    if (call->count == STYXRPC_CHANNEL_SLOTS)
    {
        return 0;
    }
    StyxRpcChannelGrant *grant = &call->grants[call->count++];
    grant->key = key;
    grant->shape[0] = '(';
    memcpy(grant->shape + 1, type + 2, length);
    grant->shape[length + 1] = ')';
    grant->shape[length + 2] = 0;
    return 1;
}


/* EN/RU: See channel_call.h. / См. channel_call.h. */
StyxRpcStatus styxrpc_channel_call(const StyxRpcSchema *schema, const void *bytes, size_t size,
    StyxRpcChannelCall *call)
{
    StyxCallShape shape;
    StyxRpcChannelCall decoded = {0};
    FVM_SRecord pid;
    size_t consumed, prefix;
    StyxScalarKind kind;
    int64_t process;
    if (!call || !bytes || size > STYXRPC_MANAGED_LIMIT || !checked(schema, &shape) ||
        fvm_srecord_read(bytes, size, STYXRPC_MANAGED_LIMIT, &decoded.arguments, &consumed) != FVM_S_OK ||
        consumed != size || !styxmanaged_graph_tag(decoded.arguments.tag) ||
        fvm_srecord_read(decoded.arguments.payload, decoded.arguments.size, STYX_SCALAR_RECORD_CAPACITY,
            &pid, &prefix) != FVM_S_OK ||
        !styxscalarread(STYXRPC_VALUE_BIG, decoded.arguments.payload, prefix, &kind, &process) ||
        kind != STYX_SCALAR_VALUE || process <= 0 || process > UINT32_MAX)
    {
        return STYXRPC_INVALID;
    }
    decoded.pid = (uint32_t)process;
    decoded.arguments.payload += prefix;
    decoded.arguments.size -= prefix;
    FVM_SRecord value = decoded.arguments;
    value.tag = STYX_MANAGED_GRAPH_TAG;
    if (!styxmanaged_channel_payload(shape.arguments, &value, admit, &decoded))
    {
        return STYXRPC_INVALID;
    }
    *call = decoded;
    return STYXRPC_OK;
}


/* EN/RU: See channel_call.h. / См. channel_call.h. */
StyxRpcStatus styxrpc_channel_call_reply(const StyxRpcSchema *schema, const StyxRpcChannelCall *call,
    const void *result, size_t result_size, const void *updates, size_t update_size,
    void *bytes, size_t capacity, size_t *used)
{
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    StyxCallShape shape;
    const StyxReplyField fields[] = {{result, result_size}, {updates, update_size}};
    return bytes && result && updates && call && call->pid && checked(schema, &shape) &&
        styxmanaged_graph_tag(call->arguments.tag) && styxmanaged_graph_reply(&shape, &call->arguments, fields) &&
        styxcall_reply_write(STYX_MANAGED_REPLY_TAG, result, result_size, updates, update_size, bytes,
            capacity > STYXRPC_MANAGED_LIMIT ? STYXRPC_MANAGED_LIMIT : capacity, used) ?
        STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See channel_call.h. / См. channel_call.h. */
StyxRpcStatus styxrpc_channel_export_value(const unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE],
    uint32_t key, int unbuffered, void *bytes, size_t capacity, size_t *used)
{
    static const unsigned char missing[STYXRPC_CHANNEL_TOKEN_SIZE] = {0};
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    if (!token || !memcmp(token, missing, sizeof(missing)) || !key || key > INT32_MAX ||
        !bytes || (unbuffered != 0 && unbuffered != 1))
    {
        return STYXRPC_INVALID;
    }
    uint32_t wire = key | (unbuffered ? UINT32_C(0x80000000) : 0);
    unsigned char identity[] = {wire >> 24, wire >> 16, wire >> 8, wire};
    return fvm_srecord_write_pair(STYX_MANAGED_EXPORTED_CHANNEL_TAG, token, sizeof(missing),
        identity, sizeof(identity), bytes, capacity, used) == FVM_S_OK ? STYXRPC_OK : STYXRPC_INVALID;
}
