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


/* EN: Return live FIFO frames and cursors from the scheduler's source to the lender.
 * If shadowed is zero, source and target already share the slot arrays.
 * RU: Вернуть текущие кадры и курсоры из source планировщика в очередь владельца.
 * При нулевом shadowed массивы слотов уже общие. */
static void return_queue(StyxRpcChannelQueue *target, const StyxRpcChannelQueue *source, int shadowed)
{
    if (shadowed)
    {
        for (size_t i = 0; i < source->count; ++i)
        {
            size_t slot = (source->head + i) % source->capacity;
            memcpy(target->bytes + slot * target->stride, source->bytes + slot * source->stride, source->sizes[slot]);
            target->sizes[slot] = source->sizes[slot];
        }
    }
    target->head = source->head;
    target->count = source->count;
}


/* EN: Return borrowed queues and free owned arenas after retirement of all fids.
 * RU: Вернуть переданные очереди и освободить собственные арены после отзыва всех fid. */
static void release_group(void *context)
{
    ChannelGroup *group = context;
    for (size_t i = 0; i < group->count; ++i)
    {
        int shadowed = (group->shadows & (1U << i)) != 0;
        if (group->borrowed)
        {
            return_queue(group->borrowed + i, group->queues + i, shadowed);
        }
        if (!group->borrowed || shadowed)
        {
            free(group->queues[i].sizes);
            free(group->queues[i].bytes);
        }
    }
    free(group);
}


/* EN: Allocate mutable slot arrays for a queue with checked capacity and stride.
 * RU: Выделить изменяемые массивы для queue с проверенными ёмкостью и шагом. */
static int allocate_slots(StyxRpcChannelQueue *queue)
{
    if (!queue->capacity)
    {
        return 1;
    }
    queue->bytes = malloc(queue->capacity * queue->stride);
    queue->sizes = calloc(queue->capacity, sizeof(*queue->sizes));
    return queue->bytes && queue->sizes;
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
    return allocate_slots(queue);
}


/* EN: Check each seed from source using the typed reply builder and scratch frame.
 * Copy to the queue at index only for an owned group.
 * RU: Проверить элементы source построителем типизированных ответов через буфер frame.
 * Копировать в очередь index только при владении группой. */
static int seed(ChannelGroup *group, size_t index, const StyxRpcChannelQueue *source, void *frame, int copy)
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
        if (!size || size > source->stride || styxrpc_channel_reply(group->grants, group->count,
            &receive, 1, bytes, size, frame, STYXRPC_MANAGED_LIMIT, &extent) != STYXRPC_OK)
        {
            return 0;
        }
        if (!copy)
        {
            continue;
        }
        unsigned char *target = queue->bytes + i * queue->stride;
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
            ok = seed(group, i, queues + i, frame, 1);
            group->queues[i].count = queues[i].count;
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


/* EN/RU: See styxrpc/channel_group.h. / См. styxrpc/channel_group.h. */
StyxRpcStatus styxrpc_channel_group_reverse_borrow(StyxRpcChannelQueue *queue, StyxRpcChannelOutput *output)
{
    if (!queue || !memchr(queue->grant.shape, 0, sizeof(queue->grant.shape)) || strchr(queue->grant.shape, 'c'))
    {
        return STYXRPC_INVALID;
    }
    return styxrpc_channel_group_reverse_borrow_graph(queue, 1, 0, output);
}


/* EN: Validate the caller's arrays, then bind or allocate slots for index in group.
 * RU: Проверить массивы вызывающего, затем привязать или выделить слоты index группы. */
static int bind_queue(ChannelGroup *group, size_t index, const StyxRpcChannelQueue *source)
{
    if (!source->grant.key || source->grant.key > INT32_MAX ||
        !memchr(source->grant.shape, 0, sizeof(source->grant.shape)) || strpbrk(source->grant.shape, "rh") ||
        source->capacity > STYXRPC_CHANNEL_GROUP_CAPACITY || source->count > source->capacity ||
        (source->capacity ? source->head >= source->capacity || !source->bytes || !source->sizes ||
            !source->stride || source->stride > STYXRPC_MANAGED_LIMIT : source->head != 0))
    {
        return 0;
    }
    StyxRpcChannelQueue *queue = group->queues + index;
    *queue = *source;
    queue->head = 0;
    queue->count = 0;
    group->grants[index] = queue->grant;
    if (queue->capacity && strchr(queue->grant.shape, 'c'))
    {
        group->shadows |= 1U << index;
        return allocate_slots(queue);
    }
    return 1;
}


/* EN: Discard unfinished staging, freeing only the SDK's owned shadow slots.
 * RU: Отменить подготовку, освободив только собственные теневые слоты SDK. */
static void discard_borrow(ChannelGroup *group)
{
    for (size_t i = 0; i < group->count; ++i)
    {
        if (group->shadows & (1U << i))
        {
            free(group->queues[i].sizes);
            free(group->queues[i].bytes);
        }
    }
    free(group);
}


/* EN/RU: See styxrpc/channel_group.h. / См. styxrpc/channel_group.h. */
StyxRpcStatus styxrpc_channel_group_reverse_borrow_graph(StyxRpcChannelQueue *queues, size_t count,
    size_t root, StyxRpcChannelOutput *output)
{
    if (!queues || !count || count > STYXRPC_CHANNEL_SLOTS || root >= count || !output)
    {
        return STYXRPC_INVALID;
    }
    ChannelGroup *group = calloc(1, sizeof(*group));
    if (!group)
    {
        return STYXRPC_INVALID;
    }
    group->policy = (StyxRpcChannelPolicy){styxrpc_group_allows, group};
    group->count = count;
    group->reverse = 1;
    int valid = 1;
    int seeded = 0;
    for (size_t i = 0; valid && i < count; ++i)
    {
        valid = bind_queue(group, i, queues + i);
        seeded = seeded || queues[i].count != 0;
    }
    valid = valid && styxrpc_scheduler_init(&group->scheduler, group->queues, count, now_ms, NULL);
    void *frame = valid && seeded ? malloc(STYXRPC_MANAGED_LIMIT) : NULL;
    valid = valid && (!seeded || frame);
    for (size_t i = 0; valid && i < count; ++i)
    {
        int shadowed = (group->shadows & (1U << i)) != 0;
        valid = seed(group, i, queues + i, frame, shadowed);
        group->queues[i].head = shadowed ? 0 : queues[i].head;
        group->queues[i].count = queues[i].count;
    }
    free(frame);
    if (!valid)
    {
        discard_borrow(group);
        return STYXRPC_INVALID;
    }
    group->borrowed = queues;
    *output = (StyxRpcChannelOutput){styxrpc_group_resource(group), queues[root].grant.key,
        !queues[root].capacity, release_group};
    return STYXRPC_OK;
}
