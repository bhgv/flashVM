/* Copyright (c) 2026 bhgv
 * EN: Hosted managed RPC for both ordinary and PID-scoped Dis exports.
 * RU: Хостовый управляемый RPC для обычных экспортов Dis и экспортов с PID.
 */
#ifndef STYXRPC_MANAGED_CLIENT_H
#define STYXRPC_MANAGED_CLIENT_H
#include "styxrpc/managed.h"
#include "styxrpc/scalar_client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StyxRpcManagedTarget
{
    struct StyxClient *session; /* Borrowed mounted session. */
    const char *module;         /* Module path relative to the session root. */
    const char *name;           /* Export component. */
    uint32_t signature;        /* Exact compiled signature. */
    StyxRpcSchema schema;      /* Exact compiled argument/result shapes. */
    uint32_t pid;              /* Published reverse caller, required for scoped offers. */
    unsigned wait_ms;          /* Pending reply polling budget. */
} StyxRpcManagedTarget;


/* EN: Encode borrowed argument children, call once and validate the complete reply.
 * On success result borrows bytes from the supplied reply buffer. PID-scoped calls
 * require the caller to hold the matching reverse publication through return.
 * RU: Закодировать аргументы, выполнить один вызов и проверить полный ответ.
 * При успехе result заимствует память reply. Для вызова с PID вызывающий
 * удерживает соответствующую встречную публикацию до возврата. */
StyxRpcOutcome styxrpc_managed_invoke(const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcManagedReply *result);


/* EN: Use a borrowed unopened module directory instead of target's path when supplied.
 * RU: Использовать переданный неоткрытый каталог вместо пути target. */
StyxRpcOutcome styxrpc_managed_invoke_at(struct StyxFid *directory, const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcManagedReply *result);

#ifdef __cplusplus
}
#endif
#endif
