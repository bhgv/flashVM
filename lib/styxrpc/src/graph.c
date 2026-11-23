/* Copyright (c) 2026 bhgv
 * EN: Reuse DisVM's complete positional graph validation without heap references.
 * RU: Переиспользование полной проверки графов DisVM без ссылок на кучу.
 */
#include "styxrpc/graph.h"
#include "call_record.h"
#include "scalar_record.h"
#include "scalar_verify.h"
#include <string.h>


/* EN: Copy the bounded schema and keep channel grants outside this pure codec.
 * RU: Скопировать схему, оставив полномочия каналов за границей чистого кодека. */
static int checked_shape(const StyxRpcSchema *schema, StyxCallShape *shape)
{
    if (!schema)
    {
        return 0;
    }
    memcpy(shape->arguments, schema->arguments, sizeof(shape->arguments));
    memcpy(shape->result, schema->result, sizeof(shape->result));
    return styxshapevalid(shape) && !strchr(shape->arguments, 'c') && !strchr(shape->result, 'c');
}


/* EN/RU: See graph.h. / См. graph.h. */
StyxRpcStatus styxrpc_graph_profile(const char *name, uint32_t signature, const StyxRpcSchema *schema,
    void *bytes, size_t capacity, size_t *used)
{
    StyxCallShape shape;
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    return bytes && styxscalar_name_length(name) && name[0] != '.' && !strchr(name, '/') &&
        checked_shape(schema, &shape) && styxshaperequest(signature, &shape) == STYX_MANAGED_REQUEST_TAG &&
        (strchr(shape.arguments, 'h') || strchr(shape.result, 'h') || styxmanaged_graph_required(shape.arguments)) &&
        styxshapedoffer(name, signature, &shape, bytes, capacity, used) ? STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See graph.h. / См. graph.h. */
StyxRpcStatus styxrpc_graph_request(const StyxRpcSchema *schema, const void *payload,
    size_t size, void *bytes, size_t capacity, size_t *used)
{
    StyxCallShape shape;
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    return bytes && (payload || !size) && checked_shape(schema, &shape) &&
        styxmanaged_list_write(STYXRPC_GRAPH_REQUEST_TAG, shape.arguments, payload, size, bytes, capacity, used) ?
        STYXRPC_OK : STYXRPC_INVALID;
}


/* EN: Check the original positional request before resolving update identities.
 * RU: Проверить исходный позиционный запрос до разрешения ID изменений. */
static int original(const StyxRpcSchema *schema, const void *bytes, size_t size,
    StyxCallShape *shape, FVM_SRecord *arguments)
{
    return bytes && checked_shape(schema, shape) &&
        styxmanaged_list_read(STYXRPC_GRAPH_REQUEST_TAG, shape->arguments, bytes, size, arguments);
}


/* EN/RU: See graph.h. / См. graph.h. */
StyxRpcStatus styxrpc_graph_arguments(const StyxRpcSchema *schema, const void *request, size_t request_size,
    FVM_SRecord *arguments)
{
    StyxCallShape shape;
    return arguments && original(schema, request, request_size, &shape, arguments) ?
        STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See graph.h. / См. graph.h. */
StyxRpcStatus styxrpc_graph_reply(const StyxRpcSchema *schema, const void *request, size_t request_size,
    const void *result, size_t result_size, const void *updates, size_t update_size,
    void *bytes, size_t capacity, size_t *used)
{
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    StyxCallShape shape;
    FVM_SRecord arguments;
    const StyxReplyField fields[] = {{result, result_size}, {updates, update_size}};
    return bytes && result && updates && original(schema, request, request_size, &shape, &arguments) &&
        styxmanaged_graph_reply(&shape, &arguments, fields) &&
        styxcall_reply_write(STYX_MANAGED_REPLY_TAG, result, result_size, updates, update_size, bytes,
            capacity > STYXRPC_MANAGED_LIMIT ? STYXRPC_MANAGED_LIMIT : capacity, used) ?
        STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See graph.h. / См. graph.h. */
StyxRpcStatus styxrpc_graph_read(const StyxRpcSchema *schema, const void *request, size_t request_size,
    const void *reply, size_t reply_size, StyxRpcGraphReply *decoded)
{
    StyxCallShape shape;
    FVM_SRecord arguments;
    if (!decoded || !reply || reply_size > STYXRPC_MANAGED_LIMIT || !original(schema, request, request_size, &shape, &arguments))
    {
        return STYXRPC_INVALID;
    }
    StyxScalarKind kind;
    int64_t code;
    if (styxscalarread(STYX_FUNCTION_REPLY_TAG, reply, reply_size, &kind, &code) && kind == STYX_SCALAR_ERROR)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    StyxReplyField fields[2];
    StyxRpcGraphReply result;
    size_t extent;
    if (!styxcall_reply_read(STYX_MANAGED_REPLY_TAG, reply, reply_size, fields) ||
        !styxmanaged_graph_reply(&shape, &arguments, fields) ||
        fvm_srecord_read(fields[0].bytes, fields[0].size, STYXRPC_MANAGED_LIMIT, &result.result, &extent) != FVM_S_OK ||
        fvm_srecord_read(fields[1].bytes, fields[1].size, STYXRPC_MANAGED_LIMIT, &result.updates, &extent) != FVM_S_OK)
    {
        return STYXRPC_INVALID;
    }
    *decoded = result;
    return STYXRPC_OK;
}
