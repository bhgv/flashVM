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
typedef void (*StyxRpcAttachNotice)(void *state, const char *name); /* Borrowed Tattach name on the owner. */

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


/* EN: Optional ordinary tree mutations. Create publishes a fresh ID and
 * revision and transfers the opened write cookie for a file. cookie is NULL
 * for directory creation. Remove unlinks an ID without closing other cookies.
 * Both callbacks return 1 on success or 0 with no change. Do not dispatch
 * recursively. See the function reference for the common DisVM profile.
 * RU: Необязательное изменение обычного дерева. Create публикует новый ID
 * и ревизию, передаёт открытое состояние записи файла. Для каталога cookie
 * равен NULL. Remove удаляет ID, сохраняя состояния других fid. Колбэки
 * возвращают 1 при успехе или 0 без изменения. Рекурсивный dispatch запрещён. */
typedef struct StyxRpcMutations
{
    int (*create)(void *context, uint64_t parent, const char *name, int directory,
        uint64_t *id, uint32_t *revision, void **cookie); /* Borrowed name; outputs only on success. */
    int (*remove)(void *context, uint64_t id);            /* Exact live ID; reject nonempty directories. */
} StyxRpcMutations;


/* EN: Enable mutations once before dispatch on an ordinary server. Return 1
 * on success, 0 on refusal. Borrow mutations until server destruction.
 * RU: Один раз включить изменение дерева до обслуживания обычного сервера.
 * Вернуть 1 при успехе или 0 при отказе. Таблица заимствована до destroy. */
int styxrpc_server_enable_mutations(StyxRpcServer *server, const StyxRpcMutations *mutations);


/* EN: Take phy on every outcome. Borrow tree/context through destroy.
 * Root ID is zero. Access uses the other-class Unix mode bits for every peer.
 * Use an already authorized connection. Returns NULL if initialization fails.
 * RU: Принять phy при любом исходе. Заимствовать tree/context до destroy.
 * Корень имеет ID ноль. Для клиента действуют биты other режима Unix.
 * Использовать уже выданное соединение. При ошибке вернуть NULL. */
StyxRpcServer *styxrpc_server_create(struct RemotePhy *phy, const StyxRpcTree *tree, void *context);


/* EN: Set or clear an owner-only observer before or between steps. The attach
 * name is borrowed for the call and conveys no permission. Pass NULL to clear.
 * RU: Установить или снять наблюдатель до запуска либо между шагами у владельца.
 * Имя attach заимствовано только на вызов и не даёт прав. NULL снимает наблюдатель. */
void styxrpc_server_observe_attach(StyxRpcServer *server, StyxRpcAttachNotice notice, void *state);


/* EN: Wait for and process one server cycle; return 1 to continue, 0 on disconnect or error.
 * Only the owning thread calls step and destroy.
 * RU: Дождаться и обработать цикл сервера; вернуть 1 для продолжения, 0 при обрыве или ошибке.
 * step и destroy вызывает владелец диспетчера. */
int styxrpc_server_step(StyxRpcServer *server);


/* EN: Process one cycle with a native wait budget in milliseconds. Zero polls, -1 waits indefinitely.
 * Timeout returns 1 (keep serving). Callback duration is separate. All calls stay on the owner.
 * RU: Обработать цикл с бюджетом ожидания в мс. Ноль опрашивает, -1 ждёт бессрочно.
 * Истечение даёт 1 (продолжать). Время колбэка отдельно. Вызовы остаются у владельца. */
int styxrpc_server_step_for(StyxRpcServer *server, int timeout_ms);


/* EN: Serve one bounded turn restricted to a module slot's native program tree.
 * Call on the owner outside another server callback. Program resources must be
 * native and safe while the application is suspended. Return zero on refusal or
 * link failure. Defer unrelated clunk callbacks to the next ordinary turn.
 * RU: Выполнить один ограниченный такт только нативного дерева программ слота.
 * Вызывать у владельца вне колбэка сервера. Ресурсы программ должны быть
 * нативными и безопасными при паузе приложения. Ноль при отказе либо обрыве.
 * Колбэки закрытия других fid откладываются до обычного такта. */
int styxrpc_server_program_step_for(StyxRpcServer *server, int timeout_ms);


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
