/* Copyright (c) 2026 bhgv
 * EN: Native system-service inbox and copied queue metrics, independent of application consumption.
 * RU: Native inbox системных служб и метрики очередей независимо от потребления приложениями.
 */
#ifndef STYX_ROUTE_TRAFFIC_H
#define STYX_ROUTE_TRAFFIC_H
#include "types.h"
#ifdef __cplusplus
extern "C" {
#endif
#define STYX_ROUTE_GENERAL 0U /* Application records without a registered recipient; legacy binding copies. */
#define STYX_ROUTE_UI 1U      /* Decoded UI recipient messages. */
#define STYX_ROUTE_SERVICE 2U /* Non-binding tag-zero records, native channel-service owner only. */
#define STYX_ROUTE_TYPED 3U   /* Registered recipient selected by immutable local index. */


/* EN/RU: Copied local counters, never a wire format; payload bytes exclude native allocation metadata.
 * Локальные counters-копии, не wire format; bytes без native allocation metadata.
 */
typedef struct StyxRouteQueueInfo
{
    size_t records;      /* Current unread message count. */
    size_t bytes;        /* Current unread payload bytes, UI uses decoded native message extent. */
    size_t peak_records; /* Maximum message count since route construction. */
    size_t peak_bytes;   /* Maximum payload bytes since route construction. */
} StyxRouteQueueInfo;


/* EN/RU: Copy one coherent snapshot, 1/0; invalid input leaves output untouched, no I/O/consumption.
 * Borrow pinned route/output; kind above, index only for TYPED, otherwise must be zero.
 * Копировать согласованный snapshot, 1/0; invalid не меняет output. Index лишь TYPED, иначе 0.
 */
int styxbidirroutequeueinfo(StyxBidirRoute *route, unsigned kind, unsigned index, StyxRouteQueueInfo *output);


/* EN/RU: Native channel-service owner reads one non-binding tag-zero body; same SMALL/EMPTY/END contract.
 * No application dispatch or authority from the body; host handler validates its agreed service format.
 * Borrow route/outputs through return, one consumer; new schemas are not guessed or executed here.
 * Native владелец служб читает один non-binding tag0 body; SMALL/EMPTY/END как next, без guest dispatch.
 */
int styxbidirrouteservicenext(StyxBidirRoute *route, void *bytes, size_t capacity, size_t *size);


/* EN/RU: Native service-consumer readiness, one monotonic timeout0..120000ms; no consumption or RX pause.
 * Native readiness владельца служб, timeout0..120000ms, без consumption либо RX pause.
 */
int styxbidirrouteservicewait(StyxBidirRoute *route, unsigned timeout);
#ifdef __cplusplus
}
#endif
#endif
