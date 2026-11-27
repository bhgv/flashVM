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
struct StyxFid;    /* Borrowed exact module directory for a pinned call. */

typedef enum StyxRpcOutcome
{
    STYXRPC_CALL_OK = 0,             /* A checked scalar result was received. */
    STYXRPC_CALL_REMOTE_ERROR = 1,   /* The export replied with an application error. */
    STYXRPC_CALL_NOT_SENT = 2,       /* Verification/open failed before any request write. */
    STYXRPC_CALL_UNCERTAIN = 3,      /* A write was attempted without a checked outcome. */
    STYXRPC_CALL_CANCELLED = 4,      /* The stop hook fired before any request write. */
    STYXRPC_CALL_CANCELLED_UNCERTAIN = 5 /* The hook fired after a write; peer effects may exist. */
} StyxRpcOutcome;

/* EN: Return nonzero to stop a call on the calling thread.
 * RU: Вернуть ненулевое значение для остановки вызова в вызывающем потоке. */
typedef int (*StyxRpcCallStop)(void *state);


/* EN: Verify the named compiled signature in the module's .interface, then call
 * once on a fresh fid. Borrow all inputs; result changes only on success.
 * RU: Сверить имя и сигнатуру по .interface модуля и вызвать один раз на свежем
 * fid. Аргументы заимствуются; result меняется только при успехе. */
StyxRpcOutcome styxrpc_scalar_invoke(struct StyxClient *session, const char *module,
    const char *name, uint32_t signature, int64_t argument, unsigned wait_ms, int64_t *result);


/* EN: Use the borrowed unopened directory when supplied, ignoring session/module.
 * RU: Использовать переданный неоткрытый каталог, игнорируя session/module. */
StyxRpcOutcome styxrpc_scalar_invoke_at(struct StyxFid *directory, struct StyxClient *session,
    const char *module, const char *name, uint32_t signature, int64_t argument, unsigned wait_ms, int64_t *result);


/* EN: Like invoke_at, checking optional stop(state) before I/O and between frame
 * transfers. The per-call fid closes on cancellation. Result changes only on success.
 * RU: Как invoke_at с необязательным stop(state) перед I/O и между фрагментами
 * кадра. При отмене закрывается fid вызова. Результат меняется только при успехе. */
StyxRpcOutcome styxrpc_scalar_invoke_at_until(struct StyxFid *directory, struct StyxClient *session,
    const char *module, const char *name, uint32_t signature, int64_t argument, unsigned wait_ms, int64_t *result,
    StyxRpcCallStop stop, void *state);

#ifdef __cplusplus
}
#endif
#endif
