/* Copyright (c) 2026 bhgv
 * EN: Publish call-local channel argument keys through a stable reverse Styx bus.
 * RU: Публикация ключей каналов вызова через стабильную встречную шину Styx.
 */
#ifndef STYXRPC_CHANNEL_ARGUMENTS_H
#define STYXRPC_CHANNEL_ARGUMENTS_H
#include "styxrpc/channel_client.h"
#include "styxrpc/channel_scheduler.h"
#include "styxrpc/program.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StyxRpcArgumentBus StyxRpcArgumentBus; /* One serialized reverse bus for a caller PID. */
typedef struct StyxRpcArgumentScope StyxRpcArgumentScope; /* One staged or active forward invocation. */


/* EN: Allocate a bus for pid. Publish its resource once before dispatch starts.
 * RU: Выделить шину pid. Опубликовать её ресурс до запуска диспетчера. */
StyxRpcArgumentBus *styxrpc_argument_bus_create(uint32_t pid);


/* EN: Return borrowed callbacks; register them on the matching process with program_unibus.
 * RU: Вернуть заимствованные колбэки для program_unibus соответствующего процесса. */
StyxRpcResource styxrpc_argument_bus_resource(StyxRpcArgumentBus *bus);


/* EN: Free the bus after the reverse server and all scopes have closed. NULL is allowed.
 * RU: Освободить шину после остановки встречного сервера и закрытия областей. Допустим NULL. */
void styxrpc_argument_bus_destroy(StyxRpcArgumentBus *bus);


/* EN: Begin an exclusive staging scope for the matching caller PID.
 * RU: Начать исключительную подготовку для соответствующего PID. */
StyxRpcArgumentScope *styxrpc_argument_scope_open(StyxRpcArgumentBus *bus, uint32_t pid);


/* EN: Encode the exact channel argument and remember its borrowed queue once per scope.
 * RU: Закодировать аргумент канала и однократно удержать его заимствованную очередь. */
StyxRpcStatus styxrpc_argument_scope_value(StyxRpcArgumentScope *scope, StyxRpcChannelQueue *queue,
    void *bytes, size_t capacity, size_t *used);


/* EN: Publish all staged keys atomically just before transmitting the forward request.
 * RU: Атомарно опубликовать подготовленные ключи перед отправкой прямого запроса. */
int styxrpc_argument_scope_activate(StyxRpcArgumentScope *scope);


/* EN: Revoke the scope, cancel the pending operations and return live queue cursors.
 * RU: Отозвать область, отменить ожидающие операции и вернуть живые курсоры очередей. */
void styxrpc_argument_scope_close(StyxRpcArgumentScope *scope);


/* EN: Consume a staged scope, verify the exact offer and activate before the first write.
 * RU: Поглотить подготовленную область, проверить предложение и выдать перед первой записью. */
StyxRpcOutcome styxrpc_argument_scope_invoke(StyxRpcArgumentScope *scope, const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcLeasedReply *result);

#ifdef __cplusplus
}
#endif
#endif
