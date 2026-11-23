/* Copyright (c) 2026 bhgv
 * EN: Dispatch scoped channel argument calls on a separate application owner.
 * RU: Диспетчеризация вызовов с аргументами-каналами у отдельного владельца приложения.
 */
#ifndef STYXRPC_CHANNEL_HANDLERS_H
#define STYXRPC_CHANNEL_HANDLERS_H
#include "styxrpc/channel_module.h"
#include "styxrpc/serve.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StyxRpcChannelHandlers StyxRpcChannelHandlers; /* Opaque hosted module and work queue. */


/* EN: On the owner, consume call/context and write at most capacity typed child bytes to result.
 * Set used on OK, including zero for void. The call and output buffer expire on return.
 * RU: У владельца обработать call/context и записать в result до capacity байт дочерних записей.
 * Установить used при OK, включая ноль для void. Вызов и буфер действуют до возврата. */
typedef StyxRpcStatus (*StyxRpcChannelHandler)(void *context, const StyxRpcChannelCall *call,
    void *result, size_t capacity, size_t *used);

typedef struct StyxRpcChannelFunction
{
    const char *name;               /* Compiled Limbo export name. */
    uint32_t signature;             /* Exact Limbo signature. */
    StyxRpcSchema schema;           /* Channel/value arguments and one copied result or void. */
    StyxRpcChannelHandler handler; /* Owner callback. */
    void *context;                  /* Borrowed application state. */
} StyxRpcChannelFunction;


/* EN: Create the tree context for name and count immutable functions, borrowed until destroy.
 * Return NULL for invalid descriptors or unavailable memory/synchronization resources.
 * RU: Создать контекст дерева name с count неизменяемыми functions до destroy.
 * NULL при ошибке описателей, нехватке памяти или ресурсов синхронизации. */
StyxRpcChannelHandlers *styxrpc_channel_handlers_create(const char *name,
    const StyxRpcChannelFunction *functions, size_t count);


/* EN: Create a mixed module. Borrow both descriptor arrays and a dispatcher-owned
 * registry; result callbacks run on the owner and transfer a movable resource.
 * RU: Создать смешанный модуль. Заимствовать оба массива и реестр диспетчера;
 * колбэки результата работают у владельца и передают перемещаемый ресурс. */
StyxRpcChannelHandlers *styxrpc_channel_handlers_create_with_results(const char *name,
    const StyxRpcChannelFunction *functions, size_t count, const StyxRpcChannelExport *results,
    size_t result_count, StyxRpcModuleChannels *registry);


/* EN: In the sole callback owner, wait for and run one job. Return 0 after stop, 1 otherwise.
 * RU: У единственного владельца ждать и выполнить одну работу. После stop вернуть 0, иначе 1. */
int styxrpc_channel_handlers_step(StyxRpcChannelHandlers *module);


/* EN: Stop admission, discard queued calls and wake the owner. An active callback may finish.
 * RU: Остановить приём, снять очередь и разбудить владельца. Начатый колбэк может завершиться. */
void styxrpc_channel_handlers_stop(StyxRpcChannelHandlers *module);


/* EN: Free module after the server and owner exit. NULL is accepted.
 * RU: Освободить module после завершения сервера и владельца. Допускается NULL. */
void styxrpc_channel_handlers_destroy(StyxRpcChannelHandlers *module);

extern const StyxRpcTree styxrpc_channel_handlers_tree; /* Context is a created StyxRpcChannelHandlers. */

#ifdef __cplusplus
}
#endif
#endif
