/* Copyright (c) 2026 bhgv
 * EN: Native-only system service mailbox and coherent recipient queue diagnostics.
 * RU: Native-only очередь системных служб и согласованная диагностика очередей адресатов.
 */
#include "internal.h"
#include "traffic.h"


/* EN/RU: See traffic.h; choose immutable mailbox location then copy counters under the common mutex.
 * См. traffic.h; выбрать immutable mailbox location и скопировать counters под общим mutex.
 */
int styxbidirroutequeueinfo(StyxBidirRoute *route, unsigned kind, unsigned index, StyxRouteQueueInfo *output)
{
    if (!route || !output || kind > STYX_ROUTE_TYPED || (kind != STYX_ROUTE_TYPED && index) ||
        (kind == STYX_ROUTE_TYPED && index >= route->typed_count))
    {
        return 0;
    }
    RouteMailbox *mailbox = kind == STYX_ROUTE_GENERAL ? &route->general : kind == STYX_ROUTE_UI ? &route->ui :
        kind == STYX_ROUTE_SERVICE ? &route->service : &route->typed[index].mailbox; /* Stable through caller's pin. */
    pthread_mutex_lock(&route->lock);
    output->records = mailbox->count;
    output->bytes = mailbox->bytes;
    output->peak_records = mailbox->peak_count;
    output->peak_bytes = mailbox->peak_bytes;
    pthread_mutex_unlock(&route->lock);
    return 1;
}


/* EN/RU: See traffic.h; opaque service bodies remain separate from general/application receivers.
 * См. traffic.h; opaque service bodies отделены от general/application receivers.
 */
int styxbidirrouteservicenext(StyxBidirRoute *route, void *bytes, size_t capacity, size_t *size)
{
    if (!route || !size || (!bytes && capacity))
    {
        return STYX_CONTROL_END;
    }
    unsigned long tag; /* Private constant-zero metadata, never an application tag assignment. */
    pthread_mutex_lock(&route->lock);
    int status = routemailboxnext(&route->service, route->reason, &tag, bytes, capacity, size);
    pthread_mutex_unlock(&route->lock);
    return status;
}


/* EN/RU: See traffic.h; service consumer waiting never owns or pauses the transport reader.
 * См. traffic.h; ожидающий service consumer не владеет transport reader и не приостанавливает его.
 */
int styxbidirrouteservicewait(StyxBidirRoute *route, unsigned timeout)
{
    return route ? routewaitcount(route, &route->service.count, timeout) : 0;
}
