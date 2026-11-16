/* Copyright (c) 2026 bhgv
 * EN: Native remote event reads, bounded incremental decoding and local journal publication.
 * RU: Native remote event reads, bounded incremental decoding и local journal publication.
 */
#include "internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Validate scalar record and fence remote epochs before assigning unrelated local generation.
 * Проверить scalar record и remote epochs до назначения независимой local generation. */
static int publish(StyxFeed *feed, StyxEvent event)
{
    if (!event.session || !event.generation || (event.kinds & ~STYX_EVENT_ALL) ||
        (event.flags & ~STYX_EVENT_RESOURCE) || ((event.flags & STYX_EVENT_RESOURCE) &&
        (!event.kinds || (event.kinds & (STYX_EVENT_CONNECTION|STYX_EVENT_LOAD|STYX_EVENT_NETWORK)))))
    {
        return 0;
    }
    pthread_mutex_lock(&feed->lock);
    int newer = !feed->stopping && (event.session > feed->info.session ||
        (event.session == feed->info.session && event.generation > feed->info.generation));
    int gap = event.session != feed->info.session || event.generation-feed->info.generation > 1;
    if (newer)
    {
        feed->info.session = event.session;
        feed->info.generation = event.generation;
        if (feed->info.records != ~0ULL)
        {
            feed->info.records++;
        }
    }
    pthread_mutex_unlock(&feed->lock);
    if (!newer)
    {
        return 1;
    }
    if (gap)
    {
        event.kinds = feed->config.kinds;
        event.flags = 0;
        event.resource = 0;
    }
    else
    {
        event.kinds = (event.kinds ? event.kinds : STYX_EVENT_ALL) & feed->config.kinds;
    }
    if (!feed->config.resources)
    {
        event.flags = 0;
        event.resource = 0;
    }
    return !event.kinds || changespost(feed->target, event.kinds, event.flags, event.resource) != 0;
}


/* EN/RU: Process one record per decoder iteration; malformed progress never loops or reads past buffer.
 * Обрабатывать по записи на decoder iteration; invalid progress не зацикливается/не выходит за buffer. */
static int decode(StyxFeed *feed)
{
    while (feed->used)
    {
        StyxEvent event = {0};
        size_t consumed = 0;
        int result = feed->config.decode(feed->config.state, feed->bytes, feed->used, &event, &consumed);
        if (!result)
        {
            return consumed == 0 && feed->used < feed->config.capacity;
        }
        if (result != 1 || !consumed || consumed > feed->used || !publish(feed, event))
        {
            return 0;
        }
        feed->used -= consumed;
        memmove(feed->bytes, feed->bytes+consumed, feed->used);
        pthread_mutex_lock(&feed->lock);
        int stopping = feed->stopping;
        pthread_mutex_unlock(&feed->lock);
        if (stopping)
        {
            break;
        }
    }
    return 1;
}


/* EN/RU: See internal.h; this dedicated native I/O thread never owns a Prog/REG or provider pointer.
 * См. internal.h; dedicated native I/O thread не владеет Prog/REG либо provider pointer. */
void *styxfeedrun(void *state)
{
    StyxFeed *feed = state;
    unsigned long long offset = 0;
    int final = STYX_FEED_FAILED;
    int started = styxfeedbegin(feed);
    if (started)
    {
        styxfeedpreviousfree(feed);
    }
    for (; started;)
    {
        StyxCancel *cancel = styxcancelcreate();
        if (!cancel)
        {
            break;
        }
        pthread_mutex_lock(&feed->lock);
        int stopping = feed->stopping;
        feed->cancel = stopping ? NULL : cancel;
        pthread_mutex_unlock(&feed->lock);
        if (stopping)
        {
            styxcancelfree(cancel);
            break;
        }
        StyxTransfer transfer;
        styxclientreadwatch(feed->fid, offset, feed->bytes+feed->used, feed->config.capacity-feed->used, cancel, &transfer);
        pthread_mutex_lock(&feed->lock);
        feed->cancel = NULL;
        stopping = feed->stopping;
        pthread_mutex_unlock(&feed->lock);
        styxcancelfree(cancel);
        if (stopping || !transfer.success || transfer.uncertain)
        {
            break;
        }
        if (!transfer.count)
        {
            final = feed->used ? STYX_FEED_FAILED : STYX_FEED_EOF;
            break;
        }
        if (transfer.count > 0x7fffffffffffffffULL-offset)
        {
            break;
        }
        offset += transfer.count;
        feed->used += transfer.count;
        if (!decode(feed))
        {
            break;
        }
    }
    pthread_mutex_lock(&feed->lock);
    int stopping = feed->stopping;
    feed->info.state = stopping ? STYX_FEED_STOPPED : final;
    pthread_mutex_unlock(&feed->lock);
    if (!stopping)
    {
        changespost(feed->target, feed->config.kinds|STYX_EVENT_CONNECTION, 0, 0);
    }
    styxclientclunk(feed->fid);
    feed->fid = NULL;
    styxclientfree(feed->client);
    feed->client = NULL;
    dvm_changes_destroy(feed->target);
    feed->target = NULL;
    free(feed->bytes);
    feed->bytes = NULL;
    pthread_mutex_lock(&feed->lock);
    feed->info.finished = 1;
    pthread_cond_broadcast(&feed->gate);
    pthread_mutex_unlock(&feed->lock);
    styxfeeddrop(feed);
    return NULL;
}
