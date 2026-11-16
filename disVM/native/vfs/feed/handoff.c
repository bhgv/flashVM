/* Copyright (c) 2026 bhgv
 * EN: Native-only staged activation and cancellation-aware predecessor handoff.
 * RU: Native-only staged activation и cancellation-aware predecessor handoff.
 */
#include "internal.h"


/* EN/RU: See internal.h; prepared worker cannot touch decoder state before this commit.
 * См. internal.h; prepared worker не касается decoder state до этого commit. */
void styxfeedactivate(StyxFeed *feed)
{
    pthread_mutex_lock(&feed->lock);
    feed->adopted = 1;
    feed->ready = 1;
    pthread_cond_broadcast(&feed->gate);
    pthread_mutex_unlock(&feed->lock);
}


/* EN/RU: See internal.h; stopped standby points directly at earlier outstanding watch, not a chain.
 * См. internal.h; stopped standby указывает прямо на earlier outstanding watch, не chain. */
int styxfeedfollow(StyxFeed *feed, StyxFeed *previous)
{
    if (!previous)
    {
        return 1;
    }
    pthread_mutex_lock(&previous->lock);
    StyxFeed *dependency = previous->previous;
    if (dependency)
    {
        if (!styxfeedretain(dependency))
        {
            pthread_mutex_unlock(&previous->lock);
            return 0;
        }
    }
    else
    {
        dependency = previous;
        if (dependency->owners == (size_t)-1 || dependency->references == (size_t)-1)
        {
            pthread_mutex_unlock(&previous->lock);
            return 0;
        }
        dependency->owners++;
        dependency->references++;
    }
    previous->stopping = 1;
    styxcancelrequest(previous->cancel);
    pthread_cond_broadcast(&previous->gate);
    if (previous->previous)
    {
        pthread_mutex_lock(&previous->previous->lock);
        pthread_cond_broadcast(&previous->previous->gate);
        pthread_mutex_unlock(&previous->previous->lock);
    }
    pthread_mutex_unlock(&previous->lock);
    pthread_mutex_lock(&feed->lock);
    feed->previous = dependency;
    pthread_mutex_unlock(&feed->lock);
    return 1;
}


/* EN/RU: See internal.h; lock order is newer feed then older dependency; waits hold no newer lock.
 * См. internal.h; порядок locks newer->older, ожидание не удерживает newer lock. */
int styxfeedbegin(StyxFeed *feed)
{
    pthread_mutex_lock(&feed->lock);
    while (!feed->ready && !feed->stopping)
    {
        pthread_cond_wait(&feed->gate, &feed->lock);
    }
    while (!feed->stopping && feed->previous)
    {
        StyxFeed *previous = feed->previous;
        pthread_mutex_lock(&previous->lock);
        if (previous->info.finished)
        {
            pthread_mutex_unlock(&previous->lock);
            break;
        }
        pthread_mutex_unlock(&feed->lock);
        pthread_cond_wait(&previous->gate, &previous->lock);
        pthread_mutex_unlock(&previous->lock);
        pthread_mutex_lock(&feed->lock);
    }
    int ready = !feed->stopping;
    pthread_mutex_unlock(&feed->lock);
    return ready;
}


/* EN/RU: See internal.h; detach under newer lock, release native predecessor outside all locks.
 * См. internal.h; detach под newer lock, release predecessor вне всех locks. */
void styxfeedpreviousfree(StyxFeed *feed)
{
    pthread_mutex_lock(&feed->lock);
    StyxFeed *previous = feed->previous;
    feed->previous = NULL;
    pthread_mutex_unlock(&feed->lock);
    styxfeedfree(previous);
}
