/* Copyright (c) 2026 bhgv
 * EN: Canonicalize capacity markers and serve virtual fids for an issued channel group.
 * RU: Канонизация признаков ёмкости и виртуальные fid выданной группы каналов.
 */
#include "channel_group_internal.h"
#include "channel_remap.h"
#include <stdlib.h>
#include <string.h>

#define FRAME STYXRPC_MANAGED_LIMIT /* Capacity of each operation buffer. */

typedef struct GroupOpen
{
    ChannelGroup *group;         /* Borrowed through registry clunk. */
    StyxRpcChannelFid fid;       /* One complete operation or pending selection. */
    unsigned char *normalized;  /* Optional owned send operands until completion. */
    unsigned char storage[];   /* Disjoint incoming request and outgoing reply. */
} GroupOpen;


/* EN: Return the exact queue for a key already checked by the group.
 * RU: Вернуть очередь точного ключа, уже проверенного группой. */
static const StyxRpcChannelQueue *find_queue(const ChannelGroup *group, uint32_t key)
{
    for (size_t i = 0; i < group->count; ++i)
    {
        if (group->grants[i].key == key)
        {
            return group->queues + i;
        }
    }
    return NULL;
}


/* EN: Preserve the exact buffer kind when Limbo echoes a key on this leased group.
 * RU: Сохранить точный вид буфера при возврате ключа Limbo в эту арендованную группу. */
static int capacity_marker(const void *context, uint32_t *key, int *unbuffered)
{
    const StyxRpcChannelQueue *queue = find_queue(context, *key);
    if (!queue)
    {
        return 0;
    }
    *unbuffered = !queue->capacity;
    return 1;
}


/* EN/RU: See channel_group_internal.h. / См. channel_group_internal.h. */
int styxrpc_group_normalize(ChannelGroup *group, const void *bytes, size_t size,
    unsigned char *output, size_t capacity, size_t *used)
{
    const ChannelKeyTransform mapping = {capacity_marker, group};
    return styxrpc_channel_reframe(&mapping, bytes, size, output, capacity, used);
}


/* EN: Canonicalize all nested sends before scheduling any alternative.
 * RU: Канонизировать вложенные отправки до планирования любой ветки. */
static int admit(void *context, StyxRpcChannelFid *fid)
{
    GroupOpen *operation = context;
    size_t written = 0;
    for (size_t i = 0; i < fid->selection.count; ++i)
    {
        StyxRpcGuard *guard = fid->selection.guards + i;
        const StyxRpcChannelQueue *queue = find_queue(operation->group, guard->key);
        if (guard->receive || !queue || !strchr(queue->grant.shape, 'c'))
        {
            continue;
        }
        if (!operation->normalized)
        {
            operation->normalized = malloc(FRAME);
        }
        if (!operation->normalized ||
            !styxrpc_group_normalize(operation->group, guard->value, guard->size,
                operation->normalized + written, FRAME - written, &guard->size))
        {
            return 0;
        }
        guard->value = operation->normalized + written;
        written += guard->size;
    }
    return styxrpc_scheduler_admit(&operation->group->scheduler, fid);
}


/* EN: Remove one unresolved fid from the dispatcher scheduler.
 * RU: Снять один ожидающий fid из планировщика диспетчера. */
static void cancel(void *context, StyxRpcChannelFid *fid)
{
    GroupOpen *operation = context;
    styxrpc_scheduler_cancel(&operation->group->scheduler, fid);
}


/* EN: Allocate request and reply buffers for one authenticated duplex open.
 * RU: Выделить буферы запроса и ответа для одного проверенного открытия. */
static int open_operation(void *context, unsigned access, void **cookie)
{
    ChannelGroup *group = context;
    if (access != (STYXRPC_READ | STYXRPC_WRITE))
    {
        return 0;
    }
    GroupOpen *operation = calloc(1, sizeof(*operation) + 2U * FRAME);
    if (!operation)
    {
        return 0;
    }
    operation->group = group;
    operation->fid = (StyxRpcChannelFid){.grants = group->grants, .grant_count = group->count,
        .request = operation->storage, .request_capacity = FRAME,
        .reply = operation->storage + FRAME, .reply_capacity = FRAME,
        .admit = admit, .cancel = cancel, .context = operation, .policy = &group->policy};
    *cookie = operation;
    return 1;
}


/* EN: Accept one contiguous request and acknowledge only validated bytes.
 * RU: Принять последовательный запрос и подтвердить проверенные байты. */
static int write_operation(void *cookie, uint64_t offset, const void *bytes, size_t size, size_t *sent)
{
    GroupOpen *operation = cookie;
    if (offset > SIZE_MAX || styxrpc_channel_fid_write(&operation->fid, (size_t)offset, bytes, size) == STYXRPC_INVALID)
    {
        return 0;
    }
    *sent = size;
    return 1;
}


/* EN: Poll WAIT before reading an immutable reply at its route's offset origin.
 * RU: Проверить WAIT и прочитать неизменяемый ответ от начала данных маршрута. */
static int read_operation(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    GroupOpen *operation = cookie;
    styxrpc_scheduler_progress(&operation->group->scheduler);
    size_t origin = operation->group->reverse ? operation->fid.staged : 0;
    return offset >= origin && offset - origin <= SIZE_MAX &&
        styxrpc_channel_fid_read(&operation->fid, (size_t)(offset - origin), bytes, capacity, size) != STYXRPC_INVALID;
}


/* EN: Retire the scheduler's borrow before releasing the operation buffers.
 * RU: Снять заимствование планировщика до освобождения буферов операции. */
static void close_operation(void *cookie)
{
    GroupOpen *operation = cookie;
    styxrpc_channel_fid_close(&operation->fid);
    free(operation->normalized);
    free(operation);
}


/* EN/RU: See channel_group_internal.h. / См. channel_group_internal.h. */
StyxRpcResource styxrpc_group_resource(ChannelGroup *group)
{
    return (StyxRpcResource){group, open_operation, read_operation, write_operation, close_operation};
}
