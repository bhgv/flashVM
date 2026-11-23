/* Copyright (c) 2026 bhgv
 * EN: Import scoped Dis functions returning live native UniBus capabilities.
 * RU: Импорт контекстных функций Dis, возвращающих живые полномочия UniBus.
 */
#ifndef FLASHVM_STYXRPC_CHANNEL_H
#define FLASHVM_STYXRPC_CHANNEL_H
#include "flashvm/value.h"
#include "styxrpc/channel_client.h"
#include "styxrpc/channel_module.h"
#include "styxrpc/channel_arguments.h"
#include "styxrpc/channel_handlers.h"

#ifdef __cplusplus
extern "C" {
#endif


/* [GC:] EN: Register a synchronous global on its owner; borrow target while callable.
 * RU: Зарегистрировать синхронную глобальную функцию; заимствовать target, пока она доступна. */
FVM_API FVM_Result fvm_styxrpc_channel_import(FVM_Context *context, const char *global,
    StyxRpcManagedTarget *target);

/* [GC:] EN: Create an opaque message value for a reverse PID published by the host
 * on the anchor's exact bidirectional mount. The returned value retains its lease.
 * RU: Создать непрозрачное значение для встречного PID, выданного хостом на
 * двунаправленном монтировании anchor. Возвращаемое значение удерживает аренду. */
FVM_API FVM_Result fvm_styxrpc_channel_delegate_pid(FVM_Context *context, const FVM_Value *anchor,
    const StyxRpcChannelRoute *route, FVM_Value *out);


/* [GC:] EN: Publish a local queue and its reachable queued channels on the
 * program server from anchor's mount. Transfer exclusive access until the
 * last wrapper or PID value of the anchor's family is released. Keep reverse
 * alive through retirement. Call on the FlashVM owner.
 * RU: Опубликовать локальную очередь и достижимые из неё каналы на сервере
 * монтирования anchor. Передать доступ к ним до освобождения последней
 * обёртки или значения PID семейства. Хранить reverse до отзыва.
 * Вызывать в потоке владельца FlashVM. */
FVM_API FVM_Result fvm_styxrpc_channel_delegate_local(FVM_Context *context, const FVM_Value *anchor,
    const FVM_Value *value, StyxRpcServer *reverse, FVM_Value *out);


/* [GC:] EN: Register global(anchor, queue) on the FlashVM owner. Borrow reverse
 * while the global is callable and until all issued publications are retired.
 * RU: Зарегистрировать global(anchor, queue) в потоке FlashVM. Заимствовать
 * reverse, пока функция доступна и её публикации действуют. */
FVM_API FVM_Result fvm_styxrpc_channel_publisher(FVM_Context *context, const char *global, StyxRpcServer *reverse);


/* [GC:] EN: Admit a module grant authenticated by the host for the anchor's lease.
 * Return a live object on the held module, preserving known aliases.
 * RU: Принять проверенное хостом полномочие модуля в аренду anchor.
 * Вернуть живой объект на удерживаемом модуле с сохранением известных алиасов. */
FVM_API FVM_Result fvm_styxrpc_channel_admit_module(FVM_Context *context, const FVM_Value *anchor,
    const StyxRpcChannelRoute *route, FVM_Value *out);


/* [GC:] EN: Decode one typed reverse message routed to anchor by the host.
 * Authenticate its module grants through the pinned module. Return the value in out.
 * RU: Декодировать типизированное встречное сообщение, направленное хостом к anchor.
 * Проверить полномочия на удерживаемом модуле. Вернуть значение через out. */
FVM_API FVM_Result fvm_styxrpc_channel_admit_message(FVM_Context *context, const FVM_Value *anchor,
    const char *shape, const void *bytes, size_t size, FVM_Value *out);


typedef struct FVM_StyxRpcChannelArguments
{
    StyxRpcManagedTarget *target; /* Borrowed exact remote offer and caller PID. */
    StyxRpcArgumentBus *bus;     /* Borrowed reverse bus registered on the same connection. */
} FVM_StyxRpcChannelArguments;


/* [GC:] EN: Install a function which lends script-local typed queues for one scoped call.
 * RU: Установить функцию, временно передающую типизированные очереди AS для одного вызова. */
FVM_API FVM_Result fvm_styxrpc_channel_import_arguments(FVM_Context *context, const char *global,
    FVM_StyxRpcChannelArguments *binding);


/* [GC:] EN: Register a script constructor taking a compact element type and slot count.
 * RU: Зарегистрировать конструктор AS с компактным типом элемента и числом слотов. */
FVM_API FVM_Result fvm_styxrpc_channel_factory(FVM_Context *context, const char *global);

#define FVM_STYXRPC_CHANNEL_QUEUE_SLOTS 8U /* Maximum FIFO slots for one FlashVM export. */

typedef struct FVM_StyxRpcChannelExport
{
    FVM_Context *context;    /* Owner running the server and the named AS initializer. */
    const char *initializer; /* Callable AS function returning a seed or undefined. */
    StyxRpcSchema schema;     /* Complete compiled value-input, single-channel result. */
    unsigned capacity;       /* FIFO slots, zero for synchronous rendezvous. */
} FVM_StyxRpcChannelExport;


/* [GC:] EN: Create a scoped channel descriptor borrowing binding and names through teardown.
 * RU: Создать дескриптор контекстного канала, заимствуя привязку и имена до teardown. */
FVM_API FVM_Result fvm_styxrpc_channel_export(FVM_StyxRpcChannelExport *binding,
    const char *name, uint32_t signature, StyxRpcChannelExport *exported);

typedef struct FVM_StyxRpcChannelHandler
{
    FVM_Context *context;       /* Context of the sole callback owner. */
    const char *function;       /* Named AVM1 function on context. */
    struct StyxClient *reverse; /* Borrowed reverse client on the same Styx session. */
    StyxRpcSchema schema;       /* Channel arguments, copied result or void. */
    unsigned wait_ms;           /* 1..60000 ms per reverse operation. */
} FVM_StyxRpcChannelHandler;


/* [GC:] EN: Bind one AVM1 callback to the exact compiled function in a hosted handler module.
 * The binding, name and reverse client are borrowed through module teardown.
 * RU: Связать колбэк AVM1 с компилированной функцией в хостовом модуле обработчиков.
 * Привязка, имя и встречный клиент заимствуются до завершения модуля. */
FVM_API FVM_Result fvm_styxrpc_channel_handler(FVM_StyxRpcChannelHandler *binding,
    const char *name, uint32_t signature, StyxRpcChannelFunction *function);

typedef struct FVM_StyxRpcChannelHandlerExport
{
    FVM_StyxRpcChannelHandler handler; /* Context, reverse attachment and compiled shape. */
    unsigned capacity;                 /* Result FIFO slots, zero for rendezvous. */
} FVM_StyxRpcChannelHandlerExport;


/* [GC:] EN: Bind a callback with channel arguments and one separately leased result channel.
 * Borrow binding and name until both the dispatcher and the callback owner exit.
 * RU: Связать колбэк с аргументами-каналами и одним отдельно арендованным результатом.
 * Заимствовать binding и name до выхода диспетчера и владельца колбэков. */
FVM_API FVM_Result fvm_styxrpc_channel_handler_export(FVM_StyxRpcChannelHandlerExport *binding,
    const char *name, uint32_t signature, StyxRpcChannelExport *exported);

#ifdef __cplusplus
}
#endif
#endif
