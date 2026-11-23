/* Copyright (c) 2026 bhgv
 * EN: Host synchronous positional graph exports in a Styx module directory.
 * RU: Хостовые синхронные позиционные экспорты графов в каталоге Styx.
 */
#ifndef STYXRPC_GRAPH_MODULE_H
#define STYXRPC_GRAPH_MODULE_H
#include "styxrpc/graph.h"
#include "styxrpc/managed_module.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StyxRpcGraphOutput
{
    const void *result;  /* Borrowed typed result children, empty for void. */
    size_t result_size;  /* Result byte extent. */
    const void *updates; /* Borrowed sparse positional update children. */
    size_t update_size;  /* Update byte extent. */
} StyxRpcGraphOutput;


/* EN: Execute one verified graph call; return OK or REMOTE_ERROR synchronously.
 * RU: Выполнить один проверенный вызов; синхронно вернуть OK или REMOTE_ERROR. */
typedef StyxRpcStatus (*StyxRpcGraphBegin)(void *context, const FVM_SRecord *arguments,
    StyxRpcGraphOutput *output);

typedef struct StyxRpcGraphExport
{
    const char *name;       /* Exact compiled export name. */
    uint32_t signature;     /* Compiled interface signature. */
    StyxRpcSchema schema;   /* Complete argument and result type lists. */
    StyxRpcGraphBegin begin; /* Owner callback, invoked once per accepted request. */
    void *context;          /* Borrowed owner state. */
} StyxRpcGraphExport;

typedef struct StyxRpcGraphModule
{
    const StyxRpcGraphExport *exports; /* Borrowed immutable descriptors. */
    size_t count;                    /* Number of functions. */
    StyxRpcEntry entries[STYXRPC_MAX_EXPORTS + 5]; /* Standard module nodes plus .ref-fields. */
    size_t profile_size;              /* Compiled interface byte count. */
    unsigned char profile[STYXRPC_MANAGED_PROFILE_BYTES]; /* Owned encoded interface. */
} StyxRpcGraphModule;


/* EN: Prepare a caller-owned module; borrow names and descriptors through server destruction.
 * RU: Подготовить модуль вызывающего; заимствовать имена и описатели до уничтожения сервера. */
StyxRpcStatus styxrpc_graph_module_init(StyxRpcGraphModule *module, const char *name,
    const StyxRpcGraphExport *exports, size_t count);

extern const StyxRpcTree styxrpc_graph_module_tree; /* Context is an initialized StyxRpcGraphModule. */

#ifdef __cplusplus
}
#endif
#endif
