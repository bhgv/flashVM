/* Copyright (c) 2026 bhgv
 * EN: Internal native members on one hosted ActionScript session object.
 * RU: Внутренние нативные методы объекта хостовой сессии ActionScript.
 */
#ifndef FLASHVM_STYXRPC_SESSION_METHODS_H
#define FLASHVM_STYXRPC_SESSION_METHODS_H
#include "flashvm/styxrpc_session.h"
#include "flashvm/gc_roots.h"
#include "styxrpc/exchange.h"

#define FVM_SESSION_CALL_WAIT_MS 1000U /* Reply polling budget after submission. */


/* EN: Add named launch methods to a rooted, unpublished object.
 * RU: Добавить методы запуска к удерживаемому неопубликованному объекту. */
FVM_Result fvm_styxrpc_launch_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding);


/* EN: Add file I/O methods to a rooted, unpublished object.
 * RU: Добавить методы файлов к удерживаемому неопубликованному объекту. */
FVM_Result fvm_styxrpc_vfs_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding);


/* EN: Install checked named calls on an unpublished rooted session object.
 * RU: Добавить проверяемые именованные вызовы к удерживаемому объекту сессии. */
FVM_Result fvm_styxrpc_call_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding);


/* EN: Install checked imports and variable proxies on the rooted session object.
 * RU: Добавить проверяемый импорт и прокси переменных к удерживаемому объекту сессии. */
FVM_Result fvm_styxrpc_module_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding);


/* EN: Install the named UniBus client port constructor on the rooted session object.
 * RU: Добавить конструктор именованного клиентского порта UniBus к удерживаемому объекту. */
FVM_Result fvm_styxrpc_message_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding);


/* [GC:] EN: Install newchan for the rooted session object, borrowing binding.
 * RU: Установить newchan для удерживаемого объекта сессии, заимствуя binding. */
FVM_Result fvm_styxrpc_channel_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding);


/* [ASINC] [GC:] EN: Join or create a named rendezvous on binding.
 * RU: Присоединиться к именованному каналу или создать его в binding. */
FVM_Result fvm_styxrpc_open_named_channel(FVM_Context *, FVM_Object *, unsigned,
    const FVM_Value *, FVM_Value *, void *);


/* [GC:] EN: Bind the player-owned mailbox to the default rpc object in the new movie context.
 * RU: Связать очередь плеера со штатным объектом rpc в контексте нового фильма. */
FVM_Result fvm_styxrpc_host_exchange(FVM_Context *context, StyxRpcExchange *exchange);


/* EN: Copy exact own byte properties into bytes up to capacity. Set size on success.
 * RU: Скопировать точные собственные байты в bytes до capacity. При успехе задать size. */
FVM_Result fvm_styxrpc_bytes(FVM_Context *context, const FVM_Value *value, unsigned char *bytes,
    size_t capacity, size_t *size);


/* EN: Read up to capacity own indexed fields into fields and pin them in roots until the caller finishes.
 * RU: Прочитать до capacity собственных полей в fields и удерживать их в roots до завершения вызывающего. */
FVM_Result fvm_styxrpc_fields(FVM_Context *context, FVM_GCRootScope *roots, const FVM_Value *descriptor,
    FVM_Value *fields, unsigned capacity, uint64_t *count);


/* EN: Copy a nonempty counted string to a NUL-terminated native name.
 * RU: Скопировать непустую строку с длиной в нуль-терминированное имя. */
int fvm_styxrpc_name(const FVM_Value *value, char *name, size_t capacity);


/* EN: Admit an exact nonnegative AS integer within the supplied range.
 * RU: Принять точное неотрицательное целое AS в заданном диапазоне. */
int fvm_styxrpc_number(const FVM_Value *value, uint64_t limit, uint64_t *result);
#endif
