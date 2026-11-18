/* Copyright (c) 2026 bhgv
 * EN: Queued single-tag renewal barriers, cached ACKs and no global traffic drain.
 * RU: Очередь сбросов одного тега, сохранённые ACK и отсутствие общего ожидания трафика.
 */
#include "internal.h"


/* EN/RU: See internal.h; reserve reply before changing RX state, queue before releasing broker lock.
 * См. internal.h; ответ уже выделен до изменения RX, ставится в очередь до освобождения mutex.
 */
int orderedack(StyxOrderedClient *client)
{
    StyxOrdered *peer = client->peer; /* Pinned native owner. */
    unsigned long long operation = orderedvalue(client->rx_request.operation); /* Exact pending correlation. */
    if (client->rx_pending && styxorderready(peer->engine, client->tag, STYX_ORDER_RX, operation))
    {
        if (!client->rx_reply || !styxorderconfirm(peer->engine, client->tag, STYX_ORDER_RX, operation, 1))
        {
            return 0;
        }
        orderedqueue(peer, client->rx_reply);
        client->rx_reply = NULL;
        client->rx_pending = 0;
    }
    return 1;
}


/* EN/RU: See internal.h; place reset after every previously accepted packet in the same native outbox.
 * См. internal.h; поставить сброс после всех ранее принятых пакетов в одной native-очереди.
 */
int orderedrequest(StyxOrderedClient *client)
{
    StyxOrdered *peer = client->peer; /* Caller holds peer lock. */
    if (!peer->active[STYX_ORDER_TX] || !client->sent || client->tx_pending || client->renewal == ~0ULL)
    {
        return 0;
    }
    FVM_SOrderControl request = {0}; /* Exact selected-counter request. */
    request.command = FVM_SORDER_RENEW;
    request.operation = orderednumber(client->renewal + 1);
    request.lane = peer->lanes[STYX_ORDER_TX];
    request.tag = client->tag;
    request.boundary = orderednumber(client->last);
    OrderedMessage *message = orderedcontrol(&request); /* Private until engine reserves counter. */
    if (!message)
    {
        return 0;
    }
    if (!styxorderprepare(peer->engine, client->tag, STYX_ORDER_TX, client->renewal + 1, client->last))
    {
        free(message);
        return 0;
    }
    client->renewal++;
    client->tx_pending = 1;
    client->tx_request = request;
    orderedqueue(peer, message);
    return 1;
}


/* EN/RU: Begin one RX request or replay its completed ACK; exact pending duplicates never bypass consumption.
 * Начать один RX-запрос либо повторить завершённое ACK; pending-повторы не обходят выдачу старых записей.
 */
static int receive(StyxOrderedClient *client, const FVM_SOrderControl *request)
{
    StyxOrdered *peer = client->peer; /* Caller holds peer lock. */
    unsigned long long operation = orderedvalue(request->operation); /* Peer tag's monotonic reset ID. */
    unsigned long long previous = orderedvalue(client->rx_request.operation); /* Latest admitted peer ID. */
    if (!peer->active[STYX_ORDER_RX] || request->lane != peer->lanes[STYX_ORDER_RX])
    {
        return 0;
    }
    if (operation < previous)
    {
        return 1;
    }
    if (operation == previous)
    {
        if (!orderedequal(request, &client->rx_request, 1))
        {
            return 0;
        }
        if (client->rx_pending)
        {
            return orderedack(client);
        }
    }
    else if (client->rx_pending)
    {
        return 0;
    }
    FVM_SOrderControl reply = *request; /* Exact selected-tag ACK. */
    reply.command = FVM_SORDER_ACK;
    OrderedMessage *message = orderedcontrol(&reply); /* Allocated before changing engine. */
    if (!message)
    {
        return -1;
    }
    if (operation == previous)
    {
        orderedqueue(peer, message);
        return 1;
    }
    if (!styxorderprepare(peer->engine, client->tag, STYX_ORDER_RX, operation, orderedvalue(request->boundary)))
    {
        free(message);
        return 0;
    }
    client->rx_request = *request;
    client->rx_pending = 1;
    client->rx_reply = message;
    return orderedack(client);
}


/* EN/RU: See internal.h; no confirmation can release another tag/direction or a mismatched boundary.
 * См. internal.h; подтверждение не освобождает другой тег/направление либо несовпавшую границу.
 */
int orderedrenewal(StyxOrdered *peer, const FVM_SOrderControl *message)
{
    StyxOrderedClient *client = orderedfind(peer, message->tag); /* Predeclared grant only. */
    if (!client)
    {
        return 0;
    }
    if (message->command == FVM_SORDER_RENEW)
    {
        return receive(client, message);
    }
    if (orderedvalue(message->operation) != orderedvalue(client->tx_request.operation))
    {
        return 1;
    }
    if (!peer->active[STYX_ORDER_TX] || !orderedequal(message, &client->tx_request, 0))
    {
        return 0;
    }
    int accepted = message->command == FVM_SORDER_ACK; /* Exact ACK versus explicit NACK. */
    if (!styxorderconfirm(peer->engine, client->tag, STYX_ORDER_TX, orderedvalue(message->operation), accepted))
    {
        return 0;
    }
    client->tx_pending = 0;
    pthread_cond_broadcast(&peer->changed);
    return 1;
}


/* EN/RU: See broker.h; native owner can renew early without pausing any unrelated client.
 * См. broker.h; native-владелец может запросить ранний сброс без остановки других клиентов.
 */
int styxorderedrenew(StyxOrdered *peer, unsigned long tag)
{
    if (!peer)
    {
        return 0;
    }
    pthread_mutex_lock(&peer->lock);
    StyxOrderedClient *client = orderedfind(peer, tag); /* Fixed native grant. */
    int valid = !peer->reason && client && orderedrequest(client); /* No queue mutation on refusal. */
    pthread_mutex_unlock(&peer->lock);
    return valid;
}
