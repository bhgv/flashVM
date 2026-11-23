/* Copyright (c) 2026 bhgv
 * EN: Construct an owned multi-channel result resource for one module lease.
 * RU: Создание собственного ресурса нескольких каналов под одной арендой модуля.
 */
#ifndef STYXRPC_CHANNEL_GROUP_H
#define STYXRPC_CHANNEL_GROUP_H
#include "styxrpc/channel_module.h"
#include "styxrpc/channel_scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_CHANNEL_GROUP_CAPACITY 8U /* Maximum FIFO slots per queue in the leased group. */


/* EN: Copy count typed FIFO/rendezvous descriptions and their queued frames into an
 * independent resource. root selects the result channel. On success output owns the
 * resource via release. On failure output stays untouched. Serialize I/O on one owner.
 * RU: Скопировать count типизированных очередей и кадров в независимый ресурс.
 * root выбирает канал результата. При успехе output владеет ресурсом через release.
 * При ошибке output сохраняется. Ввод-вывод сериализует один владелец. */
StyxRpcStatus styxrpc_channel_group_output(const StyxRpcChannelQueue *queues, size_t count,
    size_t root, StyxRpcChannelOutput *output);

/* EN: Copy the same owned queue group for a reverse PID .unibus, using
 * post-request reply offsets. Register its resource before serving the PID.
 * RU: Скопировать группу очередей для встречного PID .unibus, со смещением
 * ответа после запроса. Зарегистрировать ресурс до обслуживания PID. */
StyxRpcStatus styxrpc_channel_group_reverse_output(const StyxRpcChannelQueue *queues,
    size_t count, size_t root, StyxRpcChannelOutput *output);

#ifdef __cplusplus
}
#endif
#endif
