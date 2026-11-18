/* Copyright (c) 2026 bhgv
 * EN: Fixed-tag packet admission, immutable spool and precise refusal before ordinal allocation.
 * RU: Приём пакета своего тега, неизменяемая очередь и точный отказ до расходования номера.
 */
#include "internal.h"


/* EN/RU: Compute canonical inner-tag cost for a valid native client; no allocation or shared mutation.
 * Вычислить каноническую длину внутреннего тега для native-клиента; без выделения памяти и изменений.
 */
static size_t tagcost(StyxOrderedClient *client)
{
    FVM_SInteger tag = {0, client->tag, 0}; /* Explicit original tag integer. */
    unsigned char bytes[FVM_SRECORD_INTEGER_MAX]; /* Private scalar encoding. */
    size_t used; /* Exact encoded tag extent. */
    return fvm_sinteger_write(&tag, bytes, sizeof(bytes), &used) == FVM_S_OK ? used : FVM_SRECORD_INTEGER_MAX;
}


/* EN/RU: See broker.h; this ceiling never depends on mutable remote negotiation or a different tag.
 * См. broker.h; этот ceiling не зависит от изменяемого согласования или другого тега.
 */
size_t styxorderedclientceiling(StyxOrderedClient *client)
{
    if (!client)
    {
        return 0;
    }
    size_t cost = tagcost(client); /* Immutable fixed-tag framing cost. */
    return client->peer->limit >= cost ? client->peer->limit - cost : 0;
}


/* EN/RU: See internal.h; allocation precedes encode and queue append cannot fail after number assignment.
 * См. internal.h; выделение до encode, добавление в очередь после назначения номера не может отказать.
 */
int orderedsendlocked(StyxOrderedClient *client, const void *bytes, size_t size, int *failure)
{
    StyxOrdered *peer = client->peer; /* Caller holds owner lock through admission. */
    *failure = 0;
    if (peer->reason)
    {
        return STYX_ORDERED_END;
    }
    if (!peer->active[STYX_ORDER_TX] || client->tx_pending)
    {
        return !peer->tx_pending && peer->answer.command == FVM_SORDER_REJECT ? STYX_ORDERED_REFUSED : STYX_ORDERED_WAIT;
    }
    size_t cost = tagcost(client); /* Charge tag before any state mutation. */
    if (cost > peer->limits[STYX_ORDER_TX] || size > peer->limits[STYX_ORDER_TX] - cost)
    {
        return STYX_ORDERED_SIZE;
    }
    OrderedMessage *message = orderedcopy(peer->lanes[STYX_ORDER_TX], NULL, peer->limit + 40); /* Private retained packet. */
    if (!message)
    {
        return STYX_ORDERED_NOMEM;
    }
    size_t used; /* Exact combined packet size. */
    FVM_SResult status = styxorderencode(peer->engine, client->tag, bytes, size, message->bytes, message->size, &used); /* Atomic numbering. */
    if (status != FVM_S_OK)
    {
        free(message);
        if (status == FVM_S_LIMIT)
        {
            return STYX_ORDERED_NOMEM; /* Body/spool bounds already checked; registry allocation failed. */
        }
        if (status == FVM_S_CONFLICT)
        {
            return STYX_ORDERED_REFUSED;
        }
        *failure = STYX_CONTROL_PROTOCOL;
        return STYX_ORDERED_END;
    }
    FVM_SOrderedRecord view; /* Borrowed checked copy of the exact retained packet. */
    size_t checked; /* Complete decoded extent. */
    if (fvm_sordered_read(message->bytes, used, peer->limit, &view, &checked) != FVM_S_OK || checked != used)
    {
        free(message);
        *failure = STYX_CONTROL_PROTOCOL;
        return STYX_ORDERED_END;
    }
    message->size = used;
    client->sent = 1;
    client->last = orderedvalue(view.ordinal);
    orderedqueue(peer, message);
    if (client->last == ~0ULL && !orderedrequest(client))
    {
        *failure = STYX_CONTROL_NOMEM;
    }
    return STYX_ORDERED_SENT;
}


/* EN/RU: See broker.h; retain historical nonblocking1/0 ABI, detailed failures available in sendwait.
 * См. broker.h; сохранить неблокирующий ABI1/0, подробные отказы доступны в sendwait.
 */
int styxorderedclientsend(StyxOrderedClient *client, const void *bytes, size_t size)
{
    if (!client || (!bytes && size))
    {
        return 0;
    }
    StyxOrdered *peer = client->peer; /* Owner remains pinned through any terminal propagation. */
    int failure; /* Native terminal reason, zero for ordinary refusal. */
    pthread_mutex_lock(&peer->lock);
    int result = orderedsendlocked(client, bytes, size, &failure); /* One indivisible admission attempt. */
    pthread_mutex_unlock(&peer->lock);
    if (failure)
    {
        orderedend(peer, failure);
    }
    return result == STYX_ORDERED_SENT;
}
