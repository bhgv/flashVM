/* Copyright (c) 2026 bhgv
 * EN: Automatic native journal-to-resource projection, readiness and nonwaiting teardown.
 * RU: Автоматическая native journal-to-resource projection, readiness и nonwaiting teardown.
 */
#include "internal.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

static pthread_mutex_t identities = PTHREAD_MUTEX_INITIALIZER; /* Native ID allocation only. */
static unsigned long long lastidentity; /* Monotonic process-local ID, never wraps. */


/* EN/RU: See internal.h; full pipe already represents readiness, EINTR has no effect.
 * См. internal.h; full pipe уже означает readiness, EINTR не меняет state. */
void styxeventsignal(StyxEvents *events)
{
    char byte = 1;
    ssize_t count;
    do
    {
        count = write(events->wake[1], &byte, 1);
    } while (count < 0 && errno == EINTR);
}


/* EN/RU: See internal.h; producers share lock, so drain is bounded by pipe capacity.
 * См. internal.h; producers разделяют lock, drain ограничен pipe capacity. */
void styxeventlevel(StyxEvents *events)
{
    char bytes[128];
    ssize_t count;
    do
    {
        count = read(events->wake[0], bytes, sizeof(bytes));
    } while (count > 0 || (count < 0 && errno == EINTR));
    if (events->have || events->queued || events->closed)
    {
        styxeventsignal(events);
    }
}


/* EN/RU: See internal.h; final native context cleanup runs outside all resource locks.
 * См. internal.h; final context cleanup выполняется вне всех resource locks. */
void styxeventdrop(StyxEvents *events)
{
    pthread_mutex_lock(&events->lock);
    int last = --events->references == 0;
    pthread_mutex_unlock(&events->lock);
    if (last)
    {
        close(events->wake[0]);
        close(events->wake[1]);
        if (events->policy.release)
        {
            events->policy.release(events->policy.state);
        }
        dvm_changes_destroy(events->source);
        free(events->encoded);
        pthread_mutex_destroy(&events->reader);
        pthread_mutex_destroy(&events->lock);
        free(events);
    }
}


/* EN/RU: Tap holds its own reference until all already admitted callbacks return.
 * Tap удерживает reference до возврата всех already admitted callbacks. */
static void released(void *state)
{
    styxeventdrop(state);
}


/* EN/RU: Project only admitted metadata; stale reports drop, gaps/overflow invalidate selected view.
 * Проецировать admitted metadata; stale reports отбросить, gaps/overflow инвалидируют view. */
static void changed(void *state, const DVM_ChangeEvent *input)
{
    StyxEvents *events = state;
    StyxEvent event = {input->session, input->generation, input->kinds & events->policy.kinds,
                      input->flags, input->resource};
    if (!event.kinds || events->policy.allow(events->policy.state, &event) != 1)
    {
        return;
    }
    pthread_mutex_lock(&events->lock);
    if (!events->closed && (event.session > events->session ||
        (event.session == events->session && event.generation > events->generation)))
    {
        int gap = event.session != events->session || event.generation-events->generation > 1;
        events->session = event.session;
        events->generation = event.generation;
        if (gap || events->queued == EVENT_QUEUE)
        {
            event.kinds = events->policy.kinds;
            event.flags = 0;
            event.resource = 0;
        }
        if (events->queued == EVENT_QUEUE)
        {
            events->first = 0;
            events->queued = 0;
        }
        events->queue[(events->first+events->queued++)%EVENT_QUEUE] = event;
        styxeventsignal(events);
    }
    pthread_mutex_unlock(&events->lock);
}


/* EN/RU: See events.h; owners pin allocation while taking a new public reference.
 * См. events.h; owner удерживает allocation при получении новой reference. */
int styxeventsretain(StyxEvents *events)
{
    if (!events)
    {
        return 0;
    }
    pthread_mutex_lock(&events->lock);
    int valid = events->owners && events->references != (size_t)-1 && events->owners != (size_t)-1;
    if (valid)
    {
        events->references++;
        events->owners++;
    }
    pthread_mutex_unlock(&events->lock);
    return valid;
}


