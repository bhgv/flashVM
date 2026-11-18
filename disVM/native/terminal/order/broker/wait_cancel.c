/* Copyright (c) 2026 bhgv
 * EN: Native cancellation wakes only one wait predicate, never stops a shared broker or drives guest code.
 * RU: Native-отмена будит только предикат ожидания, не закрывает общий broker и не исполняет guest-код.
 */
#include "internal.h"
#include "wait_cancel.h"


/* EN/RU: Sticky callback borrows subscription state, locks fence before peer; no I/O or VM calls.
 * Sticky callback заимствует state подписки, берёт fence до peer; без I/O и VM-вызовов.
 */
static void cancel(void *state)
{
    OrderedWaitCancel *wait = state; /* Source retains storage until callback release. */
    pthread_mutex_lock(&wait->fence);
    if (wait->peer)
    {
        pthread_mutex_lock(&wait->peer->lock);
        wait->cancelled = 1;
        pthread_cond_broadcast(&wait->peer->changed);
        pthread_mutex_unlock(&wait->peer->lock);
    }
    pthread_mutex_unlock(&wait->fence);
}


/* EN/RU: Dispose private fence after every admitted callback; never touch a detached peer.
 * Уничтожить private fence после всех допущенных callbacks, не обращаться к отделённому peer.
 */
static void release(void *state)
{
    OrderedWaitCancel *wait = state; /* Final subscription owner. */
    pthread_mutex_destroy(&wait->fence);
    free(wait);
}


/* EN/RU: See wait_cancel.h; callback may replay before subscribe returns, all state is already initialized.
 * См. wait_cancel.h; callback может повториться до возврата subscribe, state уже инициализирован.
 */
int orderedwaitattach(StyxOrdered *peer, DVM_Cancel *source, OrderedWaitCancel **output)
{
    *output = NULL;
    if (!source)
    {
        return 1;
    }
    OrderedWaitCancel *wait = calloc(1, sizeof(*wait)); /* Private until subscription adopts it. */
    if (!wait || pthread_mutex_init(&wait->fence, NULL))
    {
        free(wait);
        return 0;
    }
    wait->peer = peer;
    wait->tap = dvm_cancel_subscribe(source, cancel, wait, release);
    if (!wait->tap)
    {
        release(wait);
        return 0;
    }
    *output = wait;
    return 1;
}


/* EN/RU: See wait_cancel.h; caller must not hold peer lock while fencing or unsubscribing.
 * См. wait_cancel.h; caller не держит peer lock при отделении либо unsubscribe.
 */
void orderedwaitdetach(OrderedWaitCancel *wait)
{
    if (wait)
    {
        pthread_mutex_lock(&wait->fence);
        wait->peer = NULL;
        pthread_mutex_unlock(&wait->fence);
        dvm_cancel_unsubscribe(wait->tap);
    }
}


/* EN/RU: See broker.h; readiness and cancellation are checked under the same lock as dispatcher publication.
 * См. broker.h; готовность и отмена проверяются под тем же lock, что публикация dispatcher.
 */
int styxorderedclientwaitcancel(StyxOrderedClient *client, unsigned timeout, DVM_Cancel *source)
{
    if (!client || timeout > 120000)
    {
        return 0;
    }
    struct timespec deadline; /* One native monotonic budget across unrelated wakes. */
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
    StyxOrdered *peer = client->peer; /* Caller pins peer through detach. */
    OrderedWaitCancel *wait; /* Optional source-pinning lifetime fence. */
    if (!orderedwaitattach(peer, source, &wait))
    {
        return 0;
    }
    pthread_mutex_lock(&peer->lock);
    int ready = styxorderwait(peer->engine, client->tag, 0); /* Ready data remains drainable after retirement. */
    while (!ready && !peer->reason && !(wait && wait->cancelled) && timeout)
    {
        if (pthread_cond_timedwait(&peer->changed, &peer->lock, &deadline))
        {
            break;
        }
        ready = styxorderwait(peer->engine, client->tag, 0);
    }
    ready = ready && !(wait && wait->cancelled);
    pthread_mutex_unlock(&peer->lock);
    orderedwaitdetach(wait);
    return ready;
}


/* EN/RU: See broker.h; combining readiness and copy prevents competing opens from stealing a checked head.
 * См. broker.h; объединение readiness и copy не позволяет другому open забрать уже проверенный head.
 */
int styxorderedclientnextwaitcancel(StyxOrderedClient *client, unsigned long long *ordinal,
    void *bytes, size_t capacity, size_t *size, unsigned timeout, DVM_Cancel *source)
{
    if (!client || !ordinal || !size || (!bytes && capacity) || timeout > 120000)
    {
        return STYX_ORDER_END;
    }
    struct timespec deadline; /* Original monotonic operation budget. */
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return STYX_ORDER_END;
    }
    deadline.tv_sec += timeout / 1000;
    deadline.tv_nsec += (long)(timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    StyxOrdered *peer = client->peer; /* Caller pins owner through callback detach. */
    OrderedWaitCancel *wait; /* Subscription-owned optional lifetime fence. */
    if (!orderedwaitattach(peer, source, &wait))
    {
        return STYX_ORDER_WAIT_NOMEM;
    }
    pthread_mutex_lock(&peer->lock);
    int result; /* Exact copied-record or terminal predicate. */
    int valid = 1; /* RX renewal ACK remains ordinary coordinator behavior. */
    for (;;)
    {
        if (wait && wait->cancelled)
        {
            result = STYX_ORDER_CANCELLED;
            break;
        }
        result = styxordernext(peer->engine, client->tag, ordinal, bytes, capacity, size);
        if (result == STYX_ORDER_EMPTY && peer->reason)
        {
            result = STYX_ORDER_END; /* Stop notification may precede sequence-engine close; never sleep after its wake. */
        }
        if (result != STYX_ORDER_EMPTY || !timeout)
        {
            break;
        }
        if (pthread_cond_timedwait(&peer->changed, &peer->lock, &deadline))
        {
            break;
        }
    }
    if (result == STYX_ORDER_RECORD)
    {
        valid = peer->reason || orderedack(client);
    }
    pthread_mutex_unlock(&peer->lock);
    orderedwaitdetach(wait);
    if (!valid)
    {
        orderedend(peer, STYX_CONTROL_PROTOCOL);
    }
    return result;
}
