/* Copyright (c) 2026 bhgv
 * EN: Bounded native subscription factory allocation, status and nonwaiting ownership.
 * RU: Создание bounded native фабрики подписок, status и nonwaiting владение.
 */
#include "internal.h"
#include <stdlib.h>


/* EN/RU: See internal.h; final policy release waits only through references, never a join.
 * См. internal.h; final policy release защищён references, без join. */
void styxsubscriptionsdrop(StyxSubscriptions *factory)
{
    pthread_mutex_lock(&factory->lock);
    int last = --factory->references == 0;
    pthread_mutex_unlock(&factory->lock);
    if (last)
    {
        if (factory->policy.release)
        {
            factory->policy.release(factory->policy.state);
        }
        dvm_changes_destroy(factory->journal);
        pthread_mutex_destroy(&factory->lock);
        free(factory);
    }
}


/* EN/RU: See subscriptions.h; validate policy before adoption, no initial subscription/callback.
 * См. subscriptions.h; validate policy до adoption, без initial subscription/callback. */
StyxSubscriptions *styxsubscriptions(DVM_Changes *journal, const StyxEventPolicy *policy, size_t limit)
{
    if (!journal || !policy || !limit || limit > 1024 || !policy->kinds ||
        (policy->kinds & ~STYX_EVENT_ALL) || (policy->resources != 0 && policy->resources != 1) ||
        !policy->capacity || policy->capacity > 65536 || !policy->allow || !policy->encode)
    {
        return NULL;
    }
    StyxSubscriptions *factory = calloc(1, sizeof(*factory));
    if (!factory)
    {
        return NULL;
    }
    if (pthread_mutex_init(&factory->lock, NULL))
    {
        free(factory);
        return NULL;
    }
    if (!changesretain(journal))
    {
        pthread_mutex_destroy(&factory->lock);
        free(factory);
        return NULL;
    }
    factory->owners = 1;
    factory->references = 1;
    factory->info.limit = limit;
    factory->journal = journal;
    factory->policy = *policy;
    return factory;
}


/* EN/RU: See subscriptions.h; borrowed capability must stay pinned throughout retain.
 * См. subscriptions.h; borrowed capability должна оставаться pinned на время retain. */
int styxsubscriptionsretain(StyxSubscriptions *factory)
{
    if (!factory)
    {
        return 0;
    }
    pthread_mutex_lock(&factory->lock);
    int valid = factory->owners && factory->owners != (size_t)-1 && factory->references != (size_t)-1;
    if (valid)
    {
        factory->owners++;
        factory->references++;
    }
    pthread_mutex_unlock(&factory->lock);
    return valid;
}


/* EN/RU: See subscriptions.h; admission is linearized under mutex, no source callbacks.
 * См. subscriptions.h; admission линеаризован под mutex, без source callbacks. */
void styxsubscriptionsclose(StyxSubscriptions *factory)
{
    if (factory)
    {
        pthread_mutex_lock(&factory->lock);
        factory->info.closed = 1;
        pthread_mutex_unlock(&factory->lock);
    }
}


/* EN/RU: See subscriptions.h; keep final owner reference through admission closure.
 * См. subscriptions.h; сохранить final owner reference до закрытия admission. */
void styxsubscriptionsfree(StyxSubscriptions *factory)
{
    if (factory)
    {
        pthread_mutex_lock(&factory->lock);
        if (!--factory->owners)
        {
            factory->info.closed = 1;
        }
        pthread_mutex_unlock(&factory->lock);
        styxsubscriptionsdrop(factory);
    }
}


/* EN/RU: See subscriptions.h; output contains no borrowed pointers.
 * См. subscriptions.h; output не содержит borrowed pointers. */
int styxsubscriptionsinfo(StyxSubscriptions *factory, StyxSubscriptionInfo *output)
{
    if (!factory || !output)
    {
        return 0;
    }
    pthread_mutex_lock(&factory->lock);
    *output = factory->info;
    pthread_mutex_unlock(&factory->lock);
    return 1;
}
