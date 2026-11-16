/* Copyright (c) 2026 bhgv
 * EN: Scalar native remote-event feed policy/status; wire record decoding belongs to host.
 * RU: Scalar native remote-event feed policy/status; wire decoding задаёт host.
 */
#ifndef STYX_FEED_H
#define STYX_FEED_H
#include "events.h"
typedef struct StyxFeed StyxFeed; /* Owned native control/status capability, no provider pointer. */
#define STYX_FEED_ACTIVE 1 /* Worker configured, admission/processing remains asynchronous. */
#define STYX_FEED_EOF 2 /* Remote event resource ended with no partial record. */
#define STYX_FEED_FAILED 3 /* Transport/decode/quota failure; no automatic replay. */
#define STYX_FEED_STOPPED 4 /* Explicit local detach/unmount. */


/* EN/RU: Immutable native policy, adopted only on successful mount/feed construction.
 * Decoder must be bounded, no guest/I/O/reentry; it may not retain borrowed bytes.
 * Native policy immutable; decoder bounded, без guest/I/O/reentry/retained bytes. */
typedef struct StyxFeedConfig
{
    const char *path; /* Borrowed canonical relative event endpoint during construction only. */
    size_t capacity; /* Bounded assembly buffer1..65536 bytes, including partial record. */
    unsigned kinds; /* Nonzero subset of STYX_EVENT_ALL; local invalidation classes allowed. */
    int resources; /*0 clears remote scope,1 explicitly accepts decoder-mapped provider IDs. */
    int (*decode)(void *, const void *, size_t, StyxEvent *, size_t *); /*1 record/consumed,0 need more/zero,-1 invalid. */
    void *state; /* Adopted native context; must outlive all decoder calls. */
    void (*release)(void *); /* Final nonwaiting context cleanup after worker/control retirement. */
} StyxFeedConfig;


/* EN/RU: Copied local status, source cursor and accepted remote record count.
 * Копия local status, source cursor и числа accepted remote records. */
typedef struct StyxFeedInfo
{
    int state; /* ACTIVE/EOF/FAILED/STOPPED. */
    int finished; /* Native read/fid/client cleanup completed; no worker join implied. */
    unsigned long long session; /* Last accepted remote journal epoch, independent of local epoch. */
    unsigned long long generation; /* Last accepted remote report, not a local journal generation. */
    unsigned long long records; /* Valid strictly newer remote reports, including filtered classes. */
} StyxFeedInfo;


/* EN/RU: Retain a borrowed live control capability;1/0. No I/O or callbacks.
 * Удержать borrowed control capability;1/0, без I/O/callbacks. */
int styxfeedretain(StyxFeed *feed);


/* EN/RU: Request native read cancellation without waiting; admitted decoder/publication may finish.
 * Запросить cancel без ожидания; admitted decoder/publication может завершиться позже. */
void styxfeedstop(StyxFeed *feed);


/* EN/RU: Consume owner ref; final owner requests stop without joining/waiting for RPC.
 * Поглотить owner ref; последний запрашивает stop без join/RPC wait. */
void styxfeedfree(StyxFeed *feed);


/* EN/RU: Copy native status under its own mutex;1/0, output untouched on invalid input.
 * Копировать native status под собственным mutex;1/0, invalid не меняет output. */
int styxfeedinfo(StyxFeed *feed, StyxFeedInfo *output);
#endif
