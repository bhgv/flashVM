/* Copyright (c) 2026 bhgv
 * EN: Hosted request and reply for the existing managed record profile.
 * RU: Хостовый запрос и ответ по действующему профилю управляемых записей.
 */
#include "styxrpc/managed_client.h"
#include "client_exchange.h"
#include <stdlib.h>


/* EN/RU: See managed_client.h. / См. managed_client.h. */
StyxRpcOutcome styxrpc_managed_invoke_at_until(StyxFid *directory, const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcManagedReply *result,
    StyxRpcCallStop stop, void *state)
{
    if (!target || !target->wait_ms || target->wait_ms > STYXRPC_SCALAR_WAIT_MAX_MS ||
        !reply || !capacity || capacity > STYXRPC_MANAGED_LIMIT || !result || size > STYXRPC_MANAGED_LIMIT)
    {
        return STYXRPC_CALL_NOT_SENT;
    }
    if (stop && stop(state))
    {
        return STYXRPC_CALL_CANCELLED;
    }
    unsigned char *request = malloc(STYXRPC_MANAGED_LIMIT);
    size_t request_size;
    if (!request || styxrpc_managed_request(target->signature, &target->schema, arguments, size,
        request, STYXRPC_MANAGED_LIMIT, &request_size) != STYXRPC_OK)
    {
        free(request);
        return STYXRPC_CALL_NOT_SENT;
    }
    size_t received;
    StyxRpcOutcome outcome = styxrpc_client_managed_exchange(directory, target, NULL, request, request_size,
        reply, capacity, &received, stop, state);
    if (outcome == STYXRPC_CALL_OK)
    {
        StyxRpcStatus status = styxrpc_managed_reply(target->signature, &target->schema,
            request, request_size, reply, received, result);
        outcome = status == STYXRPC_OK ? STYXRPC_CALL_OK :
            status == STYXRPC_REMOTE_ERROR ? STYXRPC_CALL_REMOTE_ERROR : STYXRPC_CALL_UNCERTAIN;
    }
    free(request);
    return outcome;
}


/* EN/RU: See managed_client.h. / См. managed_client.h. */
StyxRpcOutcome styxrpc_managed_invoke_at(StyxFid *directory, const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcManagedReply *result)
{
    return styxrpc_managed_invoke_at_until(directory, target, arguments, size, reply, capacity, result, NULL, NULL);
}


/* EN/RU: See managed_client.h. / См. managed_client.h. */
StyxRpcOutcome styxrpc_managed_invoke(const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcManagedReply *result)
{
    return styxrpc_managed_invoke_at(NULL, target, arguments, size, reply, capacity, result);
}
