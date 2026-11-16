/* Copyright (c) 2026 bhgv
 * EN: Scalar native control-RPC outcomes, preserving acknowledgement versus cancellation.
 * RU: Scalar native control-RPC outcomes с различением acknowledgement и cancellation.
 */
#include "internal.h"


/* EN/RU: See internal.h; a refused local operation has no remote effects.
 * См. internal.h; локальный отказ не создаёт remote effects. */
void styxopclear(StyxTransfer *result)
{
    memset(result, 0, sizeof(*result));
    result->confirmed = 1;
}


/* EN/RU: See internal.h; snapshot before cleanup RPC can overwrite client scratch state.
 * См. internal.h; snapshot до cleanup RPC, который может изменить client scratch state. */
void styxopresult(StyxClient *client, int status, StyxTransfer *result)
{
    styxopclear(result);
    result->success = status == 1;
    result->cancelled = client->cancelled;
    result->uncertain = status < 0 && client->attempted;
}
