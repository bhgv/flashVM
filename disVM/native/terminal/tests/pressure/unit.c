/* Copyright (c) 2026 bhgv
 * EN: Deterministic diagnostic-only clock failures, saturation, hysteresis, coalescing and stopped-inbox drain.
 * RU: Детерминированные diagnostic clock failures, saturation, hysteresis, coalescing и stopped-inbox drain.
 */
#include "../../route/internal.h"
#include <stdio.h>
static unsigned long long now = 1000; /* Main-only synthetic monotonic milliseconds, no transport or timer tests. */
static int clock_failed;              /* Refuse next diagnostic clock capture/query only. */


/* EN/RU: Real OS clock fallback for other clock domains, output borrowed through return.
 * Реальный OS clock для других domains, output borrowed до return.
 */
extern int __real_clock_gettime(clockid_t, struct timespec *);


/* EN/RU: Deterministically control diagnostic time without sleeping or modifying queue admission semantics.
 * Детерминированное diagnostic time без sleep либо изменения queue admission semantics.
 */
int __wrap_clock_gettime(clockid_t clock, struct timespec *value)
{
    if (clock != CLOCK_MONOTONIC)
    {
        return __real_clock_gettime(clock, value);
    }
    if (clock_failed)
    {
        clock_failed = 0;
        return -1;
    }
    value->tv_sec = (time_t)(now / 1000ULL);
    value->tv_nsec = (long)(now % 1000ULL) * 1000000L;
    return 0;
}


/* EN/RU: Explicit fixture contract, diagnostic borrowed through return/exit.
 * Явный fixture contract, diagnostic borrowed до return/exit.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "native pressure diagnostics: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Initialize isolated native mailbox owner; no fake transport success or managed VM involved.
 * Инициализировать isolated native mailbox owner, без fake transport success либо managed VM.
 */
static void initialize(StyxBidirRoute *route)
{
    memset(route, 0, sizeof(*route));
    check(!pthread_mutex_init(&route->lock, NULL) && !pthread_cond_init(&route->changed, NULL), "native locks");
    route->general.kind = STYX_ROUTE_GENERAL;
    route->ui.kind = STYX_ROUTE_UI;
    route->service.kind = STYX_ROUTE_SERVICE;
    routepressureinit(route);
}


/* EN/RU: Consume isolated native nodes/locks after every diagnostic caller retired.
 * Поглотить isolated native nodes/locks после всех diagnostic callers.
 */
static void dispose(StyxBidirRoute *route)
{
    routemailboxclear(&route->general);
    routemailboxclear(&route->ui);
    routemailboxclear(&route->service);
    pthread_cond_destroy(&route->changed);
    pthread_mutex_destroy(&route->lock);
}


/* EN/RU: Pop one original copied payload using public native general receiver, preserves FIFO.
 * Прочитать одну original copied payload public native general receiver, сохраняя FIFO.
 */
static void pop(StyxBidirRoute *route)
{
    char bytes[8]; /* One native copied payload. */
    unsigned long tag; /* Original tag metadata. */
    size_t size; /* Exact body extent. */
    check(styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD, "one native record");
}


/* EN/RU: Read one coherent general diagnostic snapshot; never consumes its records.
 * Прочитать coherent general diagnostic snapshot, не потребляя records.
 */
static StyxRouteQueueMetrics metrics(StyxBidirRoute *route)
{
    StyxRouteQueueMetrics output; /* Exact public snapshot copied under original mutex. */
    check(styxbidirroutequeuemetrics(route, STYX_ROUTE_GENERAL, 0, &output), "metrics snapshot");
    return output;
}


/* EN/RU: Check age movement with FIFO head, SMALL, empty records and diagnostic-only clock failure.
 * Проверить age при смене FIFO head, SMALL, empty records и diagnostic-only clock failure.
 */
