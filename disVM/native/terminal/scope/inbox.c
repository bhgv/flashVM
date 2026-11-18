/* Copyright (c) 2026 bhgv
 * EN: Client-local and native-service blocking waits and one-record receives, without transport access.
 * RU: Client-local и native-service blocking waits и приём по одной записи без transport access.
 */
#include "internal.h"


/* EN/RU: Copy one resolved recipient record; SMALL preserves head/tag, caller pins scope and output.
 * Скопировать одну resolved recipient record; SMALL сохраняет head/tag, scope/output pinned caller.
 */
static int next(StyxScope *scope, unsigned long *tag, void *bytes, size_t capacity, size_t *size)
{
    if (!scope || !tag || !size || (!bytes && capacity))
    {
        return STYX_CONTROL_END;
    }
    ScopeHub *hub = scope->hub; /* Root remains pinned until every external user retires. */
    pthread_mutex_lock(&hub->lock);
    int status = hub->reason ? STYX_CONTROL_END : STYX_CONTROL_EMPTY; /* Empty outcome only. */
    ScopeMessage *message = scope->head; /* Borrowed committed FIFO head. */
    if (message)
    {
        *size = message->size;
        status = STYX_CONTROL_SMALL;
        if (message->size <= capacity)
        {
            if (message->size)
            {
                memcpy(bytes, message->bytes, message->size);
            }
            *tag = message->tag;
            scope->head = message->next;
            if (!scope->head)
            {
                scope->tail = NULL;
            }
            scope->count--;
            hub->queued--;
            free(message);
            status = STYX_CONTROL_RECORD;
        }
    }
    pthread_mutex_unlock(&hub->lock);
    return status;
}


/* EN/RU: Block only the requesting inbox consumer under one monotonic budget; no queue consumption.
 * Блокировать только consumer своей inbox под одним monotonic budget, без queue consumption.
 */
static int wait(StyxScope *scope, unsigned timeout)
{
    if (!scope || timeout > 120000)
    {
        return 0;
    }
    struct timespec deadline; /* Single original timeout budget. */
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return 0;
    }
    deadline.tv_sec += timeout / 1000;
    deadline.tv_nsec += (long)(timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    ScopeHub *hub = scope->hub; /* Borrowed root owner through this blocking call. */
    pthread_mutex_lock(&hub->lock);
    while (!scope->count && !hub->reason && timeout)
    {
        if (pthread_cond_timedwait(&hub->changed, &hub->lock, &deadline))
        {
            break;
        }
    }
    int ready = scope->count != 0; /* Accepted records survive stop/remap. */
    pthread_mutex_unlock(&hub->lock);
    return ready;
}


/* EN/RU: See scope.h; client has no API accepting a parent or outer tag selection.
 * См. scope.h; client API не принимает parent либо выбор outer tag.
 */
int styxscopeclientnext(StyxScopeClient *client, unsigned long *tag, void *bytes, size_t capacity, size_t *size)
{
    return next(client ? client->scope : NULL, tag, bytes, capacity, size);
}


/* EN/RU: See scope.h; only this client's readiness is observed, no broker/other-client wait.
 * См. scope.h; наблюдается readiness лишь этого клиента, не broker/других клиентов.
 */
int styxscopeclientwait(StyxScopeClient *client, unsigned timeout)
{
    return wait(client ? client->scope : NULL, timeout);
}


/* EN/RU: See scope.h; root/group host services are inaccessible through application inbox capabilities.
 * См. scope.h; root/group host services недоступны через application inbox capabilities.
 */
int styxscopeservicenext(StyxScope *scope, void *bytes, size_t capacity, size_t *size)
{
    unsigned long tag; /* Native-only constant-zero service metadata. */
    return next(scope && !scope->leaf ? scope : NULL, &tag, bytes, capacity, size);
}


/* EN/RU: See scope.h; service owner blocks independently of all application recipients.
 * См. scope.h; service owner блокируется независимо от application recipients.
 */
int styxscopeservicewait(StyxScope *scope, unsigned timeout)
{
    return wait(scope && !scope->leaf ? scope : NULL, timeout);
}
