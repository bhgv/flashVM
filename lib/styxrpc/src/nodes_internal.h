/* Copyright (c) 2026 bhgv
 * EN: Internal node registry and ID namespace in the mixed module slot.
 * RU: Внутренний реестр узлов и область ID в смешанном слоте модулей.
 */
#ifndef STYXRPC_NODES_INTERNAL_H
#define STYXRPC_NODES_INTERNAL_H
#include "styxrpc/nodes.h"

#define STYXRPC_NODE_ID_FIRST (UINT64_C(1) << 31) /* Generation zero, above all control IDs. */

typedef struct StyxRpcNodes StyxRpcNodes; /* Owner-thread callback registry for a movie. */

extern const StyxRpcTree styxrpc_nodes_tree; /* Context is one live StyxRpcNodes. */
extern const StyxRpcVfs styxrpc_node_exec_files; /* Context is a borrowed callback registry. */


/* EN: Resolve a validated relative path and invoke a live executable node once.
 * RU: Найти проверенный относительный путь и один раз вызвать исполняемый узел. */
int styxrpc_nodes_exec(void *registry, const char *path,
    void *bytes, size_t size, size_t *used);


/* EN: Internal callback variant and ownership; exactly one transfer function must be supplied.
 * RU: Внутренний вариант колбэка и владение. Указывается ровно одна функция обмена. */
typedef struct NodeRegistration
{
    StyxRpcNodeTransfer stream; /* Original offset-free callback, or NULL. */
    StyxRpcNodeTransferAt positioned; /* Fixed-length callback, or NULL. */
    void (*release)(void *context); /* Optional release after withdrawal. */
    void *context; /* Adopted state after successful registration. */
    uint64_t length; /* Byte extent for positioned I/O. */
} NodeRegistration;


/* EN: Adjoin path atomically, allocating a registry only when needed and assigning new IDs.
 * RU: Атомарно добавить путь, выделяя реестр по необходимости и выдавая новые ID. */
StyxRpcStatus styxrpc_nodes_insert(StyxRpcNodes **nodes, uint64_t *next, uint32_t *root_revision, const char *path,
    unsigned mode, const NodeRegistration *callbacks);


/* EN: Return whether a data callback is active on this registry.
 * RU: Сообщить, работает ли колбэк передачи данных в этом реестре. */
int styxrpc_nodes_busy(const StyxRpcNodes *nodes);


/* EN: Find the exact removable file. Zero means unavailable or inside a callback.
 * RU: Найти точный файл для отзыва. Ноль означает отказ или активный колбэк. */
uint64_t styxrpc_nodes_removable(StyxRpcNodes *nodes, const char *path);


/* EN: Remove this prevalidated ID after its opens retire, releasing its owned context.
 * RU: Отозвать проверенный ID после закрытия его открытий и освободить контекст. */
void styxrpc_nodes_remove(StyxRpcNodes *nodes, uint64_t id, uint32_t *root_revision);


/* EN: Drop a detached registry after all opens have been closed. NULL is valid.
 * RU: Освободить отсоединённый реестр после закрытия всех файлов. NULL допустим. */
void styxrpc_nodes_dispose(StyxRpcNodes *nodes);

#endif