/* EN/RU: See events.h; tap cleanup is outside queue lock and never joins a publisher.
 * См. events.h; tap cleanup вне queue lock, без ожидания publisher. */
void styxeventsfinish(StyxEvents *events)
{
    if (events)
    {
        pthread_mutex_lock(&events->lock);
        events->closed = 1;
        DVM_NoticeTap *tap = events->tap;
        events->tap = NULL;
        styxeventsignal(events);
        pthread_mutex_unlock(&events->lock);
        dvm_notice_unsubscribe(tap);
    }
}


/* EN/RU: See events.h; keep final caller ref until detachment completes, even if tap retires now.
 * См. events.h; держать final caller ref до detach, даже если tap завершается немедленно. */
void styxeventsfree(StyxEvents *events)
{
    if (events)
    {
        pthread_mutex_lock(&events->lock);
        int last = --events->owners == 0;
        pthread_mutex_unlock(&events->lock);
        if (last)
        {
            styxeventsfinish(events);
        }
        styxeventdrop(events);
    }
}


/* EN/RU: See events.h; lifetime is pinned by caller, readiness descriptor is immutable.
 * См. events.h; caller удерживает lifetime, readiness descriptor immutable. */
int styxeventsfd(StyxEvents *events)
{
    return events ? events->wake[0] : -1;
}


/* EN/RU: See events_provider.h; all fallible allocation/descriptor setup precedes subscription.
 * См. events_provider.h; все fallible allocation/descriptor setup предшествуют subscription. */
StyxEvents *styxeventsource(DVM_Changes *source, const StyxEventPolicy *policy)
{
    if (!source || !policy || !policy->kinds || (policy->kinds & ~STYX_EVENT_ALL) ||
        (policy->resources != 0 && policy->resources != 1) || !policy->capacity ||
        policy->capacity > 65536 || !policy->allow || !policy->encode)
    {
        return NULL;
    }
    StyxEvents *events = calloc(1, sizeof(*events));
    if (!events)
    {
        return NULL;
    }
    if (pthread_mutex_init(&events->lock, NULL))
    {
        free(events);
        return NULL;
    }
    if (pthread_mutex_init(&events->reader, NULL))
    {
        pthread_mutex_destroy(&events->lock);
        free(events);
        return NULL;
    }
    events->encoded = malloc(policy->capacity);
    events->wake[0] = events->wake[1] = -1;
    if (!events->encoded || pipe(events->wake) ||
        fcntl(events->wake[0], F_SETFL, O_NONBLOCK) || fcntl(events->wake[1], F_SETFL, O_NONBLOCK) ||
        fcntl(events->wake[0], F_SETFD, FD_CLOEXEC) || fcntl(events->wake[1], F_SETFD, FD_CLOEXEC))
    {
        events->references = 1;
        styxeventdrop(events);
        return NULL;
    }
    pthread_mutex_lock(&identities);
    events->identity = lastidentity == ~0ULL ? 0 : ++lastidentity;
    pthread_mutex_unlock(&identities);
    if (!events->identity || !changesretain(source))
    {
        events->references = 1;
        styxeventdrop(events);
        return NULL;
    }
    events->source = source;
    events->policy = *policy;
    events->references = 2;
    events->owners = 1;
    events->tap = dvm_notice_subscribe(dvm_changes_notices(source), changed, events, released);
    if (!events->tap)
    {
        events->policy.release = NULL;
        events->references = 1;
        styxeventdrop(events);
        return NULL;
    }
    DVM_ChangeCursor cursor = {0};
    DVM_ChangeBatch batch;
    if (dvm_changes_pending(source, &cursor, DVM_CHANGE_ALL, &batch) > 0)
    {
        changed(events, &batch.event);
    }
    return events;
}
