/* Copyright (c) 2026 bhgv
 * EN: Bind FlashVM script functions to named scalar exports in libstyxrpc.
 * RU: Связь функций FlashVM с именованными скалярными экспортами libstyxrpc.
 */
#ifndef FLASHVM_STYXRPC_H
#define FLASHVM_STYXRPC_H
#include "flashvm.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct StyxClient;     /* Mounted session owned by the host. */
struct StyxRpcExport;  /* Scalar descriptor defined by libstyxrpc. */

typedef struct FVM_StyxRpcImport
{
    struct StyxClient *session; /* Borrowed connected and mounted Styx client. */
    const char *module;         /* NUL-terminated module path relative to the mount. */
    const char *name;           /* NUL-terminated function name in that module. */
    unsigned wait_ms;           /* Reply polling budget in milliseconds. */
} FVM_StyxRpcImport;

typedef struct FVM_StyxRpcExport
{
    FVM_Context *context;   /* Context owned by the serving thread. */
    const char *function;   /* NUL-terminated AS function name resolved on each call. */
} FVM_StyxRpcExport;


/* EN: Register global as a synchronous remote fn(int): int. Keep binding,
 * strings alive while callable. When idle the owner may clear the borrowed
 * session before closing it. Use a dedicated context.
 * RU: Зарегистрировать global как синхронную удалённую fn(int): int. Хранить
 * binding и строки, пока функция доступна. Между вызовами владелец может
 * обнулить заимствованную сессию перед закрытием. Использовать выделенный контекст. */
FVM_API FVM_Result fvm_styxrpc_import_int(FVM_Context *context, const char *global,
    FVM_StyxRpcImport *binding);


/* EN: Fill a Styx scalar descriptor for the named AS function. Keep binding,
 * its context and both names alive through server destruction. Dispatch on the
 * FlashVM context owner thread. On failure leave the descriptor unchanged.
 * RU: Заполнить описатель Styx для именованной функции AS. Хранить binding,
 * его контекст и оба имени до уничтожения сервера. Обслуживать в потоке
 * владельца контекста FlashVM. При отказе описатель не меняется. */
FVM_API FVM_Result fvm_styxrpc_export_int(FVM_StyxRpcExport *binding, const char *name,
    struct StyxRpcExport *exported);

#ifdef __cplusplus
}
#endif
#endif
