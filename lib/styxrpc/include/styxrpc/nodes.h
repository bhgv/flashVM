/* Copyright (c) 2026 bhgv
 * EN: Callback files published beside a paired session's playback controls.
 * RU: Файлы с колбэками рядом с управлением воспроизведением в парной сессии.
 */
#ifndef STYXRPC_NODES_H
#define STYXRPC_NODES_H
#include "styxrpc/module_slot.h"
#include "styxrpc/vfs_client.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_NODE_PATH_MAX 1024U /* Path bytes, excluding the optional leading slash and NUL. */
#define STYXRPC_NODE_LIMIT 128U /* File and implicit directory identities per active image. */
#define STYXRPC_NODE_EXEC_SERVICE ".exec" /* Hidden attach-root endpoint for executable callbacks. */
#define STYXRPC_NODE_EXEC_OWNER ".execctl" /* Local publication control; never export to a peer. */
#define STYXRPC_NODE_EXEC_QID UINT64_C(3) /* Exact service identity on the callback tree. */
#define STYXRPC_NODE_EXECUTE 4U /* Callback access selector for in-place invocation. */
#define STYXRPC_NODE_OFFSET_MAX UINT64_C(9007199254740991) /* Exact positional byte range shared with ActionScript. */
#define STYXRPC_NODE_BUFFER_MAX 65536U /* Maximum execute argument and result capacity in bytes. */

typedef int (*StyxRpcNodeTransfer)(void *context, unsigned access, void *bytes,
    size_t size, size_t *used); /* Borrowed request bytes; 1 validates used, 0 rejects the request. */

typedef struct StyxRpcNodeCallbacks
{
    StyxRpcNodeTransfer transfer; /* Owner-thread read, write or in-place execute callback. */
    void (*release)(void *context); /* Optional cleanup after withdrawal. */
    void *context; /* Adopted on successful publication when release is supplied. */
} StyxRpcNodeCallbacks;


/* EN: Handle one positioned fragment. Offset is zero for execute.
 * RU: Обработать один фрагмент со смещением. Для исполнения смещение равно нулю. */
typedef int (*StyxRpcNodeTransferAt)(void *context, unsigned access, uint64_t offset,
    void *bytes, size_t size, size_t *used);


/* EN: Own a positioned callback and its state after successful publication.
 * RU: Владеть колбэком со смещением и его состоянием после успешной публикации. */
typedef struct StyxRpcPositionedNodeCallbacks
{
    StyxRpcNodeTransferAt transfer; /* Owner-thread callback for both data operations and execute. */
    void (*release)(void *context); /* Optional context cleanup at withdrawal. */
    void *context; /* Consumed on successful registration if release is provided. */
} StyxRpcPositionedNodeCallbacks;


/* EN: Publish a new file and missing parent directories atomically. mode is 1..7
 * (Unix r/w/x mask). Callbacks run on the slot owner. Invalid leaves context
 * owned by the caller. The x bit permits STYXRPC_NODE_EXECUTE on a mutable
 * argument buffer of size bytes; used selects the returned prefix.
 * RU: Атомарно опубликовать файл и отсутствующие родительские каталоги.
 * mode от 1 до 7 (маска Unix r/w/x). Колбэки работают у владельца slot.
 * При отказе context остаётся у вызывающего. Бит x разрешает STYXRPC_NODE_EXECUTE
 * с изменяемым буфером size байт; used выбирает возвращаемый префикс. */
StyxRpcStatus styxrpc_module_slot_new_node(StyxRpcModuleSlot *slot, const char *path,
    unsigned mode, const StyxRpcNodeCallbacks *callbacks);


/* EN: Publish a callback file with fixed byte length from zero to STYXRPC_NODE_OFFSET_MAX.
 * Uses the same path, mode, thread and ownership rules as styxrpc_module_slot_new_node.
 * Reads clip to EOF; writes extending past EOF are refused before dispatch.
 * Execute uses the full request and offset zero.
 * RU: Опубликовать файл фиксированной длины от нуля до STYXRPC_NODE_OFFSET_MAX.
 * Правила пути, прав, потока и владения как у styxrpc_module_slot_new_node.
 * Чтение ограничено EOF; запись за границу отклоняется до колбэка.
 * Исполнение получает весь запрос и нулевое смещение. */
StyxRpcStatus styxrpc_module_slot_new_node_at(StyxRpcModuleSlot *slot, const char *path,
    unsigned mode, uint64_t length, const StyxRpcPositionedNodeCallbacks *callbacks);


/* EN: Withdraw every callback node, invalidate opens and release callback contexts.
 * Call on the slot owner outside callbacks. NULL is harmless.
 * RU: Отозвать все узлы, инвалидировать открытые файлы и освободить контексты.
 * Вызывать у владельца slot вне колбэков. NULL допустим. */
void styxrpc_module_slot_clear_nodes(StyxRpcModuleSlot *slot);


/* EN: Call path relative to the attached session root. Borrow size writable
 * bytes and replace their prefix with the confirmed reply. Set used to zero on
 * failure. The execution is synchronous and is never replayed by the client.
 * RU: Вызвать path от корня session. Заимствовать size изменяемых байт, вернуть
 * подтверждённый результат в их начале. При сбое used равен нулю.
 * Вызов синхронный, автоматического повтора нет. */
StyxRpcIoOutcome styxrpc_node_exec(struct StyxClient *session, const char *path,
    void *bytes, size_t size, size_t *used);

#ifdef __cplusplus
}
#endif
#endif
