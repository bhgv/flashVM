/* Copyright (c) 2026 bhgv
 * EN: Bounded native selection scheduler with atomic unbuffered replies.
 * RU: Ограниченный нативный планировщик веток с атомарными ответами rendezvous.
 */
#include "styxrpc/channel_scheduler.h"
#include "load_profile.h"
#include <string.h>

#define WAIT_WINDOW 40U /* Existing authoritative mixed-alt window in milliseconds. */


/* EN: Find one admitted key in this exact scheduler.
 * RU: Найти выданный ключ в данном планировщике. */
static StyxRpcChannelQueue *queue_for(StyxRpcChannelScheduler *scheduler, uint32_t key)
{
    for (size_t i = 0; i < scheduler->count; ++i)
    {
        if (scheduler->queues[i].grant.key == key)
        {
            return &scheduler->queues[i];
        }
    }
    return NULL;
}


/* EN: Find the selected fid's own grant; shapes are checked at admission.
 * RU: Найти полномочие fid. Формы проверяются при приёме. */
static const char *fid_shape(const StyxRpcChannelFid *fid, uint32_t key)
{
    for (size_t i = 0; i < fid->grant_count; ++i)
    {
        if (fid->grants[i].key == key)
        {
            return fid->grants[i].shape;
        }
    }
    return NULL;
}


/* EN: Check storage and key metadata without consuming the caller's buffers.
 * RU: Проверить память и ключи, не принимая владение буферами. */
static int queue_valid(const StyxRpcChannelQueue *queue)
{
    const char *shape = queue->grant.shape;
    if (!queue->grant.key || !memchr(shape, 0, sizeof(queue->grant.shape)) || shape[0] != '(' ||
        queue->count || queue->head ||
        (queue->capacity && (!queue->bytes || !queue->sizes || !queue->stride ||
            queue->stride > STYXRPC_MANAGED_LIMIT || queue->capacity > SIZE_MAX / queue->stride)))
    {
        return 0;
    }
    const char *end = styxshape_end(shape + 1);
    return end && !strcmp(end, ")");
}


/* EN/RU: See channel_scheduler.h. / См. channel_scheduler.h. */
int styxrpc_scheduler_init(StyxRpcChannelScheduler *scheduler, StyxRpcChannelQueue *queues,
    size_t count, uint64_t (*clock_ms)(void *), void *context)
{
    if (!scheduler || !queues || !count || count > STYXRPC_CHANNEL_SLOTS)
    {
        return 0;
    }
    for (size_t i = 0; i < count; ++i)
    {
        if (!queue_valid(&queues[i]))
        {
            return 0;
        }
        for (size_t j = 0; j < i; ++j)
        {
            if (queues[i].grant.key == queues[j].grant.key)
            {
                return 0;
            }
        }
    }
    *scheduler = (StyxRpcChannelScheduler){.queues = queues, .count = count,
        .clock_ms = clock_ms, .context = context};
    return 1;
}


/* EN: Stage a reply without publishing it, adapting received int values for mixed ALT.
 * RU: Подготовить ответ до публикации и завернуть int для смешанного ALT. */
static size_t stage(StyxRpcChannelFid *fid, unsigned ordinal, const StyxRpcChannelQueue *queue,
    const void *value, size_t size)
{
    unsigned char wrapper[32];
    if (value && queue && !strcmp(queue->grant.shape, "(i)"))
    {
        for (size_t i = 0; i < fid->selection.count; ++i)
        {
            const char *shape = fid_shape(fid, fid->selection.guards[i].key);
            if (strcmp(shape, "(i)"))
            {
                size_t extent;
                if (fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, value, size, wrapper,
                    sizeof(wrapper), &extent) != FVM_S_OK)
                {
                    return 0;
                }
                size = extent;
                value = wrapper;
                break;
            }
        }
    }
    size_t used;
    return styxrpc_channel_reply(fid->grants, fid->grant_count, &fid->selection, ordinal, value, size,
        fid->reply, fid->reply_capacity, &used) == STYXRPC_OK ? used : 0;
}


/* EN: Publish a previously staged reply or retire an invalid fid.
 * RU: Опубликовать подготовленный ответ или завершить недопустимый fid. */
static void publish(StyxRpcChannelFid *fid, size_t size)
{
    fid->ready = size;
    fid->pending = 0;
    fid->refused = !size;
}


/* EN: Match two different fids atomically or fail both without transferring a value.
 * RU: Сопоставить разные fid атомарно либо отклонить оба без передачи значения. */
static void pair(StyxRpcChannelQueue *queue, StyxRpcChannelFid *sender, unsigned send,
    StyxRpcChannelFid *receiver, unsigned receive)
{
    const StyxRpcGuard *guard = &sender->selection.guards[send];
    size_t received = stage(receiver, receive + 1, queue, guard->value, guard->size);
    size_t sent = received ? stage(sender, send + 1, NULL, NULL, 0) : 0;
    if (!sent || !received)
    {
        sent = stage(sender, 255, NULL, NULL, 0);
        received = stage(receiver, 255, NULL, NULL, 0);
    }
    publish(receiver, received);
    publish(sender, sent);
}


/* EN: Find the oldest compatible opposite guard on another unresolved fid.
 * RU: Найти старейшую совместимую противоположную ветку другого ожидающего fid. */
