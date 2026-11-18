/* Copyright (c) 2026 bhgv
 * EN: Native ordered broker ownership, immutable client grants and joined worker retirement.
 * RU: Владение native-брокером, неизменяемые клиентские grants и завершение workers через join.
 */
#include "internal.h"


/* EN/RU: See internal.h; no join or application consumption, ready records remain drainable.
 * См. internal.h; без join и потребления клиентских записей, готовые записи сохраняются.
 */
void orderedend(StyxOrdered *peer, int reason)
{
    pthread_mutex_lock(&peer->lock);
    if (!peer->reason)
    {
        peer->reason = reason ? reason : STYX_CONTROL_STOPPED;
    }
    pthread_cond_broadcast(&peer->changed);
    pthread_mutex_unlock(&peer->lock);
    styxorderclose(peer->engine);
    styxbidircontrolstop(peer->control);
    dvm_cancel_request(peer->retired);
}


/* EN/RU: See broker.h; native owner can stop while external users still hold pinned client handles.
 * См. broker.h; native-владелец может закрыть брокер, пока внешние users ещё удерживают handles.
 */
void styxorderedstop(StyxOrdered *peer)
{
    if (peer)
    {
        orderedend(peer, STYX_CONTROL_STOPPED);
    }
}


/* EN/RU: Dispose an owned FIFO after every worker/caller retires; no callbacks.
 * Освободить собственный FIFO после завершения workers/users; без callbacks.
 */
static void messages(OrderedMessage *message)
{
    while (message)
    {
        OrderedMessage *next = message->next; /* Transfer continuation before free. */
        free(message);
        message = next;
    }
}


/* EN/RU: See broker.h; constructed synchronization survives until both workers and owned control finish.
 * См. broker.h; синхронизация существует до завершения обоих workers и собственного control.
 */
void styxorderedfree(StyxOrdered *peer)
{
    if (!peer)
    {
        return;
    }
    styxorderedstop(peer);
    for (unsigned i = 0; i < peer->started; i++)
    {
        pthread_join(peer->workers[i], NULL);
    }
    orderedretirementdetach(peer);
    styxbidircontrolfree(peer->control);
    styxorderfree(peer->engine);
    dvm_cancel_release(peer->retired);
    while (peer->clients)
    {
        StyxOrderedClient *client = peer->clients; /* Transfer client ownership before unlink. */
        peer->clients = client->next;
        free(client->rx_reply);
        free(client);
    }
    messages(peer->head);
    messages(peer->services);
    pthread_cond_destroy(&peer->changed);
    pthread_mutex_destroy(&peer->lock);
    free(peer);
}


/* EN/RU: Initialize private monotonic synchronization, 1/0; failed setup disposes completed primitives only.
 * Инициализировать закрытую monotonic-синхронизацию: 1/0; при отказе освободить только готовые primitives.
 */
static int synchronization(StyxOrdered *peer)
{
    if (pthread_mutex_init(&peer->lock, NULL))
    {
        return 0;
    }
    pthread_condattr_t attributes; /* Temporary native condition configuration. */
    int initialized = !pthread_condattr_init(&attributes); /* Disposal guard. */
    int valid = initialized && !pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) &&
        !pthread_cond_init(&peer->changed, &attributes);
    if (initialized)
    {
        pthread_condattr_destroy(&attributes);
    }
    if (!valid)
    {
        pthread_mutex_destroy(&peer->lock);
    }
    return valid;
}


/* EN/RU: See broker.h; grants and buffers exist before any peer input can reach a client queue.
 * См. broker.h; grants и буферы создаются до возможности доставки peer-данных клиенту.
 */
StyxOrdered *styxbidirordered(StyxBidirControl *control, unsigned limit, unsigned long long window,
    const unsigned long *tags, size_t count)
{
    if (!control || control->limit < FVM_SORDER_CTL_MAX || !limit || limit > control->limit - 64 ||
        !window || window >= (1ULL << 63) || (!tags && count))
    {
        styxbidircontrolfree(control);
        return NULL;
    }
    StyxOrdered *peer = calloc(1, sizeof(*peer)); /* Private native owner. */
    if (!peer || !synchronization(peer))
    {
        free(peer);
        styxbidircontrolfree(control);
        return NULL;
    }
    peer->control = control;
    peer->limit = limit;
    peer->window = window;
    peer->engine = styxordercreate(limit, window);
    peer->retired = dvm_cancel_create();
    int valid = peer->engine && peer->retired && orderedretirementattach(peer); /* All later rollback can use the initialized owner. */
    for (size_t i = 0; valid && i < count; i++)
    {
        valid = tags[i] <= FVM_SRECORD_U32 && !orderedfind(peer, tags[i]);
        if (valid)
        {
            StyxOrderedClient *client = calloc(1, sizeof(*client)); /* Distinct fixed-tag application capability. */
            valid = client != NULL;
            if (valid)
            {
                client->peer = peer;
                client->tag = tags[i];
                client->next = peer->clients;
                peer->clients = client;
            }
        }
    }
    if (valid && !pthread_create(peer->workers, NULL, orderedreceive, peer))
    {
        peer->started = 1;
        if (!pthread_create(peer->workers + 1, NULL, orderedtransmit, peer))
        {
            peer->started = 2;
            return peer;
        }
    }
    styxorderedfree(peer);
    return NULL;
}


/* EN/RU: See broker.h; no new grant can be inferred from peer data or profile negotiation.
 * См. broker.h; peer-данные и согласование профиля не создают новые grants.
 */
StyxOrderedClient *styxorderedclient(StyxOrdered *peer, unsigned long tag)
{
    return peer ? orderedfind(peer, tag) : NULL;
}


/* EN/RU: See broker.h; copy first terminal reason without touching transport or queues.
 * См. broker.h; скопировать первую причину завершения без обращения к транспорту или очередям.
 */
int styxorderedstate(StyxOrdered *peer)
{
    if (!peer)
    {
        return STYX_CONTROL_READ_FAILED;
    }
    pthread_mutex_lock(&peer->lock);
    int reason = peer->reason; /* Coherent native snapshot. */
    pthread_mutex_unlock(&peer->lock);
    return reason;
}
