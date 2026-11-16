/* Copyright (c) 2026 bhgv
 * EN: Serialized one-record formatting/copy with a head retained across failed reads.
 * RU: Serialized one-record formatting/copy с head, сохраняемым при неуспешном read.
 */
#include "internal.h"
#include <string.h>


/* EN/RU: See events.h; formatter never holds the publisher queue lock or runs on publisher.
 * См. events.h; formatter не удерживает queue lock и не выполняется на publisher. */
int styxeventsread(StyxEvents *events, void *bytes, size_t size, size_t *count)
{
    if (count)
    {
        *count = 0;
    }
    if (!events || !count || (!bytes && size))
    {
        return STYX_EVENT_ERROR;
    }
    if (!size)
    {
        return STYX_EVENT_EMPTY;
    }
    pthread_mutex_lock(&events->reader);
    int result = STYX_EVENT_EMPTY;
    for (unsigned scanned = 0; scanned <= EVENT_QUEUE; scanned++)
    {
        pthread_mutex_lock(&events->lock);
        if (!events->have && events->queued)
        {
            events->head = events->queue[events->first];
            events->first = (events->first+1)%EVENT_QUEUE;
            events->queued--;
            events->have = 1;
        }
        int have = events->have;
        int closed = events->closed;
        StyxEvent event = events->head;
        pthread_mutex_unlock(&events->lock);
        if (!have)
        {
            result = closed ? STYX_EVENT_EOF : STYX_EVENT_EMPTY;
            break;
        }
        if (events->policy.allow(events->policy.state, &event) != 1)
        {
            events->length = 0;
            pthread_mutex_lock(&events->lock);
            events->have = 0;
            pthread_mutex_unlock(&events->lock);
            continue;
        }
        if (!events->policy.resources)
        {
            event.flags = 0;
            event.resource = 0;
        }
        if (!events->length)
        {
            size_t length = 0;
            if (events->policy.encode(events->policy.state, &event, events->encoded,
                events->policy.capacity, &length) == 1 && length && length <= events->policy.capacity)
            {
                events->length = length;
            }
        }
        if (!events->length)
        {
            result = STYX_EVENT_ERROR;
        }
        else if (events->length > size)
        {
            result = STYX_EVENT_SMALL;
        }
        else
        {
            memcpy(bytes, events->encoded, events->length);
            *count = events->length;
            events->length = 0;
            pthread_mutex_lock(&events->lock);
            events->have = 0;
            pthread_mutex_unlock(&events->lock);
            result = STYX_EVENT_RECORD;
        }
        break;
    }
    pthread_mutex_lock(&events->lock);
    if (result == STYX_EVENT_EMPTY && events->closed && !events->have && !events->queued)
    {
        result = STYX_EVENT_EOF;
    }
    styxeventlevel(events);
    pthread_mutex_unlock(&events->lock);
    pthread_mutex_unlock(&events->reader);
    return result;
}
