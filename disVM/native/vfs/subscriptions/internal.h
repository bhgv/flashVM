/* Copyright (c) 2026 bhgv
 * EN: Native subscriber quota and shared immutable policy lifetime, no provider/source cycle.
 * RU: Native quota подписчиков и lifetime общей immutable policy, без provider/source cycle.
 */
#ifndef STYX_SUBSCRIPTIONS_INTERNAL_H
#define STYX_SUBSCRIPTIONS_INTERNAL_H
#include "../subscriptions.h"
#include <pthread.h>


/* EN/RU: Each independent inbox owns one private reference until its callbacks retire.
 * Каждый независимый inbox владеет private reference до завершения callbacks. */
struct StyxSubscriptions
{
    pthread_mutex_t lock; /* Protect owners, references and admission status. */
    size_t owners; /* Explicit native factory capabilities. */
    size_t references; /* Owners plus reserved/live/retiring subscriber policies. */
    StyxSubscriptionInfo info; /* Scalar quota/admission snapshot. */
    DVM_Changes *journal; /* Owned journal pin independent of any provider. */
    StyxEventPolicy policy; /* Shared adopted callbacks/context; immutable after publication. */
};


/* EN/RU: Consume private reference; final cleanup outside locks, never joins subscribers.
 * Поглотить private reference; final cleanup вне locks, без join subscribers. */
void styxsubscriptionsdrop(StyxSubscriptions *factory);
#endif