static void ages(void)
{
    StyxBidirRoute route; /* Isolated native owner through disposal. */
    initialize(&route);
    now = 1000;
    check(routeenqueue(&route, &route.general, 1, "abc", 3), "first timestamped record");
    now = 2000;
    check(routeenqueue(&route, &route.general, 2, NULL, 0), "empty record timestamp");
    now = 5000;
    StyxRouteQueueMetrics info = metrics(&route); /* Current age of first record. */
    check(info.age_valid && info.oldest_milliseconds == 4000 && info.received == 2 && info.received_bytes == 3,
        "oldest monotonic age and exact admitted totals");
    unsigned long tag = 99; /* SMALL output sentinel. */
    size_t size = 99; /* Required output size. */
    check(styxbidirroutenext(&route, &tag, NULL, 0, &size) == STYX_CONTROL_SMALL && tag == 99 && size == 3 &&
        !metrics(&route).delivered, "SMALL does not count delivery");
    clock_failed = 1;
    check(!metrics(&route).age_valid && metrics(&route).age_valid && !route.reason, "failed age clock does not fail channel");
    now = 999;
    check(!metrics(&route).age_valid, "regressed diagnostic clock cannot create negative age");
    now = 5000;
    pop(&route);
    info = metrics(&route);
    check(info.age_valid && info.oldest_milliseconds == 3000 && info.delivered == 1 && info.delivered_bytes == 3,
        "age advances to next FIFO head");
    pop(&route);
    check(!metrics(&route).age_valid && metrics(&route).delivered == 2, "empty message counts once; drained age unavailable");
    clock_failed = 1;
    check(routeenqueue(&route, &route.general, 3, "x", 1) && !metrics(&route).age_valid && !route.reason,
        "failed admission timestamp never refuses message");
    pop(&route);
    dispose(&route);
}


/* EN/RU: Check coalesced HIGH/LOW, recovery hysteresis, explicit disable, idempotent policy and final drain.
 * Проверить coalesced HIGH/LOW, recovery hysteresis, explicit disable, idempotent policy и final drain.
 */
static void transitions(void)
{
    StyxBidirRoute route; /* Native-only diagnostics owner. */
    initialize(&route);
    StyxRoutePressureLimits limits = {2, 1, 0, 0}; /* Count-only hysteresis. */
    StyxRoutePressureLimits disabled = {0, 0, 0, 0}; /* No automatic admission effects. */
    check(styxbidirroutepressurelimits(&route, &limits), "configure thresholds");
    check(routeenqueue(&route, &route.general, 1, NULL, 0) && routeenqueue(&route, &route.general, 2, NULL, 0), "reach high");
    pop(&route);
    StyxRoutePressureEvent event; /* One coalesced event from a fixed slot. */
    check(styxbidirroutepressurewait(&route, 0) && styxbidirroutepressurenext(&route, &event) == STYX_CONTROL_RECORD &&
        event.changes == (STYX_PRESSURE_HIGH | STYX_PRESSURE_LOW) && !event.high && event.transitions == 2 &&
        event.metrics.queue.records == 1, "unread high/recovery coalesce without losing transition kinds");
    check(routeenqueue(&route, &route.general, 3, NULL, 0), "second high");
    check(styxbidirroutepressurenext(&route, &event) == STYX_CONTROL_RECORD && event.high, "high after recovery");
    check(styxbidirroutepressurelimits(&route, &disabled) &&
        styxbidirroutepressurenext(&route, &event) == STYX_CONTROL_RECORD && event.changes == STYX_PRESSURE_CONFIG && !event.high,
        "disable is configuration, not false recovery");
    check(styxbidirroutepressurelimits(&route, &limits) &&
        styxbidirroutepressurenext(&route, &event) == STYX_CONTROL_RECORD && event.high &&
        event.changes == (STYX_PRESSURE_CONFIG | STYX_PRESSURE_HIGH), "reconfigure reevaluates current backlog");
    check(styxbidirroutepressurelimits(&route, &limits) && !styxbidirroutepressurewait(&route, 0), "same policy idempotent");
    pop(&route);
    styxbidirroutestop(&route);
    check(styxbidirroutepressurenext(&route, &event) == STYX_CONTROL_RECORD && event.changes == STYX_PRESSURE_LOW &&
        styxbidirroutepressurenext(&route, &event) == STYX_CONTROL_END && !styxbidirroutepressurewait(&route, 0),
        "pending notices drain after stop, then END");
    check(!styxbidirroutepressurelimits(&route, &disabled), "retired route refuses policy replacement");
    pop(&route);
    check(styxbidirroutepressurenext(&route, &event) == STYX_CONTROL_END,
        "post-stop data drain never resurrects diagnostic events after END");
    dispose(&route);
}


/* EN/RU: Artificial near-overflow totals exercise saturation without allocating impossible record counts.
 * Искусственные near-overflow totals проверяют saturation без невозможных allocation counts.
 */
