/* Copyright (c) 2026 bhgv
 * EN: Verify a mounted interface and perform one scalar call without replay.
 * RU: Проверка смонтированного интерфейса и скалярный вызов без повтора.
 */
#include "styxrpc/scalar_client.h"
#include "client_exchange.h"

/* EN/RU: See scalar_client.h. / См. scalar_client.h. */
StyxRpcOutcome styxrpc_scalar_invoke_at_until(StyxFid *directory, StyxClient *session, const char *module, const char *name,
    uint32_t signature, int64_t argument, unsigned wait_ms, int64_t *result, StyxRpcCallStop stop, void *state)
{
    unsigned char request[STYXRPC_FRAME_SIZE];
    size_t size;
    if (!wait_ms || wait_ms > STYXRPC_SCALAR_WAIT_MAX_MS || !result ||
        styxrpc_request(signature, argument, request, sizeof(request), &size) != STYXRPC_OK)
    {
        return STYXRPC_CALL_NOT_SENT;
    }
    StyxRpcVfsWait wait = {stop, state, 0};
    if (styxrpc_vfs_control_poll(&wait))
    {
        return STYXRPC_CALL_CANCELLED;
    }
    int scoped;
    StyxFid *fid = styxrpc_client_function(directory, session, module, name, signature, NULL, NULL,
        &scoped, &wait);
    if (!fid)
    {
        return wait.stopped ? STYXRPC_CALL_CANCELLED : STYXRPC_CALL_NOT_SENT;
    }
    unsigned char reply[STYXRPC_FRAME_SIZE];
    size_t received;
    StyxRpcOutcome outcome = styxrpc_client_exchange(fid, request, size, wait_ms,
        reply, sizeof(reply), &received, stop ? styxrpc_vfs_control_poll : NULL, &wait);
    styxrpc_vfs_control_close(fid, &wait);
    if (outcome != STYXRPC_CALL_OK)
    {
        return outcome;
    }
    StyxRpcStatus status = styxrpc_reply(signature, reply, received, result);
    return status == STYXRPC_OK ? STYXRPC_CALL_OK :
        status == STYXRPC_REMOTE_ERROR ? STYXRPC_CALL_REMOTE_ERROR : STYXRPC_CALL_UNCERTAIN;
}


/* EN/RU: See scalar_client.h. / См. scalar_client.h. */
StyxRpcOutcome styxrpc_scalar_invoke_at(StyxFid *directory, StyxClient *session, const char *module, const char *name,
    uint32_t signature, int64_t argument, unsigned wait_ms, int64_t *result)
{
    return styxrpc_scalar_invoke_at_until(directory, session, module, name, signature, argument, wait_ms, result,
        NULL, NULL);
}


/* EN/RU: See scalar_client.h. / См. scalar_client.h. */
StyxRpcOutcome styxrpc_scalar_invoke(StyxClient *session, const char *module, const char *name,
    uint32_t signature, int64_t argument, unsigned wait_ms, int64_t *result)
{
    return styxrpc_scalar_invoke_at(NULL, session, module, name, signature, argument, wait_ms, result);
}
