/* Copyright (c) 2026 bhgv
 * EN: Native client TX waits on its profile/counter predicate, never on another client's queue.
 * RU: Native TX-ожидание своего профиля/счётчика, независимо от очереди другого клиента.
 */
#include "internal.h"
#include "wait_cancel.h"


/* EN/RU: See broker.h; copy only once, preserving the original deadline across unrelated notifications.
 * См. broker.h; копировать однократно, сохраняя исходный deadline при посторонних уведомлениях.
 */
int styxorderedclientsendwaitcancel(StyxOrderedClient *client, const void *bytes, size_t size, unsigned timeout, DVM_Cancel *source)
{
    if (!client || (!bytes && size) || timeout > 120000)
    {
        return STYX_ORDERED_END;
    }
    if (size > styxorderedclientceiling(client))
    {
        return STYX_ORDERED_SIZE;
    }
    struct timespec deadline; /* One total monotonic wait budget. */
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return STYX_ORDERED_END;
    }
    deadline.tv_sec += timeout / 1000;
    deadline.tv_nsec += (long)(timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    StyxOrdered *peer = client->peer; /* Borrowed until wait/admission/terminal handling completes. */
    OrderedWaitCancel *wait; /* Optional cancellation predicate, fenced through return. */
    if (!orderedwaitattach(peer, source, &wait))
    {
        return STYX_ORDERED_NOMEM;
    }
    pthread_mutex_lock(&peer->lock);
    int result = STYX_ORDERED_WAIT; /* Pending by default, never consumes an ordinal on timeout. */
    int failure = 0; /* Propagate terminal reasons only after releasing peer lock. */
    for (;;)
    {
        if (wait && wait->cancelled)
        {
            result = STYX_ORDERED_CANCELLED;
            break;
        }
        if (peer->reason)
        {
            result = STYX_ORDERED_END;
            break;
        }
        if ((peer->active[STYX_ORDER_TX] && !client->tx_pending) ||
            (!peer->tx_pending && peer->answer.command == FVM_SORDER_REJECT))
        {
            result = orderedsendlocked(client, bytes, size, &failure);
            break;
        }
        if (!timeout || pthread_cond_timedwait(&peer->changed, &peer->lock, &deadline))
        {
            break;
        }
    }
    pthread_mutex_unlock(&peer->lock);
    orderedwaitdetach(wait);
    if (failure)
    {
        orderedend(peer, failure);
    }
    return result;
}


/* EN/RU: See broker.h; preserve the existing no-cancellation API and detailed status values.
 * См. broker.h; сохранить прежний API без отмены и подробные значения статуса.
 */
int styxorderedclientsendwait(StyxOrderedClient *client, const void *bytes, size_t size, unsigned timeout)
{
    return styxorderedclientsendwaitcancel(client, bytes, size, timeout, NULL);
}
