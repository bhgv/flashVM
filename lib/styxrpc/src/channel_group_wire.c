/* Copyright (c) 2026 bhgv
 * EN: Canonicalize capacity markers and serve virtual fids for an issued channel group.
 * RU: Канонизация признаков ёмкости и виртуальные fid выданной группы каналов.
 */
#include "channel_group_internal.h"
#include "call_record.h"
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


/* EN: Copy or canonically reframe one record from a checked typed element.
 * RU: Скопировать или заново оформить одну запись проверенного типизированного элемента. */
static int rewrite(const ChannelGroup *group, const unsigned char *bytes, const FVM_SRecord *record,
    size_t extent, unsigned depth, unsigned char *output, size_t capacity, size_t *used)
{
    if (record->tag == STYX_MANAGED_CHANNEL_TAG && record->size == 4)
    {
        const unsigned char *p = record->payload;
        uint32_t key = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
        const StyxRpcChannelQueue *queue = find_queue(group, key);
        if (!queue)
        {
            return 0;
        }
        if (!queue->capacity)
        {
            return fvm_srecord_write(STYX_MANAGED_UNBUFFERED_CHANNEL_TAG, p, 4,
                output, capacity, used) == FVM_S_OK;
        }
    }
    int children = record->tag == STYXRPC_CHANNEL_ELEMENT_TAG ||
        record->tag == STYXRPC_VALUE_ARRAY || record->tag == STYXRPC_VALUE_TUPLE;
    if (!children || !record->size)
    {
        if (extent > capacity)
        {
            return 0;
        }
        memcpy(output, bytes, extent);
        *used = extent;
        return 1;
    }
    if (depth == STYXRPC_MANAGED_SHAPE || !capacity)
    {
        return 0;
    }
    unsigned char *body = malloc(capacity);
    if (!body)
    {
        return 0;
    }
    size_t offset = 0, written = 0;
    int ok = 1;
    while (ok && offset < record->size)
    {
        FVM_SRecord child;
        size_t length, placed;
        const unsigned char *source = record->payload + offset;
        ok = fvm_srecord_read(source, record->size - offset, FRAME, &child, &length) == FVM_S_OK &&
            rewrite(group, source, &child, length, depth + 1, body + written, capacity - written, &placed);
        if (ok)
        {
            offset += length;
            written += placed;
        }
    }
    ok = ok && fvm_srecord_write(record->tag, body, written, output, capacity, used) == FVM_S_OK;
    free(body);
    return ok;
}


/* EN/RU: See channel_group_internal.h. / См. channel_group_internal.h. */
int styxrpc_group_normalize(ChannelGroup *group, const void *bytes, size_t size,
    unsigned char *output, size_t capacity, size_t *used)
{
    FVM_SRecord outer;
    size_t extent;
    return fvm_srecord_read(bytes, size, FRAME, &outer, &extent) == FVM_S_OK &&
        extent == size && outer.tag == STYXRPC_CHANNEL_ELEMENT_TAG &&
        rewrite(group, bytes, &outer, extent, 0, output, capacity, used);
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


/* EN: Poll WAIT deadlines before copying the immutable reply from offset zero.
 * RU: Проверить сроки WAIT до копирования неизменяемого ответа от смещения ноль. */
static int read_operation(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    GroupOpen *operation = cookie;
    styxrpc_scheduler_progress(&operation->group->scheduler);
    return offset <= SIZE_MAX &&
        styxrpc_channel_fid_read(&operation->fid, (size_t)offset, bytes, capacity, size) != STYXRPC_INVALID;
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
