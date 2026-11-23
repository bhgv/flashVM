/* Copyright (c) 2026 bhgv
 * EN: Validate and copy a bounded graph of module-owned channel queues.
 * RU: Проверка и копирование ограниченного графа очередей модуля.
 */
#define _POSIX_C_SOURCE 200809L
#include "channel_group_internal.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN: Provide monotonic milliseconds for the group scheduler.
 * RU: Предоставить планировщику группы монотонные миллисекунды. */
static uint64_t now_ms(void *unused)
{
    struct timespec now;
    (void)unused;
    return !clock_gettime(CLOCK_MONOTONIC, &now) ?
        UINT64_C(1000) * (uint64_t)now.tv_sec + (uint64_t)now.tv_nsec / 1000000 : 0;
}


/* EN: Drop every owned arena after the registry retires all operation fids.
 * RU: Освободить все арены после отзыва реестром всех fid операций. */
static void release_group(void *context)
{
    ChannelGroup *group = context;
    for (size_t i = 0; i < group->count; ++i)
    {
        free(group->queues[i].sizes);
        free(group->queues[i].bytes);
    }
    free(group);
}


/* EN/RU: See channel_group_internal.h. / См. channel_group_internal.h. */
int styxrpc_group_allows(const void *context, const StyxRpcChannelRoute *route)
{
    const ChannelGroup *group = context;
    if (route->kind != STYXRPC_ROUTE_CURRENT)
    {
        return 0;
    }
    for (size_t i = 0; i < group->count; ++i)
    {
        const StyxRpcChannelQueue *queue = group->queues + i;
        if (queue->grant.key == route->channel.key)
        {
            return (!route->unbuffered || !queue->capacity) && !strcmp(queue->grant.shape, route->channel.shape);
        }
    }
    return 0;
}


/* EN: Allocate one cleared queue, permitting owner writes up to the native frame limit.
 * RU: Выделить пустую очередь, допускающую запись до предела штатного кадра. */
static int allocate(StyxRpcChannelQueue *queue, const StyxRpcChannelQueue *source)
{
    if (!source->grant.key || source->grant.key > INT32_MAX ||
        !memchr(source->grant.shape, 0, sizeof(source->grant.shape)) || strpbrk(source->grant.shape, "rh") ||
        source->capacity > STYXRPC_CHANNEL_GROUP_CAPACITY || source->count > source->capacity ||
        (source->capacity ? source->head >= source->capacity : source->head != 0) ||
        (source->count && (!source->sizes || !source->bytes ||
            !source->stride || source->stride > STYXRPC_MANAGED_LIMIT)))
    {
        return 0;
    }
    queue->grant = source->grant;
    queue->capacity = source->capacity;
    queue->stride = STYXRPC_MANAGED_LIMIT;
    if (queue->capacity)
    {
        queue->bytes = malloc(queue->capacity * queue->stride);
        queue->sizes = calloc(queue->capacity, sizeof(*queue->sizes));
        if (!queue->bytes || !queue->sizes)
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Check every seed against its declared type and known group keys before copying.
 * RU: Проверить каждый начальный элемент по типу и ключам группы перед копированием. */
static int seed(ChannelGroup *group, size_t index, const StyxRpcChannelQueue *source, void *frame)
{
    StyxRpcChannelQueue *queue = group->queues + index;
    const StyxRpcSelection receive = {.count = 1, .guards = {{queue->grant.key, 1, NULL, 0}},
        .policy = &group->policy};
    int nested = strchr(queue->grant.shape, 'c') != NULL;
    for (size_t i = 0; i < source->count; ++i)
    {
        size_t slot = (source->head + i) % source->capacity;
        const unsigned char *bytes = source->bytes + slot * source->stride;
        size_t size = source->sizes[slot], extent;
        unsigned char *target = queue->bytes + i * queue->stride;
        if (!size || size > source->stride || styxrpc_channel_reply(group->grants, group->count,
            &receive, 1, bytes, size, frame, STYXRPC_MANAGED_LIMIT, &extent) != STYXRPC_OK)
        {
            return 0;
        }
        if (nested)
        {
            if (!styxrpc_group_normalize(group, bytes, size, target, queue->stride, &size) ||
                styxrpc_channel_reply(group->grants, group->count, &receive, 1,
                    target, size, frame, STYXRPC_MANAGED_LIMIT, &extent) != STYXRPC_OK)
            {
                return 0;
            }
        }
        else
        {
            memcpy(target, bytes, size);
        }
        queue->sizes[i] = size;
    }
    queue->count = source->count;
    return 1;
}


/* EN: Copy and validate queues for one module or reverse PID wire profile.
 * RU: Скопировать и проверить очереди для профиля модуля либо встречного PID. */
static StyxRpcStatus create_output(const StyxRpcChannelQueue *queues, size_t count,
    size_t root, StyxRpcChannelOutput *output, int reverse)
{
    if (!queues || !count || count > STYXRPC_CHANNEL_SLOTS || root >= count || !output)
    {
        return STYXRPC_INVALID;
    }
    ChannelGroup *group = calloc(1, sizeof(*group));
    void *frame = malloc(STYXRPC_MANAGED_LIMIT);
    int ok = group && frame;
    if (group)
    {
        group->count = count;
        group->reverse = reverse;
        group->policy = (StyxRpcChannelPolicy){styxrpc_group_allows, group};
        for (size_t i = 0; ok && i < count; ++i)
        {
            ok = allocate(group->queues + i, queues + i);
            group->grants[i] = queues[i].grant;
        }
        ok = ok && styxrpc_scheduler_init(&group->scheduler, group->queues, count, now_ms, NULL);
        for (size_t i = 0; ok && i < count; ++i)
        {
            ok = seed(group, i, queues + i, frame);
        }
    }
    free(frame);
    if (ok)
    {
        *output = (StyxRpcChannelOutput){styxrpc_group_resource(group), group->grants[root].key,
            !group->queues[root].capacity, release_group};
        return STYXRPC_OK;
    }
    if (group)
    {
        release_group(group);
    }
    return STYXRPC_INVALID;
}


/* EN/RU: See styxrpc/channel_group.h. / См. styxrpc/channel_group.h. */
StyxRpcStatus styxrpc_channel_group_output(const StyxRpcChannelQueue *queues, size_t count,
    size_t root, StyxRpcChannelOutput *output)
{
    return create_output(queues, count, root, output, 0);
}


/* EN/RU: See styxrpc/channel_group.h. / См. styxrpc/channel_group.h. */
StyxRpcStatus styxrpc_channel_group_reverse_output(const StyxRpcChannelQueue *queues, size_t count,
    size_t root, StyxRpcChannelOutput *output)
{
    return create_output(queues, count, root, output, 1);
}
