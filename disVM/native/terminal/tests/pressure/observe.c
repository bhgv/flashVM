/* Copyright (c) 2026 bhgv
 * EN: Automatic native pressure/recovery on actual original Styx unread recipient queues.
 * RU: Автоматические native pressure/recovery на настоящих original Styx unread recipient queues.
 */
#include "route/pressure.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
static StyxRouteQueueMetrics before[4]; /* Main-only snapshots for general/UI/service/typed0. */
static pthread_t observer; /* Independent native diagnostics consumer, joined before owner disposal. */
static int ready; /* Diagnostics-wait result, read by main only after join. */
static unsigned high; /* Bit mask of queues whose HIGH events were actually delivered. */
static unsigned long long generation; /* Last ordered single-transition event. */


/* EN/RU: Explicit native fixture contract, diagnostic borrowed through return/exit.
 * Явный native fixture contract, diagnostic borrowed до return/exit.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "queue pressure: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Block this native diagnostics consumer on its inbox without consuming application messages.
 * Блокировать native diagnostics consumer на своём inbox, не потребляя application messages.
 */
static void *waiting(void *state)
{
    ready = styxbidirroutepressurewait(state, 120000);
    return NULL;
}


/* EN/RU: Install low warning count or byte limits before stress publication, no capacity/admission effects.
 * Задать low warning count либо byte limits до stress publication, без capacity/admission effects.
 */
void pressurestart(StyxBidirRoute *route)
{
    StyxRoutePressureLimits limits = STYX_PRESSURE_CASE < 3 ?
        (StyxRoutePressureLimits){8, 4, 0, 0} : (StyxRoutePressureLimits){0, 0, 2048, 1024}; /* Observation only. */
    check(styxbidirroutepressurelimits(route, &limits), "native warning policy");
    StyxRoutePressureLimits invalid = {8, 8, 0, 0}; /* Invalid lower/upper relation leaves policy intact. */
    check(!styxbidirroutepressurelimits(route, &invalid), "invalid hysteresis refusal");
    check(!pthread_create(&observer, NULL, waiting, route), "native diagnostics waiter");
}


/* EN/RU: Native receiver has accumulated all real data before this call; inspect notices without data consumption.
 * Native receiver накопил все real data до call; notices читаются без data consumption.
 */
void pressurefilled(StyxBidirRoute *route)
{
    check(!pthread_join(observer, NULL) && ready, "blocked diagnostics consumer wakes without recipient drain");
    for (unsigned kind = 0; kind < 4; kind++)
    {
        check(styxbidirroutequeuemetrics(route, kind, 0, &before[kind]) && before[kind].age_valid == (before[kind].queue.records != 0) &&
            !before[kind].delivered && !before[kind].delivered_bytes && !before[kind].saturated,
            "real unread age and lifetime counters");
    }
    check(styxbidirroutepressurewait(route, 0), "automatic pressure arrived before any recipient drain");
    StyxRoutePressureEvent event; /* One local diagnostic event, not a wire record. */
    unsigned notices = 0; /* Hysteresis emits once per queue despite hundreds of arrivals. */
    while (styxbidirroutepressurenext(route, &event) == STYX_CONTROL_RECORD)
    {
        check(event.kind < 4 && !event.index && event.high && event.changes == STYX_PRESSURE_HIGH && event.transitions == 1 &&
            event.generation > generation && event.metrics.queue.records == before[event.kind].queue.records,
            "one high transition and latest coherent snapshot per accumulated queue");
        high |= 1U << event.kind;
        generation = event.generation;
        notices++;
    }
    unsigned kind = STYX_PRESSURE_CASE % 3; /* The intentionally slow recipient from original fixture. */
    kind = kind == 2 ? STYX_ROUTE_TYPED : kind;
    check(notices <= 4 && (high & (1U << kind)), "selected recipient warning without event flood");
    check(!styxbidirroutepressurewait(route, 0) && styxbidirroutestate(route) == STYX_CONTROL_LIVE,
        "warnings neither stop broker nor consume recipient data");
}


/* EN/RU: Original host already drained exact FIFO data; verify automatic recovery and lifetime totals.
 * Original host уже прочитал exact FIFO data; проверить automatic recovery и lifetime totals.
 */
void pressuredrained(StyxBidirRoute *route)
{
    unsigned recovered = 0; /* One recovery for every prior high state. */
    StyxRoutePressureEvent event; /* Single copied local event. */
    while (styxbidirroutepressurenext(route, &event) == STYX_CONTROL_RECORD)
    {
        check(event.kind < 4 && !event.index && !event.high && event.changes == STYX_PRESSURE_LOW && event.transitions == 1 &&
            !event.metrics.queue.records && !event.metrics.queue.bytes && !event.metrics.age_valid,
            "automatic low transition after complete recipient drain");
        recovered |= 1U << event.kind;
    }
    check(recovered == high, "all warned queues recover without guest polling");
    for (unsigned kind = 0; kind < 4; kind++)
    {
        StyxRouteQueueMetrics after; /* Coherent final local snapshot. */
        check(styxbidirroutequeuemetrics(route, kind, 0, &after) && !after.queue.records && !after.queue.bytes &&
            !after.age_valid && after.received == before[kind].received && after.received_bytes == before[kind].received_bytes &&
            after.delivered == after.received && after.delivered_bytes == after.received_bytes,
            "drain updates delivery totals and preserves admission totals");
    }
}
