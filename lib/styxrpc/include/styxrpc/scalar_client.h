/* Copyright (c) 2026 bhgv
 * EN: Synchronous hosted calls to compiled scalar Styx exports.
 * RU: Синхронные вызовы компилированных скалярных экспортов Styx.
 */
#ifndef STYXRPC_SCALAR_CLIENT_H
#define STYXRPC_SCALAR_CLIENT_H
#include "styxrpc/rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_SCALAR_WAIT_MAX_MS 60000U /* Maximum pending reply polling budget. */

struct StyxClient; /* Borrowed mounted Styx session. */

typedef enum StyxRpcOutcome
{
    STYXRPC_CALL_OK = 0,             /* A checked scalar result was received. */
    STYXRPC_CALL_REMOTE_ERROR = 1,   /* The export replied with an application error. */
    STYXRPC_CALL_NOT_SENT = 2,       /* Verification/open failed before any request write. */
    STYXRPC_CALL_UNCERTAIN = 3       /* A write was attempted without a checked outcome. */
} StyxRpcOutcome;


/* EN: Verify the named compiled signature in the module's .interface, then call
 * once on a fresh fid. Borrow all inputs; result changes only on success.
 * RU: Сверить имя и сигнатуру по .interface модуля и вызвать один раз на свежем
 * fid. Аргументы заимствуются; result меняется только при успехе. */
StyxRpcOutcome styxrpc_scalar_invoke(struct StyxClient *session, const char *module,
    const char *name, uint32_t signature, int64_t argument, unsigned wait_ms, int64_t *result);

#ifdef __cplusplus
}
#endif
#endif
