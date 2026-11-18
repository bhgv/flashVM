/* Copyright (c) 2026 bhgv
 * EN: Dedicated native control dispatch and retained-packet sender, independent from client waits.
 * RU: Выделенные native-диспетчер и отправитель сохранённых пакетов, независимые от клиентских ожиданий.
 */
#include "internal.h"


/* EN/RU: Admit one outer record under peer lock; root services never enter an application tag0 queue.
 * Принять внешнюю запись под mutex peer; root-службы никогда не попадают в клиентскую очередь тега 0.
 */
static int dispatch(StyxOrdered *peer, unsigned long tag, const void *bytes, size_t size)
{
    if (!tag)
    {
        FVM_SOrderControl message; /* Checked named root service. */
        FVM_SResult result = fvm_sorder_control_read(bytes, size, &message); /* Unknown root service remains native. */
        if (result == FVM_S_UNKNOWN)
        {
            OrderedMessage *copy = orderedcopy(0, bytes, size); /* Native-only inbox copy. */
            if (!copy)
            {
                return -1;
            }
            if (peer->service_tail)
            {
                peer->service_tail->next = copy;
            }
            else
            {
                peer->services = copy;
            }
            peer->service_tail = copy;
            pthread_cond_broadcast(&peer->changed);
            return 1;
        }
        if (result != FVM_S_OK)
        {
            return 0;
        }
        return message.command <= FVM_SORDER_REJECT ? orderedprofile(peer, &message) : orderedrenewal(peer, &message);
    }
    FVM_SOrderedRecord record; /* Verified inner packet, original outer checksum already checked. */
    size_t used; /* Exact packet extent, no trailing packets. */
    if (!peer->active[STYX_ORDER_RX] || tag != peer->lanes[STYX_ORDER_RX] ||
        fvm_sordered_read(bytes, size, peer->limits[STYX_ORDER_RX], &record, &used) != FVM_S_OK || used != size ||
        !orderedfind(peer, record.record.tag))
    {
        return 0;
    }
    int result = styxorderfeed(peer->engine, bytes, size); /* Engine owns immutable payload copy. */
    pthread_cond_broadcast(&peer->changed); /* Native cancellable inbox waits share the dispatch publication lock. */
    return result == STYX_ORDER_NOMEM ? -1 : result == STYX_ORDER_ACCEPTED || result == STYX_ORDER_DUPLICATE;
}


/* EN/RU: See internal.h; underlying native watch wakes immediately on connection retirement.
 * См. internal.h; native watch транспорта немедленно будит при завершении соединения.
 */
void *orderedreceive(void *argument)
{
    StyxOrdered *peer = argument; /* Borrowed until owner joins this worker. */
    unsigned char *bytes = malloc(peer->control->limit); /* Sole dispatcher scratch, no guest buffer. */
    if (!bytes)
    {
        orderedend(peer, STYX_CONTROL_NOMEM);
        return NULL;
    }
    for (;;)
    {
        if (!styxbidircontrolwait(peer->control, 120000))
        {
            int reason = styxbidircontrolstate(peer->control); /* Timeout alone does not retire live channel. */
            if (reason)
            {
                orderedend(peer, reason);
                break;
            }
            continue;
        }
        unsigned long tag; /* Actual original outer tag. */
        size_t size; /* Exact copied body. */
        int result = styxbidircontrolnext(peer->control, &tag, bytes, peer->control->limit, &size); /* One record. */
        pthread_mutex_lock(&peer->lock);
        int valid = result == STYX_CONTROL_RECORD && !peer->reason ? dispatch(peer, tag, bytes, size) : 0;
        pthread_mutex_unlock(&peer->lock);
        if (valid <= 0)
        {
            orderedend(peer, valid < 0 ? STYX_CONTROL_NOMEM : STYX_CONTROL_PROTOCOL);
            break;
        }
    }
    free(bytes);
    return NULL;
}


/* EN/RU: See internal.h; one FIFO freezes old packets before renewal requests, head lives until admission finishes.
 * См. internal.h; единый FIFO ставит старые пакеты перед запросом сброса, head живёт до завершения admission.
 */
void *orderedtransmit(void *argument)
{
    StyxOrdered *peer = argument; /* Borrowed through join. */
    for (;;)
    {
        pthread_mutex_lock(&peer->lock);
        while (!peer->reason && !peer->head)
        {
            pthread_cond_wait(&peer->changed, &peer->lock);
        }
        OrderedMessage *message = peer->head; /* Retained immutable head during transport wait. */
        int ended = peer->reason; /* Sticky stop. */
        pthread_mutex_unlock(&peer->lock);
        if (ended)
        {
            break;
        }
        if (!styxbidircontrolsendwait(peer->control, message->tag, message->bytes, message->size, 120000, NULL))
        {
            int reason = styxbidircontrolstate(peer->control); /* Preserve actual transport failure if available. */
            orderedend(peer, reason ? reason : STYX_CONTROL_WRITE_FAILED);
            break;
        }
        pthread_mutex_lock(&peer->lock);
        peer->head = message->next;
        if (!peer->head)
        {
            peer->tail = NULL;
        }
        pthread_mutex_unlock(&peer->lock);
        free(message);
    }
    return NULL;
}
