/* Copyright (c) 2026 bhgv
 * EN: Check the callable surface of a mounted module without running any entry.
 * RU: Проверка входов смонтированного модуля без запуска колбэков.
 */
#ifndef STYXRPC_MODULE_CLIENT_H
#define STYXRPC_MODULE_CLIENT_H
#include "styxrpc/module.h"
#include "styxrpc/managed_client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StyxRpcModuleImport
{
    const char *name;        /* Required literal export name. */
    StyxRpcModuleKind kind; /* Native codec required for the function. */
    uint32_t signature;      /* Exact compiled signature. */
    StyxRpcSchema schema;   /* Exact resolved schema for shaped profiles. */
} StyxRpcModuleImport;


/* EN: Check all requirements on one held directory of the attached session.
 * The path is relative. A nonzero PID is required by any scoped requirement.
 * RU: Проверить все требования на одном удерживаемом каталоге смонтированной сессии.
 * Путь относительный. Для контекстной функции требуется ненулевой PID. */
StyxRpcStatus styxrpc_module_check(struct StyxClient *session, const char *module,
    const StyxRpcModuleImport *imports, size_t count, uint32_t pid);

#ifdef __cplusplus
}
#endif
#endif
