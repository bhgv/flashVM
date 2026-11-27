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

struct StyxFid; /* Unopened retained directory issued by the mounted peer. */

typedef struct StyxRpcModuleImport
{
    const char *name;        /* Required literal export name. */
    StyxRpcModuleKind kind; /* Native codec required for the function. */
    uint32_t signature;      /* Exact compiled signature. */
    StyxRpcSchema schema;   /* Exact resolved schema for shaped profiles. */
} StyxRpcModuleImport;


/* EN: Own discovered names alongside their checked contracts.
 * RU: Хранить найденные имена вместе с проверенными контрактами. */
typedef struct StyxRpcModuleExports
{
    StyxRpcModuleImport methods[STYXRPC_MAX_EXPORTS];      /* Verified contracts. */
    char names[STYXRPC_MAX_EXPORTS][STYXRPC_TREE_NAME + 1]; /* Owned export names. */
    size_t count;                                        /* Number of contracts. */
} StyxRpcModuleExports;

/* EN: Discover directly callable exports. Keep out at its original address after success.
 * RU: Найти доступные для прямого вызова экспорты. После успеха не перемещать out. */
StyxRpcStatus styxrpc_module_exports(struct StyxClient *, const char *, uint32_t, StyxRpcModuleExports *);

/* EN: Check all requirements on one held directory of the attached session.
 * The path is relative. A nonzero PID is required by any scoped requirement.
 * RU: Проверить все требования на одном удерживаемом каталоге смонтированной сессии.
 * Путь относительный. Для контекстной функции требуется ненулевой PID. */
StyxRpcStatus styxrpc_module_check(struct StyxClient *session, const char *module,
    const StyxRpcModuleImport *imports, size_t count, uint32_t pid);


/* EN: Walk an ordinary module directory; return an owned, unopened fid or NULL.
 * Release with styxclientclunk or styxrpc_vfs_close. No path fallback after capture.
 * RU: Пройти каталог модуля и вернуть свой неоткрытый fid либо NULL.
 * Закрыть через styxclientclunk либо styxrpc_vfs_close. Без повторного поиска пути. */
struct StyxFid *styxrpc_module_open(struct StyxClient *session, const char *path);

/* EN: Ask the attached Dis owner to load a visible relative .dis file.
 * Return an owned exact directory fid or NULL. On success lease receives the
 * owned request fid. Keep it open for all calls and returned channel use.
 * Close both via styxclientclunk. The operation may wait for owner readiness.
 * RU: Запросить загрузку видимого относительного .dis у владельца Dis.
 * Вернуть свой точный fid каталога либо NULL. При успехе lease получает fid
 * запроса. Удерживать его на время вызовов и использования возвращённых каналов.
 * Закрыть оба через styxclientclunk. Операция может ждать готовности владельца. */
struct StyxFid *styxrpc_module_live_open(struct StyxClient *session, const char *path,
    struct StyxFid **lease);

/* EN: Discover checked exports relative to the borrowed, unopened module fid.
 * RU: Найти проверенные экспорты от заимствованного неоткрытого fid модуля. */
StyxRpcStatus styxrpc_module_exports_at(struct StyxFid *directory, uint32_t pid, StyxRpcModuleExports *out);

/* EN: Discover checked exports and optionally report required caller scopes.
 * Bit i of scoped corresponds to methods[i]. Leave both outputs unchanged on
 * failure. Like exports_at, keep out at its original address on success.
 * RU: Найти проверенные экспорты и при необходимости выдать области вызовов.
 * Бит i в scoped соответствует methods[i]. При отказе сохранить оба выхода.
 * Как и в exports_at, держать out по прежнему адресу при успехе. */
StyxRpcStatus styxrpc_module_exports_scoped_at(struct StyxFid *directory, uint32_t pid,
    StyxRpcModuleExports *out, uint32_t *scoped);

/* EN: Check exact imports relative to the borrowed, unopened module fid.
 * RU: Проверить точные импорты от заимствованного неоткрытого fid модуля. */
StyxRpcStatus styxrpc_module_check_at(struct StyxFid *directory,
    const StyxRpcModuleImport *imports, size_t count, uint32_t pid);

#ifdef __cplusplus
}
#endif
#endif
