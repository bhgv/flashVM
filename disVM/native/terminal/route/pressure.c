/* Copyright (c) 2026 bhgv
 * EN: Automatic hysteretic backlog notices in preallocated slots; no broker waits or wire side effects.
 * RU: Автоматические hysteretic backlog notices в предвыделенных slots, без broker waits и wire effects.
 */
#include "internal.h"


/* EN/RU: Map immutable queue identity to one preallocated diagnostic slot, caller holds route mutex.
 * Immutable identity очереди в один preallocated diagnostic slot, caller держит mutex route.
 */
static unsigned position(RouteMailbox *mailbox)
{
    return mailbox->kind == STYX_ROUTE_TYPED ? 3U + mailbox->index : mailbox->kind;
}


/* EN/RU: See pressure_internal.h; default warnings are observation only and allocate no storage.
 * См. pressure_internal.h; default warnings лишь observation и не выделяют storage.
 */
void routepressureinit(StyxBidirRoute *route)
{
    route->pressure.limits.high_records = STYX_PRESSURE_RECORDS;
    route->pressure.limits.low_records = STYX_PRESSURE_RECORDS / 2U;
    route->pressure.limits.high_bytes = STYX_PRESSURE_BYTES;
    route->pressure.limits.low_bytes = STYX_PRESSURE_BYTES / 2U;
}


/* EN/RU: See pressure_internal.h; no callback, dynamic diagnostic queue, allocation or channel operation.
 * См. pressure_internal.h; без callback, dynamic diagnostic queue, allocation либо channel operation.
 */
void routepressureupdate(StyxBidirRoute *route, RouteMailbox *mailbox, int configure)
{
    if (route->reason)
    {
        return; /* Retired observation cannot publish new events after diagnostics END. */
    }
    RoutePressure *pressure = &route->pressure; /* Owner-lock protected native-only observer state. */
    StyxRoutePressureLimits *limits = &pressure->limits; /* Immutable during this update. */
    RoutePressureSlot *slot = &pressure->slots[position(mailbox)]; /* One slot per recipient. */
    int enabled = limits->high_records || limits->high_bytes; /* Neither dimension is an admission limit. */
    int high = slot->high; /* Hysteresis retains prior state between thresholds. */
    if (!enabled)
    {
        high = 0;
    }
    else if (!high)
    {
        high = (limits->high_records && mailbox->count >= limits->high_records) ||
            (limits->high_bytes && mailbox->bytes >= limits->high_bytes);
    }
    else if ((!limits->high_records || mailbox->count <= limits->low_records) &&
        (!limits->high_bytes || mailbox->bytes <= limits->low_bytes))
    {
        high = 0;
    }
    if (high == slot->high && !configure)
    {
        return;
    }
    if (configure && high == slot->high && !slot->pending && !high)
    {
        return; /* Policy reevaluation alone does not create notices for unchanged quiet queues. */
    }
    unsigned changes = configure ? STYX_PRESSURE_CONFIG : 0; /* Explicit reconfiguration is distinguishable. */
    if (high != slot->high && enabled)
    {
        changes |= high ? STYX_PRESSURE_HIGH : STYX_PRESSURE_LOW;
    }
    slot->high = high;
    if (pressure->generation != ~0ULL)
    {
        pressure->generation++;
    }
    if (!slot->pending)
    {
        memset(&slot->event, 0, sizeof(slot->event));
        slot->pending = 1;
        slot->first = pressure->generation;
        pressure->pending++;
    }
    slot->event.kind = mailbox->kind;
    slot->event.index = mailbox->index;
    slot->event.changes |= changes;
    slot->event.high = high;
    slot->event.generation = pressure->generation;
    if (slot->event.transitions != (size_t)-1)
    {
        slot->event.transitions++;
    }
    pthread_cond_broadcast(&route->changed);
}


/* EN/RU: See pressure.h; reject malformed hysteresis before changing limits or pending notifications.
 * См. pressure.h; отвергнуть malformed hysteresis до изменения limits либо pending notifications.
 */
int styxbidirroutepressurelimits(StyxBidirRoute *route, const StyxRoutePressureLimits *limits)
{
    if (!route || !limits || (limits->high_records ? limits->low_records >= limits->high_records : limits->low_records != 0) ||
        (limits->high_bytes ? limits->low_bytes >= limits->high_bytes : limits->low_bytes != 0))
    {
        return 0;
    }
    pthread_mutex_lock(&route->lock);
    if (route->reason)
    {
        pthread_mutex_unlock(&route->lock);
        return 0;
    }
    if (route->pressure.limits.high_records == limits->high_records &&
        route->pressure.limits.low_records == limits->low_records &&
        route->pressure.limits.high_bytes == limits->high_bytes && route->pressure.limits.low_bytes == limits->low_bytes)
    {
        pthread_mutex_unlock(&route->lock);
        return 1;
    }
    route->pressure.limits = *limits;
    routepressureupdate(route, &route->general, 1);
    routepressureupdate(route, &route->ui, 1);
    routepressureupdate(route, &route->service, 1);
    for (unsigned i = 0; i < route->typed_count; i++)
    {
        routepressureupdate(route, &route->typed[i].mailbox, 1);
    }
    pthread_mutex_unlock(&route->lock);
    return 1;
}


/* EN/RU: See pressure.h; choose oldest pending queue, refresh snapshot at delivery without draining data.
 * См. pressure.h; выбрать oldest pending queue, обновить snapshot при выдаче, не потребляя data.
 */
int styxbidirroutepressurenext(StyxBidirRoute *route, StyxRoutePressureEvent *output)
{
    if (!route || !output)
    {
        return STYX_CONTROL_END;
    }
    pthread_mutex_lock(&route->lock);
    RoutePressureSlot *selected = NULL; /* Selected only while route mutex pins all diagnostic slots. */
    for (unsigned i = 0; i < ROUTE_PRESSURE_QUEUES; i++)
    {
        RoutePressureSlot *slot = &route->pressure.slots[i]; /* Preallocated immutable location. */
        if (slot->pending && (!selected || slot->first < selected->first))
        {
            selected = slot;
        }
    }
    int status = route->reason ? STYX_CONTROL_END : STYX_CONTROL_EMPTY; /* Empty diagnostic inbox outcome. */
    if (selected)
    {
        *output = selected->event;
        routemailboxmetrics(routemailboxselect(route, output->kind, output->index), &output->metrics);
        selected->pending = 0;
        route->pressure.pending--;
        status = STYX_CONTROL_RECORD;
    }
    pthread_mutex_unlock(&route->lock);
    return status;
}


/* EN/RU: See pressure.h; diagnostics wait owns neither transport nor application recipient queues.
 * См. pressure.h; diagnostics wait не владеет transport либо application recipient queues.
 */
int styxbidirroutepressurewait(StyxBidirRoute *route, unsigned timeout)
{
    return route ? routewaitcount(route, &route->pressure.pending, timeout) : 0;
}
