/* Copyright (c) 2026 bhgv
 * EN: Nonwaiting root-service callbacks pinned independently through native replacement and broker dispatch.
 * RU: Nonwaiting root-service callbacks с независимым pin при replacement и broker dispatch.
 */
#include "../internal.h"


/* EN/RU: See service/internal.h; caller serializes references with hub lock.
 * См. service/internal.h; caller сериализует references под hub lock.
 */
ScopeServiceHandler *scopeservicedrop(ScopeServiceHandler *handler)
{
    return handler && !--handler->references ? handler : NULL;
}


/* EN/RU: See service/internal.h; native adopted state is released only after the final callback.
 * См. service/internal.h; adopted native state освобождается лишь после final callback.
 */
void scopeservicefree(ScopeServiceHandler *handler)
{
    if (handler)
    {
        if (handler->release)
        {
            handler->release(handler->state);
        }
        free(handler);
    }
}


/* EN/RU: See service.h; allocate before state adoption, never run release under hub lock.
 * См. service.h; allocation до state adoption, release никогда не работает под hub lock.
 */
int styxscopeservicehandler(StyxScope *root, StyxScopeService receive, void *state, void (*release)(void *))
{
    if (!root || root->parent || root->leaf || (!receive && (state || release)))
    {
        return 0;
    }
    ScopeServiceHandler *handler = receive ? calloc(1, sizeof(*handler)) : NULL; /* Private until replacement succeeds. */
    if (receive && !handler)
    {
        return 0;
    }
    if (handler)
    {
        handler->references = 1;
        handler->receive = receive;
        handler->state = state;
        handler->release = release;
    }
    ScopeHub *hub = root->hub; /* Root pinned through setup by native host. */
    pthread_mutex_lock(&hub->lock);
    if (hub->reason)
    {
        pthread_mutex_unlock(&hub->lock);
        free(handler);
        return 0;
    }
    ScopeServiceHandler *previous = scopeservicedrop(hub->handler); /* Final context, or retained by active callback. */
    hub->handler = handler;
    pthread_mutex_unlock(&hub->lock);
    scopeservicefree(previous);
    return 1;
}


/* EN/RU: See service/internal.h; only the broker calls this for verified transport root0 records.
 * См. service/internal.h; лишь broker вызывает для verified transport root0 records.
 */
int scopeservicehandle(ScopeHub *hub, const void *bytes, size_t size)
{
    pthread_mutex_lock(&hub->lock);
    ScopeServiceHandler *handler = hub->handler; /* Independently pinned while invoked outside locks. */
    if (handler)
    {
        handler->references++;
    }
    int live = !hub->reason; /* Native terminal state supersedes service dispatch. */
    pthread_mutex_unlock(&hub->lock);
    int result = STYX_SCOPE_PASS; /* Unknown root services retain ordinary native inbox delivery. */
    if (!live)
    {
        result = STYX_SCOPE_SERVICE_ERROR;
    }
    else if (handler)
    {
        result = handler->receive(handler->state, hub->root, bytes, size);
        if (result != STYX_SCOPE_PASS && result != STYX_SCOPE_HANDLED)
        {
            result = STYX_SCOPE_SERVICE_ERROR;
        }
    }
    pthread_mutex_lock(&hub->lock);
    ScopeServiceHandler *final = scopeservicedrop(handler); /* Registered owner may already have been replaced. */
    pthread_mutex_unlock(&hub->lock);
    scopeservicefree(final);
    return result;
}
