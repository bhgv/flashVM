/* Copyright (c) 2026 bhgv
 * EN: Hosted function fids for a table of synchronous managed exports.
 * RU: Хостовые fid функций для таблицы синхронных управляемых экспортов.
 */
#ifndef STYXRPC_MANAGED_MODULE_H
#define STYXRPC_MANAGED_MODULE_H
#include "styxrpc/managed.h"
#include "styxrpc/serve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_MANAGED_PROFILE_BYTES (192U * STYXRPC_MAX_EXPORTS) /* Maximum compiled offer table. */

typedef struct StyxRpcManagedModule
{
    const StyxRpcManagedExport *exports; /* Borrowed immutable descriptors. */
    size_t count;                        /* Export descriptor count. */
    StyxRpcEntry entries[STYXRPC_MAX_EXPORTS + 4]; /* Root, mounted, module, profile, functions. */
    size_t profile_size;                 /* Length of the immutable interface. */
    unsigned char profile[STYXRPC_MANAGED_PROFILE_BYTES]; /* Owned interface bytes. */
} StyxRpcManagedModule;


/* EN: Prepare a synchronous managed module. Borrow name and exports through server destruction.
 * RU: Подготовить синхронный управляемый модуль. Заимствовать имя и экспорты до уничтожения сервера. */
StyxRpcStatus styxrpc_managed_module_init(StyxRpcManagedModule *module, const char *name,
    const StyxRpcManagedExport *exports, size_t count);

extern const StyxRpcTree styxrpc_managed_module_tree; /* Context is an initialized StyxRpcManagedModule. */

#ifdef __cplusplus
}
#endif
#endif
