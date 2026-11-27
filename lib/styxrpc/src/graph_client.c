/* Copyright (c) 2026 bhgv
 * EN: Exchange one graph request over the shared hosted transport.
 * RU: Обмен одним графовым запросом через общий хостовый транспорт.
 */
#include "styxrpc/graph_client.h"
#include "client_exchange.h"
#include <stdlib.h>


/* EN/RU: See graph_client.h. / См. graph_client.h. */
StyxRpcOutcome styxrpc_graph_invoke_at(StyxFid *directory, const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcGraphReply *result)
{
    if (!target || !target->wait_ms || target->wait_ms > STYXRPC_SCALAR_WAIT_MAX_MS || !reply ||
        !capacity || capacity > STYXRPC_MANAGED_LIMIT || !result || size > STYXRPC_MANAGED_LIMIT)
    {
        return STYXRPC_CALL_NOT_SENT;
    }
    unsigned char *request = malloc(STYXRPC_MANAGED_LIMIT);
    size_t request_size;
    if (!request || styxrpc_graph_request(&target->schema, arguments, size, request,
        STYXRPC_MANAGED_LIMIT, &request_size) != STYXRPC_OK)
    {
        free(request);
        return STYXRPC_CALL_NOT_SENT;
    }
    size_t received;
    StyxRpcOutcome outcome = styxrpc_client_managed_exchange(directory, target, STYXRPC_POSITIONAL_GRAPH,
        request, request_size, reply, capacity, &received);
    if (outcome == STYXRPC_CALL_OK)
    {
        StyxRpcStatus status = styxrpc_graph_read(&target->schema, request, request_size, reply, received, result);
        outcome = status == STYXRPC_OK ? STYXRPC_CALL_OK :
            status == STYXRPC_REMOTE_ERROR ? STYXRPC_CALL_REMOTE_ERROR : STYXRPC_CALL_UNCERTAIN;
    }
    free(request);
    return outcome;
}


/* EN/RU: See graph_client.h. / См. graph_client.h. */
StyxRpcOutcome styxrpc_graph_invoke(const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcGraphReply *result)
{
    return styxrpc_graph_invoke_at(NULL, target, arguments, size, reply, capacity, result);
}
