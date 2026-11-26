/* Copyright (c) 2026 bhgv
 * EN: Internal node registry and ID namespace in the mixed module slot.
 * RU: Внутренний реестр узлов и область ID в смешанном слоте модулей.
 */
#ifndef STYXRPC_NODES_INTERNAL_H
#define STYXRPC_NODES_INTERNAL_H
#include "styxrpc/nodes.h"

#define STYXRPC_NODE_ID_FIRST (UINT64_C(1) << 31) /* Generation zero, above all control IDs. */

typedef struct StyxRpcNodes StyxRpcNodes; /* Owner-thread append-only registry for a movie. */

extern const StyxRpcTree styxrpc_nodes_tree; /* Context is one live StyxRpcNodes. */
extern const StyxRpcVfs styxrpc_node_exec_files; /* Context is a borrowed callback registry. */


/* EN: Resolve a counted relative path and invoke a live executable node once.
 * RU: Найти относительный путь с длиной и один раз вызвать исполняемый узел. */
int styxrpc_nodes_exec(StyxRpcNodes *nodes, const char *path, size_t length,
    void *bytes, size_t size, size_t *used);


/* EN: Adjoin path atomically, allocating a registry only when needed and assigning new IDs.
 * RU: Атомарно добавить путь, выделяя реестр по необходимости и выдавая новые ID. */
StyxRpcStatus styxrpc_nodes_insert(StyxRpcNodes **nodes, uint64_t *next, const char *path,
    unsigned mode, const StyxRpcNodeCallbacks *callbacks);


/* EN: Drop a detached registry after all opens have been closed. NULL is valid.
 * RU: Освободить отсоединённый реестр после закрытия всех файлов. NULL допустим. */
void styxrpc_nodes_dispose(StyxRpcNodes *nodes);

#endif
