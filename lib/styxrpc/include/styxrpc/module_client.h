/* Copyright (c) 2026 bhgv
 * EN: Check the callable surface of a mounted module without running any entry.
 * RU: Проверка входов смонтированного модуля без запуска колбэков.
 */
#ifndef STYXRPC_MODULE_CLIENT_H
#define STYXRPC_MODULE_CLIENT_H
#include "styxrpc/module.h"
#include "styxrpc/managed_client.h"
#include "styxrpc/vfs_client.h"

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

/* EN: Return nonzero to stop a module operation on the calling thread.
 * RU: Вернуть ненулевое значение для остановки операции с модулем в потоке вызова. */
typedef int (*StyxRpcModuleStop)(void *state);

/* EN: Capture a visible module directory or clone a borrowed unopened base
 * when supplied. Poll stop through walk/stat and transfer out only on success.
 * RU: Захватить видимый каталог модуля или клонировать неоткрытый base.
 * Проверять stop при обходе и stat. Передать out только при успехе. */
StyxRpcIoOutcome styxrpc_module_capture_until(struct StyxClient *session, const char *path,
    struct StyxFid *base, struct StyxFid **out, StyxRpcModuleStop stop, void *state);


/* EN: Check count imports for pid on the borrowed unopened directory.
 * Poll stop(state) during verification and return STYXRPC_CANCELLED if stopped.
 * RU: Проверить count импортов для pid на заимствованном неоткрытом directory.
 * Вызывать stop(state) при проверке, вернуть STYXRPC_CANCELLED при остановке. */
StyxRpcStatus styxrpc_module_check_at_until(struct StyxFid *directory,
    const StyxRpcModuleImport *imports, size_t count, uint32_t pid, StyxRpcModuleStop stop, void *state);


/* EN: Discover checked exports for pid on the borrowed unopened directory.
 * Publish out and optional scope bits only on success. Poll stop(state) during verification.
 * RU: Найти экспорты для pid на заимствованном неоткрытом directory.
 * Выдать out и необязательные биты областей только при успехе. Проверять stop(state). */
StyxRpcStatus styxrpc_module_exports_scoped_at_until(struct StyxFid *directory, uint32_t pid,
    StyxRpcModuleExports *out, uint32_t *scoped, StyxRpcModuleStop stop, void *state);


/* EN: Load with an optional stop hook. On success transfer both fids; otherwise
 * leave both output slots unchanged and retire any temporary request fid.
 * RU: Загрузить с необязательной проверкой остановки. При успехе выдать оба fid;
 * при отказе сохранить выходы и освободить временный fid запроса. */
StyxRpcIoOutcome styxrpc_module_live_open_until(struct StyxClient *session, const char *path,
    struct StyxFid **directory, struct StyxFid **lease, StyxRpcModuleStop stop, void *state);

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