static int rendezvous(StyxRpcChannelScheduler *scheduler, StyxRpcChannelQueue *queue,
    StyxRpcChannelFid *fid, unsigned index)
{
    const StyxRpcGuard *guard = &fid->selection.guards[index];
    for (size_t i = 0; i < scheduler->used; ++i)
    {
        StyxRpcChannelFid *peer = scheduler->pending[i].fid;
        if (peer == fid || !peer->pending)
        {
            continue;
        }
        for (size_t j = 0; j < peer->selection.count; ++j)
        {
            const StyxRpcGuard *other = &peer->selection.guards[j];
            if (other->key == guard->key && other->receive != guard->receive)
            {
                if (guard->receive)
                {
                    pair(queue, peer, (unsigned)j, fid, index);
                }
                else
                {
                    pair(queue, fid, index, peer, (unsigned)j);
                }
                return 1;
            }
        }
    }
    return 0;
}


/* EN: Transfer exactly one buffered guard after its reply fits.
 * RU: Выполнить ровно одну буферизованную ветку после подготовки ответа. */
static int buffered(StyxRpcChannelQueue *queue, StyxRpcChannelFid *fid, unsigned index)
{
    const StyxRpcGuard *guard = &fid->selection.guards[index];
    if (guard->receive ? !queue->count : queue->count == queue->capacity)
    {
        return 0;
    }
    size_t position = guard->receive ? queue->head : (queue->head + queue->count) % queue->capacity;
    unsigned char *storage = queue->bytes + position * queue->stride;
    size_t ready = guard->receive ? stage(fid, index + 1, queue, storage, queue->sizes[position]) :
        guard->size <= queue->stride ? stage(fid, index + 1, NULL, NULL, 0) : 0;
    if (!ready)
    {
        publish(fid, stage(fid, 255, NULL, NULL, 0));
        return 1;
    }
    if (guard->receive)
    {
        queue->head = (queue->head + 1) % queue->capacity;
        queue->count--;
    }
    else
    {
        memcpy(storage, guard->value, guard->size);
        queue->sizes[position] = guard->size;
        queue->count++;
    }
    publish(fid, ready);
    return 1;
}


/* EN: Choose the first ready guard of one admitted selection.
 * RU: Выбрать первую готовую ветку принятой операции. */
static int ready_one(StyxRpcChannelScheduler *scheduler, StyxRpcChannelFid *fid)
{
    for (size_t i = 0; i < fid->selection.count; ++i)
    {
        StyxRpcChannelQueue *queue = queue_for(scheduler, fid->selection.guards[i].key);
        int done = queue->capacity ? buffered(queue, fid, (unsigned)i) :
            rendezvous(scheduler, queue, fid, (unsigned)i);
        if (done)
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: See channel_scheduler.h. / См. channel_scheduler.h. */
void styxrpc_scheduler_progress(StyxRpcChannelScheduler *scheduler)
{
    if (!scheduler)
    {
        return;
    }
    int progress;
    do
    {
        progress = 0;
        for (size_t i = 0; i < scheduler->used; ++i)
        {
            StyxRpcChannelFid *fid = scheduler->pending[i].fid;
            if (fid->pending)
            {
                progress |= ready_one(scheduler, fid);
            }
        }
    } while (progress);
    uint64_t now = scheduler->clock_ms ? scheduler->clock_ms(scheduler->context) : 0;
    size_t retained = 0;
    for (size_t i = 0; i < scheduler->used; ++i)
    {
        StyxRpcSchedulerEntry entry = scheduler->pending[i];
        if (!entry.fid->pending)
        {
            continue;
        }
        if (entry.fid->selection.mode == STYXRPC_CHANNEL_TRY || (entry.deadline && now >= entry.deadline))
        {
            publish(entry.fid, stage(entry.fid, 0, NULL, NULL, 0));
        }
        else
        {
            scheduler->pending[retained++] = entry;
        }
    }
    scheduler->used = retained;
}


/* EN/RU: See channel_scheduler.h. / См. channel_scheduler.h. */
int styxrpc_scheduler_admit(void *context, StyxRpcChannelFid *fid)
{
    StyxRpcChannelScheduler *scheduler = context;
    if (!scheduler || !fid || !fid->pending || scheduler->used == STYXRPC_CHANNEL_PENDING)
    {
        return 0;
    }
    for (size_t i = 0; i < scheduler->used; ++i)
    {
        if (scheduler->pending[i].fid == fid)
        {
            return 0;
        }
    }
    for (size_t i = 0; i < fid->selection.count; ++i)
    {
        uint32_t key = fid->selection.guards[i].key;
        const StyxRpcChannelQueue *queue = queue_for(scheduler, key);
        const char *shape = fid_shape(fid, key);
        if (!queue || !shape || strcmp(shape, queue->grant.shape))
        {
            return 0;
        }
    }
    uint64_t deadline = 0;
    if (fid->selection.mode == STYXRPC_CHANNEL_WAIT)
    {
        if (!scheduler->clock_ms)
        {
            return 0;
        }
        uint64_t now = scheduler->clock_ms(scheduler->context);
        deadline = now > UINT64_MAX - WAIT_WINDOW ? UINT64_MAX : now + WAIT_WINDOW;
    }
    scheduler->pending[scheduler->used++] = (StyxRpcSchedulerEntry){fid, deadline};
    styxrpc_scheduler_progress(scheduler);
    return 1;
}


/* EN/RU: See channel_scheduler.h. / См. channel_scheduler.h. */
void styxrpc_scheduler_cancel(void *context, StyxRpcChannelFid *fid)
{
    StyxRpcChannelScheduler *scheduler = context;
    if (scheduler)
    {
        for (size_t i = 0; i < scheduler->used; ++i)
        {
            if (scheduler->pending[i].fid == fid)
            {
                --scheduler->used;
                memmove(scheduler->pending + i, scheduler->pending + i + 1,
                    (scheduler->used - i) * sizeof(*scheduler->pending));
                break;
            }
        }
    }
}
