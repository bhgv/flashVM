/* Copyright (c) 2026 bhgv
 * EN: Hosted Styx dispatcher for a callback tree with virtual directories.
 * RU: Диспетчер Styx для дерева колбэков с виртуальными каталогами.
 */
#ifndef STYXRPC_SERVE_H
#define STYXRPC_SERVE_H
#include "styxrpc/vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RemotePhy; /* Owned delegated Styx stream consumed by create. */
typedef struct StyxRpcServer StyxRpcServer; /* Owner-thread dispatcher. */

#define STYXRPC_TREE_NAME 255U       /* Maximum name in bytes, excluding NUL. */
#define STYXRPC_TREE_CHILDREN 4096U  /* Maximum immediate children in this adapter. */


/* EN: Describe a stable Qid and the current revision of a directory.
 * RU: Описать устойчивый Qid и текущую ревизию каталога. */
typedef struct StyxRpcNode
{
    StyxRpcEntry entry; /* Copied entry; name borrows storage until the next callback. */
    uint64_t parent;    /* Logical parent ID; zero at root. */
    uint64_t length;    /* Current file extent in bytes; zero on directories. */
    uint32_t revision;  /* Change on directory membership or enumeration order change. */
} StyxRpcNode;


/* EN: Resolve names and IDs live; IDs never name a different node while serving.
 * RU: Разрешать имена и ID при запросе; ID не переиспользуется в работающем сервере. */
typedef struct StyxRpcTree
{
    StyxRpcVfs files; /* Enumerate by directory, then open and use a separate cookie per fid. */
    int (*stat)(void *context, uint64_t id, StyxRpcNode *node); /* Return 1 for a live node, 0 if absent. */
} StyxRpcTree;


/* EN: Take phy on every outcome. Borrow tree/context through destroy.
 * Root ID is zero. Access uses the other-class Unix mode bits for every peer.
 * Use an already authorized connection. Returns NULL if initialization fails.
 * RU: Принять phy при любом исходе. Заимствовать tree/context до destroy.
 * Корень имеет ID ноль. Для клиента действуют биты other режима Unix.
 * Использовать уже выданное соединение. При ошибке вернуть NULL. */
StyxRpcServer *styxrpc_server_create(struct RemotePhy *phy, const StyxRpcTree *tree, void *context);


/* EN: Wait for and process one server cycle; return 1 to continue, 0 on disconnect or error.
 * Only the owning thread calls step and destroy.
 * RU: Дождаться и обработать цикл сервера; вернуть 1 для продолжения, 0 при обрыве или ошибке.
 * step и destroy вызывает владелец диспетчера. */
int styxrpc_server_step(StyxRpcServer *server);


/* EN: Consume the server, closing all owned fid cookies including on link loss.
 * RU: Освободить сервер и закрыть все состояния fid, в том числе после обрыва. */
void styxrpc_server_destroy(StyxRpcServer *server);

/* EN: On-demand, owner-thread snapshot. No new wire requests or per-call counters.
 * RU: Снимок по запросу в потоке владельца. Без новых запросов и счётчиков на вызов. */
typedef struct StyxRpcServerState
{
    size_t clients;      /* Clients currently attached to the native session. */
    size_t reply_bytes;  /* Unsent bytes in the existing Styx transport queue. */
    unsigned free_slots; /* Available slots in the existing reply queue. */
} StyxRpcServerState;


/* EN: Fill state from the existing native session, or return zero for invalid input.
 * Call on the owner thread between steps or from an owner callback.
 * RU: Заполнить state из текущей нативной сессии либо вернуть ноль при ошибке аргумента.
 * Вызывать в потоке владельца между шагами или из колбэка владельца. */
int styxrpc_server_state(StyxRpcServer *server, StyxRpcServerState *state);

#ifdef __cplusplus
}
#endif
#endif
