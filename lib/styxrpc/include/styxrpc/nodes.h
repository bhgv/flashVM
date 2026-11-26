/* Copyright (c) 2026 bhgv
 * EN: Callback files published beside a paired session's playback controls.
 * RU: Файлы с колбэками рядом с управлением воспроизведением в парной сессии.
 */
#ifndef STYXRPC_NODES_H
#define STYXRPC_NODES_H
#include "styxrpc/module_slot.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_NODE_PATH_MAX 1024U /* Path bytes, excluding the optional leading slash and NUL. */
#define STYXRPC_NODE_LIMIT 128U /* File and implicit directory identities per active image. */

typedef int (*StyxRpcNodeTransfer)(void *context, unsigned access, void *bytes,
    size_t size, size_t *used); /* Borrowed request bytes; 1 validates used, 0 rejects the request. */

typedef struct StyxRpcNodeCallbacks
{
    StyxRpcNodeTransfer transfer; /* Required owner-thread read/write callback. */
    void (*release)(void *context); /* Optional cleanup after withdrawal. */
    void *context; /* Adopted on successful publication when release is supplied. */
} StyxRpcNodeCallbacks;


/* EN: Publish a new file and missing parent directories atomically. mode is 1..7
 * (Unix r/w/x mask). Callbacks run on the slot owner. Invalid leaves context
 * owned by the caller. The x bit is metadata reserved for execution.
 * RU: Атомарно опубликовать файл и отсутствующие родительские каталоги.
 * mode от 1 до 7 (маска Unix r/w/x). Колбэки работают у владельца slot.
 * При отказе context остаётся у вызывающего. Бит x зарезервирован для исполнения. */
StyxRpcStatus styxrpc_module_slot_new_node(StyxRpcModuleSlot *slot, const char *path,
    unsigned mode, const StyxRpcNodeCallbacks *callbacks);


/* EN: Withdraw every callback node, invalidate opens and release callback contexts.
 * Call on the slot owner outside callbacks. NULL is harmless.
 * RU: Отозвать все узлы, инвалидировать открытые файлы и освободить контексты.
 * Вызывать у владельца slot вне колбэков. NULL допустим. */
void styxrpc_module_slot_clear_nodes(StyxRpcModuleSlot *slot);

#ifdef __cplusplus
}
#endif
#endif
