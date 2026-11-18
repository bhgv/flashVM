/* Copyright (c) 2026 bhgv
 * EN: Sticky native broker retirement and lower-control lifetime fence, independent of RX/TX progress.
 * RU: Sticky-завершение брокера и fence нижнего control, независимо от продвижения RX/TX.
 */
#include "internal.h"


/* EN/RU: Subscription-owned fence may outlive peer when an admitted callback is delayed.
 * Fence принадлежит подписке и может пережить peer при задержке допущенного callback.
 */
struct OrderedRetirement
{
    pthread_mutex_t lock; /* Callback lifetime fence, acquired before peer/engine locks. */
    StyxOrdered *peer; /* Borrowed until detach, NULL after native owner retires access. */
};


/* EN/RU: Copy actual lower reason, wake native consumers and notices without recursively stopping control.
 * Скопировать причину нижнего control, разбудить потребителей и подписки без рекурсивного stop.
 */
static void retired(void *state)
{
    struct OrderedRetirement *notice = state; /* Subscription retains this context through callback. */
    pthread_mutex_lock(&notice->lock);
    StyxOrdered *peer = notice->peer; /* Protected by lifetime fence. */
    if (peer)
    {
        int reason = styxbidircontrolstate(peer->control); /* Sticky native snapshot, no I/O. */
        if (reason == STYX_CONTROL_EOF)
        {
            /* EN/RU: Orderly EOF follows already accepted lower records; dispatcher drains them before ending the engine.
             * Корректный EOF идёт после принятых нижних записей; dispatcher выдаёт их engine до завершения.
             */
            pthread_mutex_unlock(&notice->lock);
            return;
        }
        pthread_mutex_lock(&peer->lock);
        if (!peer->reason)
        {
            peer->reason = reason ? reason : STYX_CONTROL_READ_FAILED;
        }
        pthread_cond_broadcast(&peer->changed);
        pthread_mutex_unlock(&peer->lock);
        styxorderclose(peer->engine);
        dvm_cancel_request(peer->retired);
    }
    pthread_mutex_unlock(&notice->lock);
}


/* EN/RU: Dispose subscription-owned fence after every admitted callback; no peer access.
 * Освободить fence после всех допущенных callbacks; без доступа к peer.
 */
static void release(void *state)
{
    struct OrderedRetirement *notice = state; /* Sole final context owner. */
    pthread_mutex_destroy(&notice->lock);
    free(notice);
}


/* EN/RU: See internal.h; peer/engine/notification source exist before sticky callback can replay.
 * См. internal.h; peer/engine/source существуют до возможности sticky-повтора callback.
 */
int orderedretirementattach(StyxOrdered *peer)
{
    struct OrderedRetirement *notice = calloc(1, sizeof(*notice)); /* Private until subscription succeeds. */
    if (!notice)
    {
        return 0;
    }
    if (pthread_mutex_init(&notice->lock, NULL))
    {
        free(notice);
        return 0;
    }
    notice->peer = peer;
    DVM_CancelTap *tap = styxbidircontrolwatch(peer->control, retired, notice, release); /* Adopts state only on success. */
    if (!tap)
    {
        release(notice);
        return 0;
    }
    peer->notice = notice;
    peer->transport_retired = tap;
    return 1;
}


/* EN/RU: See internal.h; no late callback may touch peer after this fence is detached.
 * См. internal.h; после отделения fence поздний callback не обращается к peer.
 */
void orderedretirementdetach(StyxOrdered *peer)
{
    if (peer->notice)
    {
        pthread_mutex_lock(&peer->notice->lock);
        peer->notice->peer = NULL;
        pthread_mutex_unlock(&peer->notice->lock);
        peer->notice = NULL;
        dvm_cancel_unsubscribe(peer->transport_retired);
        peer->transport_retired = NULL;
    }
}


/* EN/RU: See broker.h; source-pinning subscription may outlive owner, but its state must fence owner access.
 * См. broker.h; подписка удерживает source после owner, её state обязан защищать доступ к owner.
 */
struct DVM_CancelTap *styxorderedwatch(StyxOrdered *peer, void (*receive)(void *), void *state, void (*dispose)(void *))
{
    return peer ? dvm_cancel_subscribe(peer->retired, receive, state, dispose) : NULL;
}
