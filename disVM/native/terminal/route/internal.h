/* Copyright (c) 2026 bhgv
 * EN: Private dispatcher ownership and mutex-protected bounded routing queues.
 * RU: Владение диспетчером и ограниченные очереди маршрутизации под mutex.
 */
#ifndef STYX_BIDIR_ROUTE_INTERNAL_H
#define STYX_BIDIR_ROUTE_INTERNAL_H
#include "types.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN/RU: One immutable schema and its mutex-protected raw FIFO; no payload-specific interpretation.
 * Неизменяемая схема и её raw FIFO под mutex, без толкования содержимого payload.
 */
typedef struct RouteTypedQueue
{
    char type[FVM_SBINDINGS_TYPE_MAX + 1]; /* Owned immutable schema identity. */
    unsigned long version; /* Immutable schema version. */
    unsigned char *raw; /* Owned slots*limit copied payload storage. */
    size_t sizes[32]; /* Payload byte counts under route lock. */
    unsigned long tags[32]; /* Original numeric RX tags under route lock. */
    unsigned first; /* Oldest accepted record under route lock. */
    unsigned count; /* Occupancy under route lock. */
} RouteTypedQueue;


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
    char type[FVM_SBINDINGS_TYPE_MAX + 1]; /* Immutable negotiated UI schema identity, empty when disabled. */
    unsigned long version; /* Immutable negotiated UI schema version, zero when disabled. */
    FVM_SBinding bindings[FVM_SBINDINGS_MAX]; /* Worker-only bounded RX associations. */
    FVM_SBindings incoming; /* Worker-only registry; never used for TX authority. */
    DVM_UIMessage *ui; /* Owned copied UI queue, NULL for data-only sessions. */
    unsigned ui_first; /* Oldest UI entry, under lock. */
    unsigned ui_count; /* Accepted unread UI entries, under lock. */
    unsigned char *raw; /* Owned general queue bytes, slots*limit. */
    size_t sizes[32]; /* General queue active byte counts, under lock. */
    unsigned long tags[32]; /* General queue tags, under lock. */
    unsigned first; /* Oldest general entry, under lock. */
    unsigned count; /* Accepted unread general entries, under lock. */
    unsigned char *scratch; /* Worker-only record storage. */
    unsigned char *encoded; /* Worker-only framing of a tag-zero declaration. */
    RouteTypedQueue *typed; /* Owned immutable array; each FIFO is protected by route lock. */
    unsigned typed_count; /* Immutable number of typed queues. */
};


/* EN/RU: Latch reason and wake waiters, then cancel owned transport; borrowed route.
 * Зафиксировать reason и разбудить ожидания, затем отменить свой transport.
 */
void routeend(StyxBidirRoute *route, int reason);


/* EN/RU: Native RX entry, borrowing route through join; never invokes guest callbacks.
 * Native RX-диспетчер, route заимствован до join, guest callbacks отсутствуют.
 */
void *routedispatch(void *route);


/* EN/RU: Validate schema identity/version before native I/O; 1 valid/0 invalid, no allocation.
 * Проверить identity/version до native I/O, 1/0, без allocation; type заимствован до возврата.
 */
int routeschema(const char *type, unsigned long version);


/* EN/RU: Native wait on borrowed lock-protected count; one monotonic budget, 1 ready/0 otherwise.
 * Native-ожидание заимствованного count под lock, один monotonic budget, 1 есть запись/0 иначе.
 */
int routewaitcount(StyxBidirRoute *route, unsigned *count, unsigned timeout);


/* EN/RU: Dispatcher-only copy of scratch to queue; 1 published/0 stopped; may wait for capacity.
 * Только dispatcher: копия scratch в queue, 1 принято/0 stop; возможно ожидание свободного места.
 */
int routepublishtyped(StyxBidirRoute *route, RouteTypedQueue *queue, unsigned long tag, size_t size);
#endif
