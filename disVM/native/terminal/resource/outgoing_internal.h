/* Copyright (c) 2026 bhgv
 * EN: Private native offer publisher ownership, queued-generation state and callback lifetime.
 * RU: Private-владение native offer publisher, queued-generation state и callback lifetime.
 */
#ifndef STYX_BIDIR_MOVIE_OUTGOING_INTERNAL_H
#define STYX_BIDIR_MOVIE_OUTGOING_INTERNAL_H
#include "outgoing.h"
#include "../../movie/publisher.h"
#include "internal.h"


/* EN/RU: One native worker reads outbox; host owns final join while source/watch callbacks hold own pins.
 * Один native worker читает outbox, host владеет final join; callbacks имеют отдельные pins.
 */
struct StyxBidirMoviePublisher
{
    pthread_mutex_t lock;          /* Protects first terminal phase and fully queued generation. */
    pthread_cond_t changed;        /* Monotonic native status/generation readiness. */
    pthread_t worker;              /* Sole outbox reader; independent of source/guest execution. */
    StyxBidirRoute *route;         /* Borrowed through final free. */
    StyxEvents *events;            /* Owned independent source outbox retaining trusted movie offer. */
    DVM_Cancel *cancel;            /* Owned source for bounded TX-capacity waits. */
    DVM_CancelTap *retired;        /* Owned route-only terminal subscription, independent of client lifetime. */
    StyxWatch *link;               /* Owned exact-client terminal watch, callback state has no publisher pointer. */
    unsigned long tag;             /* Immutable separately reserved outgoing offer tag. */
    unsigned timeout;              /* Entire declaration/offer-pair admission budget, ms. */
    unsigned long long generation; /* Last pair fully copied to shared TX, initially zero. */
    int phase;                     /* READING or first terminal phase, under lock. */
};


/* EN/RU: Publish first terminal phase, wake waiters; caller borrows publisher, no source/network I/O.
 * Установить первый terminal phase и разбудить waiters, без source/network I/O.
 */
void movie_publisher_end(StyxBidirMoviePublisher *publisher, int phase);


/* EN/RU: Native worker borrows publisher until join; no guest callbacks or competing control writer.
 * Native worker заимствует publisher до join, без guest callbacks/конкурирующего raw writer.
 */
void *movie_publisher_run(void *publisher);
#endif
