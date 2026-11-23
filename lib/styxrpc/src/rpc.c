/* Copyright (c) 2026 bhgv
 * EN: Register scalar callbacks against the original Dis profile and frame codec.
 * RU: Связь скалярных колбэков с исходным профилем и кодеком Dis.
 */
#include "styxrpc/rpc.h"
#include "call_record.h"
#include "scalar_record.h"
#include "scalar_verify.h"
#include <string.h>


/* EN: Select the established scalar request tag by compiled signature.
 * RU: Выбрать действующий тег скалярного запроса по сигнатуре. */
static unsigned long scalar_tag(uint32_t signature)
{
    switch (signature)
    {
    case STYXRPC_INT_SIGNATURE:
        return STYX_FUNCTION_REQUEST_TAG;
    case STYXRPC_BIG_SIGNATURE:
        return STYX_FUNCTION_BIG_TAG;
    default:
        return 0;
    }
}


/* EN: Accept a signed value only within its exact compiled width.
 * RU: Принять знаковое значение только в пределах его разрядности. */
static int in_range(uint32_t signature, int64_t value)
{
    return signature != STYXRPC_INT_SIGNATURE || (value >= INT32_MIN && value <= INT32_MAX);
}


/* EN/RU: See rpc.h. / См. rpc.h. */
StyxRpcStatus styxrpc_request(uint32_t signature, int64_t argument, void *bytes,
    size_t capacity, size_t *size)
{
    unsigned long tag = scalar_tag(signature);
    if (!size)
    {
        return STYXRPC_INVALID;
    }
    *size = 0;
    return tag && bytes && in_range(signature, argument) &&
        styxscalarwrite(tag, STYX_SCALAR_CALL, argument, bytes, capacity, size) ?
        STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See rpc.h. / См. rpc.h. */
StyxRpcStatus styxrpc_reply(uint32_t signature, const void *bytes, size_t size, int64_t *value)
{
    StyxScalarKind kind;
    int64_t reply;
    if (!value || !scalar_tag(signature) ||
        !styxscalarread(STYX_FUNCTION_REPLY_TAG, bytes, size, &kind, &reply))
    {
        return STYXRPC_INVALID;
    }
    if (kind == STYX_SCALAR_ERROR)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    if (kind != STYX_SCALAR_VALUE || !in_range(signature, reply))
    {
        return STYXRPC_INVALID;
    }
    *value = reply;
    return STYXRPC_OK;
}


/* EN: Return the wire tag for a valid borrowed scalar export, or zero on refusal.
 * RU: Вернуть тег проверенного скалярного экспорта либо ноль при отказе. */
static unsigned long request_tag(const StyxRpcExport *exported)
{
    if (!exported || !exported->invoke || !styxscalar_name_length(exported->name) ||
        strchr(exported->name, '/') || exported->name[0] == '.')
    {
        return 0;
    }
    return scalar_tag(exported->signature);
}


/* EN/RU: See rpc.h. / См. rpc.h. */
StyxRpcStatus styxrpc_profile(const StyxRpcExport *exports, size_t count,
    void *bytes, size_t capacity, size_t *size)
{
    if (!size)
    {
        return STYXRPC_INVALID;
    }
    *size = 0;
    if (!exports || !count || count > STYXRPC_MAX_EXPORTS || !bytes)
    {
        return STYXRPC_INVALID;
    }
    size_t used = 0;
    for (size_t i = 0; i < count; i++)
    {
        if (!request_tag(exports + i))
        {
            return STYXRPC_INVALID;
        }
        for (size_t j = 0; j < i; j++)
        {
            if (!strcmp(exports[i].name, exports[j].name))
            {
                return STYXRPC_INVALID;
            }
        }
        size_t wrote;
        if (!styxloadedoffer(exports[i].name, exports[i].signature,
                (unsigned char *)bytes + used, capacity - used, &wrote))
        {
            return STYXRPC_INVALID;
        }
        used += wrote;
    }
    *size = used;
    return STYXRPC_OK;
}


/* EN/RU: See rpc.h. / См. rpc.h. */
StyxRpcStatus styxrpc_open(StyxRpcCall *call, const StyxRpcExport *exported)
{
    if (!call || !request_tag(exported))
    {
        return STYXRPC_INVALID;
    }
    *call = (StyxRpcCall){.exported = exported};
    return STYXRPC_OK;
}


/* EN: Run the borrowed callback once after all wire and signed-width checks.
 * RU: Запустить заимствованный колбэк один раз после проверки кадра и разрядности. */
static StyxRpcStatus finish(StyxRpcCall *call)
{
    const StyxRpcExport *exported = call->exported;
    StyxScalarKind kind;
    int64_t argument, result = 0;
    if (!styxscalarread(request_tag(exported), call->request, call->staged, &kind, &argument) ||
        kind != STYX_SCALAR_CALL || !in_range(exported->signature, argument))
    {
        call->refused = 1;
        return STYXRPC_INVALID;
    }
    int success = exported->invoke(exported->context, argument, &result) == 1 &&
        in_range(exported->signature, result);
    if (!styxscalarwrite(STYX_FUNCTION_REPLY_TAG, success ? STYX_SCALAR_VALUE : STYX_SCALAR_ERROR,
            success ? result : 0, call->reply, sizeof(call->reply), &call->ready))
    {
        call->refused = 1;
        return STYXRPC_INVALID;
    }
    return STYXRPC_OK;
}


/* EN/RU: See rpc.h. / См. rpc.h. */
StyxRpcStatus styxrpc_write(StyxRpcCall *call, size_t offset, const void *bytes, size_t size)
{
    if (!call || !call->exported || call->refused || call->ready || !bytes || !size ||
        offset != call->staged || size > sizeof(call->request) - call->staged)
    {
        return STYXRPC_INVALID;
    }
    memcpy(call->request + call->staged, bytes, size);
    call->staged += size;
    FVM_SRecord record;
    size_t used;
    FVM_SResult parsed = fvm_srecord_read(call->request, call->staged, sizeof(call->request), &record, &used);
    if (parsed == FVM_S_MORE && call->staged < sizeof(call->request))
    {
        return STYXRPC_MORE;
    }
    if (parsed != FVM_S_OK || used != call->staged)
    {
        call->refused = 1;
        return STYXRPC_INVALID;
    }
    return finish(call);
}


/* EN/RU: See rpc.h. / См. rpc.h. */
StyxRpcStatus styxrpc_read(const StyxRpcCall *call, size_t offset, void *bytes,
    size_t capacity, size_t *size)
{
    if (!size)
    {
        return STYXRPC_INVALID;
    }
    *size = 0;
    if (!call || !call->exported || call->refused || (!bytes && capacity))
    {
        return STYXRPC_INVALID;
    }
    if (!call->ready)
    {
        return STYXRPC_MORE;
    }
    if (offset < call->ready && capacity)
    {
        size_t available = call->ready - offset;
        *size = available < capacity ? available : capacity;
        memcpy(bytes, call->reply + offset, *size);
    }
    return STYXRPC_OK;
}
