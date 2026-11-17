/* Copyright (c) 2026 bhgv
 * EN: Private dispatcher ownership and mutex-protected bounded routing queues.
 * RU: Владение диспетчером и ограниченные очереди маршрутизации под mutex.
 */
#ifndef STYX_BIDIR_ROUTE_INTERNAL_H
#define STYX_BIDIR_ROUTE_INTERNAL_H
#include "route.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN/RU: Native state retained until the dispatcher and all callers retire.
 * Native-состояние живёт до завершения диспетчера и всех пользователей.
 */
struct StyxBidirRoute
{
    StyxBidirControl *control; /* Owned transport; never exposed to a second reader. */
    pthread_mutex_t lock; /* Protects queue indices and first terminal reason. */
    pthread_cond_t changed; /* Monotonic readiness and returned mailbox capacity. */
    pthread_t worker; /* Sole RX dispatcher; no VM/SDL access. */
    unsigned slots; /* Capacity of each queue, 1..32. */
    unsigned limit; /* Maximum borrowed record payload capacity. */
    int reason; /* First terminal reason; earlier queued items remain readable. */
    char type[FVM_SBINDINGS_TYPE_MAX + 1]; /* Immutable negotiated UI schema identity. */
    unsigned long version; /* Immutable negotiated UI schema version. */
    FVM_SBinding bindings[FVM_SBINDINGS_MAX]; /* Worker-only bounded RX associations. */
    FVM_SBindings incoming; /* Worker-only registry; never used for TX authority. */
    DVM_UIMessage *ui; /* Owned copied UI queue. */
    unsigned ui_first; /* Oldest UI entry, under lock. */
    unsigned ui_count; /* Accepted unread UI entries, under lock. */
    unsigned char *raw; /* Owned general queue bytes, slots*limit. */
    size_t sizes[32]; /* General queue active byte counts, under lock. */
    unsigned long tags[32]; /* General queue tags, under lock. */
    unsigned first; /* Oldest general entry, under lock. */
    unsigned count; /* Accepted unread general entries, under lock. */
    unsigned char *scratch; /* Worker-only record storage. */
    unsigned char *encoded; /* Worker-only framing of a tag-zero declaration. */
};


/* EN/RU: Latch reason and wake waiters, then cancel owned transport; borrowed route.
 * Зафиксировать reason и разбудить ожидания, затем отменить свой transport.
 */
void routeend(StyxBidirRoute *route, int reason);


/* EN/RU: Native RX entry, borrowing route through join; never invokes guest callbacks.
 * Native RX-диспетчер, route заимствован до join, guest callbacks отсутствуют.
 */
void *routedispatch(void *route);
#endif
