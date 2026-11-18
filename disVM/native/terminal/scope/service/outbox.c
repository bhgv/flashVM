/* Copyright (c) 2026 bhgv
 * EN: Dynamic native root-service output; broker admission never waits for the underlying bounded TX ring.
 * RU: Dynamic native root-service output, broker admission не ждёт bounded TX ring транспорта.
 */
#include "../internal.h"


/* EN/RU: See service/internal.h; root tag0 costs one canonical body byte, no new envelope/schema.
 * См. service/internal.h; root tag0 занимает один body byte, без нового envelope/schema.
 */
ScopeServiceMessage *scopeservicecopy(ScopeHub *hub, const void *bytes, size_t size)
{
    if ((size && !bytes) || size >= hub->limit || size > (size_t)-1 - sizeof(ScopeServiceMessage))
    {
        return NULL;
    }
    ScopeServiceMessage *message = malloc(sizeof(*message) + size); /* Exact copied native outbox node. */
    if (message)
    {
        message->next = NULL;
        message->size = size;
        if (size)
        {
            memcpy(message->bytes, bytes, size);
        }
    }
    return message;
}


/* EN/RU: See service/internal.h; caller preallocates and checks live hub/accounting under the same lock.
 * См. service/internal.h; caller preallocates и проверяет live hub/accounting под тем же lock.
 */
void scopeservicequeue(ScopeHub *hub, ScopeServiceMessage *message)
{
    if (hub->service_tail)
    {
        hub->service_tail->next = message;
    }
    else
    {
        hub->service_head = message;
    }
    hub->service_tail = message;
    hub->service_count++;
    pthread_cond_broadcast(&hub->changed);
}


/* EN/RU: See service.h; copy first, atomically enqueue without touching native transport while holding hub lock.
 * См. service.h; сначала copy, затем atomic enqueue без transport work под hub lock.
 */
int styxscopeservicesend(StyxScope *root, const void *bytes, size_t size)
{
    if (!root || root->parent || root->leaf)
    {
        return 0;
    }
    ScopeHub *hub = root->hub; /* Caller retains native root authority through return. */
    ScopeServiceMessage *message = scopeservicecopy(hub, bytes, size); /* Private until enqueue succeeds. */
    if (!message)
    {
        return 0;
    }
    pthread_mutex_lock(&hub->lock);
    int valid = !hub->reason && hub->service_count != (size_t)-1; /* No partial service acceptance. */
    if (valid)
    {
        scopeservicequeue(hub, message);
    }
    pthread_mutex_unlock(&hub->lock);
    if (!valid)
    {
        free(message);
    }
    return valid;
}


/* EN/RU: See service/internal.h; keep head pinned while native TX admission waits, unrelated application tags remain independent.
 * См. service/internal.h; head удерживается во время native TX wait, unrelated application tags независимы.
 */
void *scopeservicerun(void *state)
{
    ScopeHub *hub = state; /* Owned by root through worker join. */
    for (;;)
    {
        pthread_mutex_lock(&hub->lock);
        while (!hub->reason && !hub->service_head)
        {
            pthread_cond_wait(&hub->changed, &hub->lock);
        }
        ScopeServiceMessage *message = hub->service_head; /* Pinned owned head until admission/retirement. */
        int ended = hub->reason; /* No native transport call after observed retirement. */
        pthread_mutex_unlock(&hub->lock);
        if (ended)
        {
            break;
        }
        if (!styxbidircontrolsendwait(hub->control, 0, message->bytes, message->size, 120000, NULL))
        {
            scopeend(hub, STYX_CONTROL_WRITE_FAILED);
            break;
        }
        pthread_mutex_lock(&hub->lock);
        hub->service_head = message->next;
        if (!hub->service_head)
        {
            hub->service_tail = NULL;
        }
        hub->service_count--;
        pthread_cond_broadcast(&hub->changed);
        pthread_mutex_unlock(&hub->lock);
        free(message);
    }
    return NULL;
}
