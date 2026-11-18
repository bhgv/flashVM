/* Copyright (c) 2026 bhgv
 * EN: Independent directional lane offers and exact correlated activation, unknown traffic never grants TX.
 * RU: Независимые предложения направлений и точное подтверждение; чужой трафик не выдаёт TX-права.
 */
#include "internal.h"


/* EN/RU: Begin native TX offer, copy before changing pending state; no waiting or I/O.
 * Начать native TX-предложение, скопировать до изменения pending; без ожидания и I/O.
 */
int styxorderedoffer(StyxOrdered *peer, unsigned long long operation, unsigned long lane)
{
    if (!peer || !operation || !lane || lane > FVM_SRECORD_U32)
    {
        return 0;
    }
    FVM_SOrderControl offer = {0}; /* Complete zeroed proposal, no unused field ambiguity. */
    offer.command = FVM_SORDER_OFFER;
    offer.operation = orderednumber(operation);
    offer.lane = lane;
    offer.limit = peer->limit;
    offer.window = orderednumber(peer->window);
    OrderedMessage *message = orderedcontrol(&offer); /* Private node until admission. */
    if (!message)
    {
        return 0;
    }
    pthread_mutex_lock(&peer->lock);
    int valid = !peer->reason && !peer->active[STYX_ORDER_TX] && !peer->tx_pending &&
        operation > orderedvalue(peer->offer.operation); /* Strict proposal correlation. */
    if (valid)
    {
        peer->offer = offer;
        peer->tx_pending = 1;
        orderedqueue(peer, message);
    }
    pthread_mutex_unlock(&peer->lock);
    if (!valid)
    {
        free(message);
    }
    return valid;
}


/* EN/RU: Reply to one peer offer under lock, cache exact latest result before possible duplicate reception.
 * Ответить peer-предложению под mutex, сохранить точный последний ответ для повторов.
 */
static int respond(StyxOrdered *peer, const FVM_SOrderControl *offer)
{
    unsigned long long operation = orderedvalue(offer->operation); /* Peer direction's operation ID. */
    unsigned long long previous = orderedvalue(peer->remote.operation); /* Greatest seen offer. */
    if (operation < previous)
    {
        return 1;
    }
    FVM_SOrderControl reply = *offer; /* ACK/NACK remains correlated to offered direction. */
    if (operation == previous)
    {
        if (!orderedequal(offer, &peer->remote, 1))
        {
            return 0;
        }
        reply = peer->reply;
    }
    else
    {
        reply.command = peer->active[STYX_ORDER_RX] ? FVM_SORDER_REJECT : FVM_SORDER_ACCEPT;
        if (reply.command == FVM_SORDER_ACCEPT)
        {
            if (reply.limit > peer->limit)
            {
                reply.limit = peer->limit;
            }
            if (orderedvalue(reply.window) > peer->window)
            {
                reply.window = orderednumber(peer->window);
            }
        }
    }
    OrderedMessage *message = orderedcontrol(&reply); /* Allocate reply before activation. */
    if (!message)
    {
        return -1;
    }
    if (operation > previous)
    {
        if (reply.command == FVM_SORDER_ACCEPT && !styxorderreceivewindow(peer->engine, orderedvalue(reply.window)))
        {
            free(message);
            return 0;
        }
        peer->remote = *offer;
        peer->reply = reply;
        if (reply.command == FVM_SORDER_ACCEPT)
        {
            peer->active[STYX_ORDER_RX] = 1;
            peer->lanes[STYX_ORDER_RX] = reply.lane;
            peer->limits[STYX_ORDER_RX] = reply.limit;
        }
    }
    orderedqueue(peer, message);
    return 1;
}


/* EN/RU: See internal.h; exact ACK is the only point at which this TX direction becomes usable.
 * См. internal.h; лишь точное ACK включает это TX-направление.
 */
int orderedprofile(StyxOrdered *peer, const FVM_SOrderControl *message)
{
    if (message->command == FVM_SORDER_OFFER)
    {
        return respond(peer, message);
    }
    if (orderedvalue(message->operation) != orderedvalue(peer->offer.operation))
    {
        return 1;
    }
    if (!peer->tx_pending)
    {
        return orderedequal(message, &peer->answer, 1);
    }
    int accepted = message->command == FVM_SORDER_ACCEPT; /* Explicit NACK preserves inactive TX. */
    int valid = message->lane == peer->offer.lane &&
        (accepted ? message->limit <= peer->offer.limit && orderedvalue(message->window) <= peer->window :
            orderedequal(message, &peer->offer, 0));
    if (valid)
    {
        peer->tx_pending = 0;
        peer->answer = *message;
        if (accepted)
        {
            peer->active[STYX_ORDER_TX] = 1;
            peer->lanes[STYX_ORDER_TX] = message->lane;
            peer->limits[STYX_ORDER_TX] = message->limit;
        }
        pthread_cond_broadcast(&peer->changed);
    }
    return valid;
}
