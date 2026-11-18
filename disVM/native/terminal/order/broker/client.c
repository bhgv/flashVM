/* Copyright (c) 2026 bhgv
 * EN: Fixed-tag copied receives, independent native waits and the native-only root-service inbox.
 * RU: Поштучное чтение своего тега, независимое native-ожидание и root-очередь только для native-владельца.
 */
#include "internal.h"


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
