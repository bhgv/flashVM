/* Copyright (c) 2026 bhgv
 * EN: Native feed construction, nonwaiting stop and independent control lifetime.
 * RU: Native feed construction, nonwaiting stop и independent control lifetime.
 */
#include "internal.h"
#include <stdlib.h>


/* EN/RU: See internal.h; final cleanup performs no worker join or pending-read wait.
 * См. internal.h; final cleanup без worker join/pending-read wait. */
void styxfeeddrop(StyxFeed *feed)
{
    pthread_mutex_lock(&feed->lock);
    int last = --feed->references == 0;
    pthread_mutex_unlock(&feed->lock);
    if (last)
    {
        styxclientclunk(feed->fid);
        styxclientfree(feed->client);
        dvm_changes_destroy(feed->target);
        if (feed->config.release)
        {
            feed->config.release(feed->config.state);
        }
        free(feed->bytes);
        pthread_mutex_destroy(&feed->lock);
        free(feed);
    }
}


/* EN/RU: See feed.h; caller already pins a native owner through the retain.
 * См. feed.h; caller удерживает native owner на время retain. */
int styxfeedretain(StyxFeed *feed)
{
    if (!feed)
    {
        return 0;
    }
    pthread_mutex_lock(&feed->lock);
    int valid = feed->owners && feed->owners != (size_t)-1 && feed->references != (size_t)-1;
    if (valid)
    {
        feed->owners++;
        feed->references++;
    }
    pthread_mutex_unlock(&feed->lock);
    return valid;
}


/* EN/RU: See feed.h; token remains pinned under handoff lock while native wake is requested.
 * См. feed.h; token удержан под handoff lock на время native wake request. */
void styxfeedstop(StyxFeed *feed)
{
    if (feed)
    {
        pthread_mutex_lock(&feed->lock);
        if (!feed->info.finished && feed->info.state == STYX_FEED_ACTIVE)
        {
            feed->stopping = 1;
            styxcancelrequest(feed->cancel);
        }
        pthread_mutex_unlock(&feed->lock);
    }
}


/* EN/RU: See feed.h; final owner keeps its ref through stop even if worker retires immediately.
 * См. feed.h; final owner держит ref до stop, даже если worker немедленно завершается. */
void styxfeedfree(StyxFeed *feed)
{
    if (feed)
    {
        pthread_mutex_lock(&feed->lock);
        int last = --feed->owners == 0;
        pthread_mutex_unlock(&feed->lock);
        if (last)
        {
            styxfeedstop(feed);
        }
        styxfeeddrop(feed);
    }
}


/* EN/RU: See feed.h; native snapshot contains no mutable borrowed pointers.
 * См. feed.h; native snapshot не содержит mutable borrowed pointers. */
int styxfeedinfo(StyxFeed *feed, StyxFeedInfo *output)
{
    if (!feed || !output)
    {
        return 0;
    }
    pthread_mutex_lock(&feed->lock);
    *output = feed->info;
    pthread_mutex_unlock(&feed->lock);
    return 1;
}


/* EN/RU: See internal.h; setup is native I/O, all fallible setup precedes worker publication.
 * См. internal.h; setup — native I/O, все fallible шаги до worker publication. */
StyxFeed *styxfeedcreate(StyxClient *client, DVM_Changes *journal, const StyxFeedConfig *config)
{
    if (!config || !config->path || !config->decode || !config->capacity || config->capacity > 65536 ||
        !config->kinds || (config->kinds & ~STYX_EVENT_ALL) || (config->resources != 0 && config->resources != 1) ||
        !journal || !styxclientwatchable(client))
    {
        return NULL;
    }
    StyxFeed *feed = calloc(1, sizeof(*feed));
    if (!feed)
    {
        return NULL;
    }
    if (pthread_mutex_init(&feed->lock, NULL))
    {
        free(feed);
        return NULL;
    }
    feed->references = 1;
    feed->owners = 1;
    feed->bytes = malloc(config->capacity);
    if (!feed->bytes || !changesretain(journal))
    {
        styxfeeddrop(feed);
        return NULL;
    }
    feed->target = journal;
    if (!styxclientretain(client))
    {
        styxfeeddrop(feed);
        return NULL;
    }
    feed->client = client;
    feed->fid = styxclientwalk(client, config->path);
    StyxMeta meta;
    if (!feed->fid || !styxclientstat(feed->fid, &meta) || meta.directory || !styxclientopen(feed->fid, 0))
    {
        styxfeeddrop(feed);
        return NULL;
    }
    feed->config = *config;
    feed->config.path = NULL;
    feed->info.state = STYX_FEED_ACTIVE;
    feed->references++;
    pthread_t thread;
    pthread_attr_t attr;
    int initialized = !pthread_attr_init(&attr);
    int ready = initialized && !pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (!ready || pthread_create(&thread, &attr, styxfeedrun, feed))
    {
        if (initialized)
        {
            pthread_attr_destroy(&attr);
        }
        feed->config.release = NULL;
        feed->references--;
        styxfeeddrop(feed);
        return NULL;
    }
    pthread_attr_destroy(&attr);
    return feed;
}
