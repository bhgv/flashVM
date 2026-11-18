/* Copyright (c) 2026 bhgv
 * EN: Automatic native queue-pressure notices and extended counters; no negotiated wire format.
 * RU: Автоматические native-уведомления о backlog и расширенные counters, без нового wire format.
 */
#ifndef STYX_ROUTE_PRESSURE_H
#define STYX_ROUTE_PRESSURE_H
#include "traffic.h"
#ifdef __cplusplus
extern "C" {
#endif
#define STYX_PRESSURE_HIGH 1U    /* Queue crossed an enabled upper threshold. */
#define STYX_PRESSURE_LOW 2U     /* Queue recovered below both enabled lower thresholds. */
#define STYX_PRESSURE_CONFIG 4U  /* Pending state includes an explicit policy reconfiguration. */
#define STYX_PRESSURE_RECORDS 1024U /* Default warning count; not a capacity limit. */
#define STYX_PRESSURE_BYTES (8U * 1024U * 1024U) /* Default warning payload bytes; not a quota. */


/* EN/RU: Hysteresis policy copied locally; zero high disables that dimension, both zero disable observation.
 * Enabled lower must be below upper; disabled lower must be zero. No execution or admission policy.
 * Локальные пороги hysteresis; high0 отключает измерение, оба high0 отключают observation.
 */
typedef struct StyxRoutePressureLimits
{
    size_t high_records; /* Warn when unread records reach this nonzero value. */
    size_t low_records;  /* Count must return to or below this value for recovery. */
    size_t high_bytes;   /* Warn when unread payload bytes reach this nonzero value. */
    size_t low_bytes;    /* Payload bytes must return to or below this value for recovery. */
} StyxRoutePressureLimits;


/* EN/RU: Coherent local metrics; totals saturate with explicit flag and never fail the channel.
 * Согласованные local metrics; totals насыщаются с флагом, не завершая канал.
 */
typedef struct StyxRouteQueueMetrics
{
    StyxRouteQueueInfo queue;               /* Current occupancy and historical peaks. */
    size_t received;                        /* Lifetime admitted records. */
    size_t received_bytes;                  /* Lifetime admitted payload bytes. */
    size_t delivered;                       /* Lifetime successfully copied records. */
    size_t delivered_bytes;                 /* Lifetime successfully copied payload bytes. */
    unsigned long long oldest_milliseconds; /* Oldest queued record age, meaningful only when age_valid. */
    int age_valid;                          /* Queue nonempty and native monotonic time available. */
    int saturated;                          /* At least one lifetime diagnostic counter saturated. */
} StyxRouteQueueMetrics;


/* EN/RU: One coalesced local event, not an S-record schema; no side identity or remote authority implied.
 * Latest snapshot plus all transitions since last read for that queue; generation orders latest transition.
 * Локальное coalesced event, не S-record schema; snapshot и переходы после последнего read данной очереди.
 */
typedef struct StyxRoutePressureEvent
{
    unsigned kind;                 /* GENERAL/UI/SERVICE/TYPED from traffic.h. */
    unsigned index;                /* Typed recipient index, otherwise zero. */
    unsigned changes;              /* OR of HIGH/LOW/CONFIG since last delivery. */
    int high;                      /* Latest warning state, not a debugger conclusion. */
    unsigned long long generation; /* Latest route-local transition generation, saturating. */
    size_t transitions;            /* Coalesced state/policy revision count, saturating. */
    StyxRouteQueueMetrics metrics; /* Latest coherent snapshot, refreshed on next. */
} StyxRoutePressureEvent;


/* EN/RU: Copy local metrics, 1/0; same pinned-owner/kind/index rules as queueinfo, invalid leaves output untouched.
 * Скопировать local metrics, 1/0; правила owner/kind/index как queueinfo, invalid не меняет output.
 */
int styxbidirroutequeuemetrics(StyxBidirRoute *route, unsigned kind, unsigned index, StyxRouteQueueMetrics *output);


/* EN/RU: Atomically replace local limits and reevaluate all inboxes, 1/0; invalid leaves policy/inbox unchanged.
 * Borrow pinned owner/input through return. No wire traffic, callbacks or consumer/RX suspension.
 * Атомарно сменить limits и переоценить inboxes, 1/0; без wire/callbacks/consumer либо RX suspension.
 */
int styxbidirroutepressurelimits(StyxBidirRoute *route, const StyxRoutePressureLimits *limits);


/* EN/RU: Copy one pending native notice, RECORD/EMPTY/END; unchanged output otherwise, no allocation or data drain.
 * One native diagnostics consumer, route/output borrowed through return; pending notices drain after stop.
 * Выдать одно native notice, RECORD/EMPTY/END; pending дочитываются после stop, без data drain/allocation.
 */
int styxbidirroutepressurenext(StyxBidirRoute *route, StyxRoutePressureEvent *output);


/* EN/RU: Block only the diagnostics consumer for notice readiness under one monotonic budget0..120000ms.
 * Borrow route through return, 1 ready/0 timeout or terminal; does not consume notices or stop the broker.
 * Блокировать только diagnostics consumer до notice, 1/0; без consumption либо остановки broker.
 */
int styxbidirroutepressurewait(StyxBidirRoute *route, unsigned timeout);
#ifdef __cplusplus
}
#endif
#endif
