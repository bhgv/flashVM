/* Copyright (c) 2026 bhgv
 * EN: Verify a mounted interface and perform one scalar call without replay.
 * RU: Проверка смонтированного интерфейса и скалярный вызов без повтора.
 */
#include "styxrpc/scalar_client.h"
#include "client_exchange.h"

/* EN/RU: See scalar_client.h. / См. scalar_client.h. */
StyxRpcOutcome styxrpc_scalar_invoke(StyxClient *session, const char *module, const char *name,
    uint32_t signature, int64_t argument, unsigned wait_ms, int64_t *result)
{
    unsigned char request[STYXRPC_FRAME_SIZE];
    size_t size;
    if (!wait_ms || wait_ms > STYXRPC_SCALAR_WAIT_MAX_MS || !result ||
        styxrpc_request(signature, argument, request, sizeof(request), &size) != STYXRPC_OK)
    {
        return STYXRPC_CALL_NOT_SENT;
    }
    int scoped;
    StyxFid *fid = styxrpc_client_function(session, module, name, signature, NULL, NULL, &scoped);
    if (!fid)
    {
        return STYXRPC_CALL_NOT_SENT;
    }
    unsigned char reply[STYXRPC_FRAME_SIZE];
    size_t received;
    int transferred = styxrpc_client_exchange(fid, request, size, wait_ms, reply, sizeof(reply), &received);
    styxclientclunk(fid);
    StyxRpcStatus status = transferred ? styxrpc_reply(signature, reply, received, result) : STYXRPC_INVALID;
    return status == STYXRPC_OK ? STYXRPC_CALL_OK :
        status == STYXRPC_REMOTE_ERROR ? STYXRPC_CALL_REMOTE_ERROR : STYXRPC_CALL_UNCERTAIN;
}