static void saturation(void)
{
    StyxBidirRoute route; /* Private native owner, no worker concurrency in this unit. */
    initialize(&route);
    route.general.received = (size_t)-1;
    route.general.received_bytes = (size_t)-2;
    route.general.delivered = (size_t)-1;
    route.general.delivered_bytes = (size_t)-2;
    check(routeenqueue(&route, &route.general, 1, "abc", 3), "saturated diagnostics never refuse real record");
    pop(&route);
    StyxRouteQueueMetrics info = metrics(&route); /* Saturated counters copied with explicit flag. */
    check(!route.reason && info.saturated && info.received == (size_t)-1 && info.received_bytes == (size_t)-1 &&
        info.delivered == (size_t)-1 && info.delivered_bytes == (size_t)-1 && !info.queue.records,
        "all lifetime counters saturate independently of exact occupancy");
    dispose(&route);
}


/* EN/RU: Exercise default automatic policy and oldest-pending fairness across different recipient queues.
 * Проверить default automatic policy и oldest-pending fairness разных очередей адресатов.
 */
static void defaults(void)
{
    StyxBidirRoute route; /* No transport, exact native queue/observer implementation. */
    initialize(&route);
    check(route.pressure.limits.high_records == STYX_PRESSURE_RECORDS &&
        route.pressure.limits.high_bytes == STYX_PRESSURE_BYTES, "default automatic warning policy");
    for (unsigned i = 0; i < STYX_PRESSURE_RECORDS; i++)
    {
        check(routeenqueue(&route, &route.general, 1, NULL, 0), "default count backlog");
    }
    StyxRoutePressureLimits lower = {1, 0, 0, 0}; /* Reconfiguration adds a later UI warning. */
    check(styxbidirroutepressurelimits(&route, &lower) && routeenqueue(&route, &route.service, 0, "x", 1),
        "independent service warning");
    StyxRoutePressureEvent event; /* Oldest pending queue remains first despite later updates. */
    check(route.pressure.pending == 2 && styxbidirroutepressurenext(&route, &event) == STYX_CONTROL_RECORD &&
        event.kind == STYX_ROUTE_GENERAL && (event.changes & STYX_PRESSURE_HIGH) && (event.changes & STYX_PRESSURE_CONFIG),
        "oldest pending queue remains first after coalesced configuration");
    check(styxbidirroutepressurenext(&route, &event) == STYX_CONTROL_RECORD && event.kind == STYX_ROUTE_SERVICE &&
        !styxbidirroutepressurewait(&route, 0), "one notice per queue and call, no recursive diagnostic queue");
    dispose(&route);
}


/* EN/RU: Either upper threshold warns; both enabled lower thresholds must recover before LOW.
 * Любой upper threshold вызывает HIGH; для LOW нужны оба enabled lower thresholds.
 */
static void mixed(void)
{
    StyxBidirRoute route; /* Private native owner. */
    initialize(&route);
    StyxRoutePressureLimits limits = {4, 1, 4, 1}; /* Both dimensions contribute to hysteresis. */
    check(styxbidirroutepressurelimits(&route, &limits), "mixed warning limits");
    check(routeenqueue(&route, &route.general, 1, "abcde", 5), "byte-only crossing despite one record");
    StyxRoutePressureEvent event; /* Single copied notice. */
    check(styxbidirroutepressurenext(&route, &event) == STYX_CONTROL_RECORD && event.high, "byte dimension alone warns");
    for (unsigned i = 0; i < 3; i++)
    {
        check(routeenqueue(&route, &route.general, 1, NULL, 0), "empty records still contribute to pressure");
    }
    pop(&route);
    check(!styxbidirroutepressurewait(&route, 0), "low bytes alone cannot recover while count remains high");
    pop(&route);
    check(!styxbidirroutepressurewait(&route, 0), "count in hysteresis band retains warning state");
    pop(&route);
    check(styxbidirroutepressurenext(&route, &event) == STYX_CONTROL_RECORD && !event.high &&
        event.changes == STYX_PRESSURE_LOW && event.metrics.queue.records == 1, "both enabled lower dimensions recover");
    dispose(&route);
}


/* EN/RU: Execute all deterministic diagnostic contracts; 0 success, explicit failure exits2.
 * Выполнить deterministic diagnostic contracts; 0 success, явный failure завершает exit2.
 */
int main(void)
{
    ages();
    transitions();
    saturation();
    defaults();
    mixed();
    puts("PASS native age, hysteresis, coalescing and diagnostic saturation");
    return 0;
}
