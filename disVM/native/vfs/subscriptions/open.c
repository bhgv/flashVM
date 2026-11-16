/* Copyright (c) 2026 bhgv
 * EN: Independent inbox admission and shared native policy forwarding outside factory locks.
 * RU: Admission независимых inbox и вызов общей native policy вне factory locks.
 */
#include "internal.h"


/* EN/RU: Forward admission/delivery filter with retained factory state, no factory lock.
 * Передать admission/delivery filter с удержанным factory state, без factory lock. */
static int allow(void *state, const StyxEvent *event)
{
    StyxSubscriptions *factory = state;
    return factory->policy.allow(factory->policy.state, event);
}


/* EN/RU: Forward one stream's formatting; distinct streams may encode concurrently.
 * Передать formatting одного stream; разные streams могут кодироваться одновременно. */
static int encode(void *state, const StyxEvent *event, void *bytes, size_t size, size_t *count)
{
    StyxSubscriptions *factory = state;
    return factory->policy.encode(factory->policy.state, event, bytes, size, count);
}


/* EN/RU: Release reserved quota only after inbox callbacks/owners retire, including failed setup.
 * Освободить reserved quota после retirement inbox callbacks/owners, включая failed setup. */
static void release(void *state)
{
    StyxSubscriptions *factory = state;
    pthread_mutex_lock(&factory->lock);
    factory->info.active--;
    pthread_mutex_unlock(&factory->lock);
    styxsubscriptionsdrop(factory);
}


/* EN/RU: See subscriptions.h; reserve before allocation and roll back every unsuccessful setup.
 * См. subscriptions.h; reserve до allocation и rollback каждого unsuccessful setup. */
StyxEvents *styxsubscribe(StyxSubscriptions *factory)
{
    if (!factory)
    {
        return NULL;
    }
    pthread_mutex_lock(&factory->lock);
    int valid = !factory->info.closed && factory->info.active < factory->info.limit &&
        factory->references != (size_t)-1;
    if (valid)
    {
        factory->info.active++;
        factory->references++;
    }
    pthread_mutex_unlock(&factory->lock);
    if (!valid)
    {
        return NULL;
    }
    StyxEventPolicy policy = factory->policy;
    policy.allow = allow;
    policy.encode = encode;
    policy.state = factory;
    policy.release = release;
    StyxEvents *events = styxeventsource(factory->journal, &policy);
    if (!events)
    {
        release(factory);
    }
    return events;
}
