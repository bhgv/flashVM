/* Copyright (c) 2026 bhgv
 * EN: Coherent monotonic age and lifetime traffic diagnostics, independent of native wire schemas.
 * RU: Согласованные monotonic age и lifetime traffic diagnostics независимо от wire schemas.
 */
#include "internal.h"


/* EN/RU: See pressure_internal.h; immutable queue association while host pins owner through return.
 * См. pressure_internal.h; immutable queue association при удержанном host owner до return.
 */
RouteMailbox *routemailboxselect(StyxBidirRoute *route, unsigned kind, unsigned index)
{
    if (!route || kind > STYX_ROUTE_TYPED || (kind != STYX_ROUTE_TYPED && index) ||
        (kind == STYX_ROUTE_TYPED && index >= route->typed_count))
    {
        return NULL;
    }
    return kind == STYX_ROUTE_GENERAL ? &route->general : kind == STYX_ROUTE_UI ? &route->ui :
        kind == STYX_ROUTE_SERVICE ? &route->service : &route->typed[index].mailbox;
}


/* EN/RU: See pressure_internal.h; diagnostics failure only clears age_valid, queue data remain unchanged.
 * См. pressure_internal.h; diagnostic failure лишь очищает age_valid, queue data не меняются.
 */
void routemailboxmetrics(RouteMailbox *mailbox, StyxRouteQueueMetrics *output)
{
    memset(output, 0, sizeof(*output));
    output->queue.records = mailbox->count;
    output->queue.bytes = mailbox->bytes;
    output->queue.peak_records = mailbox->peak_count;
    output->queue.peak_bytes = mailbox->peak_bytes;
    output->received = mailbox->received;
    output->received_bytes = mailbox->received_bytes;
    output->delivered = mailbox->delivered;
    output->delivered_bytes = mailbox->delivered_bytes;
    output->saturated = mailbox->saturated != 0;
    output->age_valid = routemailboxage(mailbox, &output->oldest_milliseconds);
}


/* EN/RU: See pressure.h; preserve output on preflight refusal, native clock cannot fail routing.
 * См. pressure.h; сохранить output при preflight refusal, native clock не отказывает routing.
 */
int styxbidirroutequeuemetrics(StyxBidirRoute *route, unsigned kind, unsigned index, StyxRouteQueueMetrics *output)
{
    RouteMailbox *mailbox = routemailboxselect(route, kind, index); /* Borrowed through pinned route. */
    if (!mailbox || !output)
    {
        return 0;
    }
    pthread_mutex_lock(&route->lock);
    routemailboxmetrics(mailbox, output);
    pthread_mutex_unlock(&route->lock);
    return 1;
}
