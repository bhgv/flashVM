/* Copyright (c) 2026 bhgv
 * EN: Preallocated per-queue hysteresis and coalesced native diagnostic slots under the route mutex.
 * RU: Предвыделенные hysteresis и coalesced native diagnostic slots под mutex route.
 */
#ifndef STYX_ROUTE_PRESSURE_INTERNAL_H
#define STYX_ROUTE_PRESSURE_INTERNAL_H
#include "pressure.h"
#include "mailbox.h"
#define ROUTE_PRESSURE_QUEUES (STYX_ROUTE_TYPES_MAX + 3U) /* General/UI/service plus configured typed recipients. */


/* EN/RU: One queue's independent warning state and optional pending event, owned by route.
 * Независимый warning state очереди и optional pending event принадлежат route.
 */
typedef struct RoutePressureSlot
{
    int high;                     /* Hysteretic state, even after event is consumed. */
    int pending;                  /* One allocated slot has an undelivered coalesced event. */
    unsigned long long first;     /* First pending transition order, stable until delivered. */
    StyxRoutePressureEvent event; /* Scalar event; no provider, guest or callback pointers. */
} RoutePressureSlot;


/* EN/RU: Complete diagnostics state, initialized before workers and protected by route lock thereafter.
 * Полное diagnostics state готово до workers, далее защищено lock route.
 */
typedef struct RoutePressure
{
    StyxRoutePressureLimits limits;                 /* Current local observation policy. */
    unsigned long long generation;                  /* Saturating transition counter, not wire generation. */
    size_t pending;                                 /* Pending slots, at most ROUTE_PRESSURE_QUEUES. */
    RoutePressureSlot slots[ROUTE_PRESSURE_QUEUES]; /* Preallocated notification storage, no recursive FIFO. */
} RoutePressure;
struct StyxBidirRoute; /* Borrowed owner in private helpers. */


/* EN/RU: Select a mailbox from pinned owner; NULL for invalid kind/index, no I/O or mutation.
 * Выбрать mailbox pinned owner; NULL при invalid kind/index, без I/O/mutation.
 */
RouteMailbox *routemailboxselect(struct StyxBidirRoute *route, unsigned kind, unsigned index);


/* EN/RU: Fill coherent metrics under caller-held route mutex, no allocation, output borrowed.
 * Заполнить coherent metrics под mutex route, без allocation, output borrowed.
 */
void routemailboxmetrics(RouteMailbox *mailbox, StyxRouteQueueMetrics *output);


/* EN/RU: Initialize default warning thresholds before any callback/worker publication.
 * Задать default warning thresholds до публикации callbacks/workers.
 */
void routepressureinit(struct StyxBidirRoute *route);


/* EN/RU: Under owner lock, reevaluate one queue after committed enqueue/pop; configure marks explicit policy change.
 * No callback, allocation or I/O; may wake diagnostics conditions. / Под lock переоценить очередь после enqueue/pop.
 */
void routepressureupdate(struct StyxBidirRoute *route, RouteMailbox *mailbox, int configure);
#endif
