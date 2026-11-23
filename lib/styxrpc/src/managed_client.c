/* Copyright (c) 2026 bhgv
 * EN: Hosted request and reply for the existing managed record profile.
 * RU: Хостовый запрос и ответ по действующему профилю управляемых записей.
 */
#include "styxrpc/managed_client.h"
#include "client_exchange.h"
#include <stdlib.h>


/* EN: Add the published caller PID as the first field of one complete request.
 * RU: Добавить опубликованный PID первым полем полного запроса. */
static unsigned char *scope_request(uint32_t pid, const void *request, size_t size, size_t *used)
{
    FVM_SRecord inner;
    unsigned char prefix[STYXRPC_FRAME_SIZE];
    size_t extent, prefix_size;
    if (!pid || fvm_srecord_read(request, size, STYXRPC_MANAGED_LIMIT, &inner, &extent) != FVM_S_OK ||
        extent != size || styxrpc_value_integer(STYXRPC_VALUE_BIG, pid, prefix, sizeof(prefix), &prefix_size) != STYXRPC_OK)
    {
        return NULL;
    }
    unsigned char *scoped = malloc(STYXRPC_MANAGED_LIMIT);
    if (scoped && fvm_srecord_write_pair(inner.tag, prefix, prefix_size, inner.payload, inner.size,
        scoped, STYXRPC_MANAGED_LIMIT, used) != FVM_S_OK)
    {
        free(scoped);
        scoped = NULL;
    }
    return scoped;
}


/* EN/RU: See managed_client.h. / См. managed_client.h. */
StyxRpcOutcome styxrpc_managed_invoke(const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcManagedReply *result)
{
    if (!target || !target->wait_ms || target->wait_ms > STYXRPC_SCALAR_WAIT_MAX_MS ||
        !reply || !capacity || capacity > STYXRPC_MANAGED_LIMIT || !result || size > STYXRPC_MANAGED_LIMIT)
    {
        return STYXRPC_CALL_NOT_SENT;
    }
    unsigned char *request = malloc(STYXRPC_MANAGED_LIMIT);
    size_t request_size;
    if (!request || styxrpc_managed_request(target->signature, &target->schema, arguments, size,
        request, STYXRPC_MANAGED_LIMIT, &request_size) != STYXRPC_OK)
    {
        free(request);
        return STYXRPC_CALL_NOT_SENT;
    }
    int scoped;
    StyxFid *fid = styxrpc_client_function(target->session, target->module, target->name,
        target->signature, &target->schema, &scoped);
    unsigned char *wrapped = NULL;
    size_t wire_size = request_size;
    StyxRpcOutcome outcome = STYXRPC_CALL_NOT_SENT;
    if (fid && (!scoped || (wrapped = scope_request(target->pid, request, request_size, &wire_size))))
    {
        size_t received;
        int sent = styxrpc_client_exchange(fid, wrapped ? wrapped : request, wire_size,
            target->wait_ms, reply, capacity, &received);
        StyxRpcStatus status = sent ? styxrpc_managed_reply(target->signature, &target->schema,
            request, request_size, reply, received, result) : STYXRPC_INVALID;
        outcome = status == STYXRPC_OK ? STYXRPC_CALL_OK :
            status == STYXRPC_REMOTE_ERROR ? STYXRPC_CALL_REMOTE_ERROR : STYXRPC_CALL_UNCERTAIN;
    }
    styxclientclunk(fid);
    free(wrapped);
    free(request);
    return outcome;
}
