/* Copyright (c) 2026 bhgv
 * EN: Fixed-tag client sends, one-record copies and independent native waits; no root authority escapes.
 * RU: Отправка фиксированного тега, поштучное чтение и native-ожидание; полномочия root клиенту не выдаются.
 */
#include "internal.h"


/* EN/RU: Charge encoded inner tag to negotiated remote body ceiling before assigning an ordinal.
 * Учесть кодирование внутреннего тега в согласованном ограничении до назначения номера.
 */
static int fits(StyxOrderedClient *client, size_t size)
{
    FVM_SInteger tag = {0, client->tag, 0}; /* Original canonical tag. */
    unsigned char bytes[FVM_SRECORD_INTEGER_MAX]; /* Private encoded scalar. */
    size_t used; /* Body bytes occupied by tag. */
    unsigned limit = client->peer->limits[STYX_ORDER_TX]; /* Negotiated remote ceiling. */
    return fvm_sinteger_write(&tag, bytes, sizeof(bytes), &used) == FVM_S_OK && used <= limit && size <= limit - used;
}


/* EN/RU: See broker.h; allocate before numbering, then retain exactly those encoded bytes through native TX.
 * См. broker.h; выделить память до нумерации, затем сохранять именно полученные байты до native TX.
 */
int styxorderedclientsend(StyxOrderedClient *client, const void *bytes, size_t size)
{
    if (!client || (!bytes && size))
    {
        return 0;
    }
    StyxOrdered *peer = client->peer; /* Caller pins native owner. */
    pthread_mutex_lock(&peer->lock);
    int valid = !peer->reason && peer->active[STYX_ORDER_TX] && !client->tx_pending && fits(client, size);
    OrderedMessage *message = valid ? orderedcopy(peer->lanes[STYX_ORDER_TX], NULL, peer->limit + 40) : NULL; /* Private spool. */
    valid = valid && message;
    int failure = 0; /* An accepted MAX packet can require terminal handling if reset allocation fails. */
    if (valid)
    {
        size_t used; /* Exact packet extent. */
        valid = styxorderencode(peer->engine, client->tag, bytes, size, message->bytes, message->size, &used) == FVM_S_OK;
        if (valid)
        {
            FVM_SOrderedRecord view; /* Checked ordinal from the exact retained packet. */
            size_t checked; /* Complete packet extent. */
            valid = fvm_sordered_read(message->bytes, used, peer->limit, &view, &checked) == FVM_S_OK && checked == used;
            if (valid)
            {
                message->size = used;
                client->sent = 1;
                client->last = orderedvalue(view.ordinal);
                orderedqueue(peer, message);
                if (client->last == ~0ULL && !orderedrequest(client))
                {
                    failure = STYX_CONTROL_NOMEM;
                }
            }
        }
    }
    pthread_mutex_unlock(&peer->lock);
    if (!valid)
    {
        free(message);
    }
    if (failure)
    {
        orderedend(peer, failure);
    }
    return valid;
}


/* EN/RU: See broker.h; a small buffer never advances the renewal barrier or acknowledges unseen data.
 * См. broker.h; маленький буфер не двигает границу сброса и не подтверждает невыданные данные.
 */
int styxorderedclientnext(StyxOrderedClient *client, unsigned long long *ordinal,
    void *bytes, size_t capacity, size_t *size)
{
    if (!client)
    {
        return STYX_ORDER_END;
    }
    StyxOrdered *peer = client->peer; /* Owner remains pinned through delivery and optional ACK admission. */
    pthread_mutex_lock(&peer->lock);
    int result = styxordernext(peer->engine, client->tag, ordinal, bytes, capacity, size); /* Exactly one copied message. */
    int valid = peer->reason || orderedack(client); /* No post-retirement ACK. */
    pthread_mutex_unlock(&peer->lock);
    if (!valid)
    {
        orderedend(peer, STYX_CONTROL_PROTOCOL);
    }
    return result;
}


/* EN/RU: See broker.h; wait has no root-service access and never blocks broker dispatch.
 * См. broker.h; ожидание не даёт доступа к root-службе и не блокирует диспетчер брокера.
 */
int styxorderedclientwait(StyxOrderedClient *client, unsigned timeout)
{
    return client && styxorderwait(client->peer->engine, client->tag, timeout);
}


/* EN/RU: See broker.h; root service copies are native-only and unrelated to inner client tag0.
 * См. broker.h; копии root-служб доступны только native-владельцу и не связаны с клиентским tag0.
 */
int styxorderedservicenext(StyxOrdered *peer, void *bytes, size_t capacity, size_t *size)
{
    if (!peer || !size || (!bytes && capacity))
    {
        return STYX_CONTROL_END;
    }
    pthread_mutex_lock(&peer->lock);
    OrderedMessage *message = peer->services; /* Selected native root inbox head. */
    int result = peer->reason ? STYX_CONTROL_END : STYX_CONTROL_EMPTY;
    if (message)
    {
        *size = message->size;
        result = STYX_CONTROL_SMALL;
        if (message->size <= capacity)
        {
            if (message->size)
            {
                memcpy(bytes, message->bytes, message->size);
            }
            peer->services = message->next;
            if (!peer->services)
            {
                peer->service_tail = NULL;
            }
            free(message);
            result = STYX_CONTROL_RECORD;
        }
    }
    pthread_mutex_unlock(&peer->lock);
    return result;
}
