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


/* EN: Lend one by-value FIFO or rendezvous queue exclusively to a reverse PID
 * resource. Keep queue and its arrays alive and idle until output.release after
 * all fids retire. Release restores the advanced cursors. On failure leave
 * queue and output unchanged.
 * RU: Эксклюзивно передать FIFO или rendezvous с элементами по значению ресурсу
 * встречного PID. Хранить queue и её массивы без доступа до output.release после
 * отзыва всех fid. Освобождение возвращает обновлённые курсоры. При ошибке
 * queue и output не меняются. */
StyxRpcStatus styxrpc_channel_group_reverse_borrow(StyxRpcChannelQueue *queue, StyxRpcChannelOutput *output);


/* EN: Lend count queues to one reverse PID. root selects the returned key.
 * Nested leaves refer to exact current-group keys. Release returns FIFO state
 * into queues and its arrays after all fids retire. On failure leave inputs unchanged.
 * RU: Передать count очередей одному встречному PID. root выбирает ключ.
 * Вложенные ссылки используют точные текущие ключи группы. После отзыва всех fid
 * освобождение возвращает состояние в queues и её массивы. При отказе входы сохраняются. */
StyxRpcStatus styxrpc_channel_group_reverse_borrow_graph(StyxRpcChannelQueue *queues, size_t count,
    size_t root, StyxRpcChannelOutput *output);

#ifdef __cplusplus
}
#endif
#endif
