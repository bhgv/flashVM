/* Copyright (c) 2026 bhgv
 * EN: Publish scalar descriptors as a loadable module over a hosted Styx tree.
 * RU: Публикация скалярных описателей как загружаемого модуля в дереве Styx.
 */
#ifndef STYXRPC_SCALAR_MODULE_H
#define STYXRPC_SCALAR_MODULE_H
#include "styxrpc/serve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_SCALAR_PROFILE_BYTES (64U * STYXRPC_MAX_EXPORTS) /* Bounded compiled scalar offers. */

typedef struct StyxRpcScalarModule
{
    const StyxRpcExport *exports; /* Borrowed immutable descriptors through server destruction. */
    size_t count;                 /* Number of exports and entries in use. */
    StyxRpcEntry entries[STYXRPC_MAX_EXPORTS + 4]; /* Root, mounted, module, profile, functions. */
    size_t profile_size;          /* Extent of the immutable interface file. */
    unsigned char profile[STYXRPC_SCALAR_PROFILE_BYTES]; /* Owned interface bytes. */
} StyxRpcScalarModule;


/* EN: Prepare caller-owned module for the scalar tree. Borrow name and exports
 * until the last server using module has been destroyed. Return INVALID on refusal.
 * RU: Подготовить module для скалярного дерева. Заимствовать name и exports
 * до уничтожения последнего сервера модуля. При отказе вернуть INVALID. */
StyxRpcStatus styxrpc_scalar_module_init(StyxRpcScalarModule *module, const char *name,
    const StyxRpcExport *exports, size_t count);

extern const StyxRpcTree styxrpc_scalar_module_tree; /* Context is an initialized StyxRpcScalarModule. */

#ifdef __cplusplus
}
#endif
#endif
