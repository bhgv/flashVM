/* Copyright (c) 2026 bhgv
 * EN: Private dispatcher ownership and mutex-protected dynamic recipient queues.
 * RU: Владение диспетчером и динамические очереди адресатов под mutex.
 */
#ifndef STYX_BIDIR_ROUTE_INTERNAL_H
#define STYX_BIDIR_ROUTE_INTERNAL_H
#include "types.h"
#include "mailbox.h"
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
    unsigned long version;                 /* Immutable schema version. */
    RouteMailbox mailbox;                  /* Dynamically growing copied FIFO, protected by route lock. */
} RouteTypedQueue;


/* EN/RU: Native state retained until the dispatcher and all callers retire.
 * Native-состояние живёт до завершения диспетчера и всех пользователей.
 */
struct StyxBidirRoute
{
    StyxBidirControl *control;                /* Owned transport; never exposed to a second reader. */
    struct RouteRetirement *retirement;       /* Subscription-owned lifetime fence, detached before control disposal. */
    struct DVM_CancelTap *retired;            /* Owned sticky transport retirement subscription. */
    pthread_mutex_t lock;                     /* Protects queue indices and first terminal reason. */
    pthread_cond_t changed;                   /* Monotonic recipient readiness and terminal wakeups. */
    pthread_t worker;                         /* Sole RX dispatcher; no VM/SDL access. */
    unsigned slots;                           /* Underlying transport staging slots, 1..32; never a recipient limit. */
    unsigned limit;                           /* Maximum borrowed record payload capacity. */
    int reason;                               /* First terminal reason; earlier queued items remain readable. */
    char type[FVM_SBINDINGS_TYPE_MAX + 1];    /* Immutable negotiated UI schema identity, empty when disabled. */
    unsigned long version;                    /* Immutable negotiated UI schema version, zero when disabled. */
    FVM_SBinding bindings[FVM_SBINDINGS_MAX]; /* Worker-only bounded RX associations. */
    FVM_SBindings incoming;                   /* Worker-only registry; never used for TX authority. */
    RouteMailbox ui;                          /* Copied decoded UI messages, protected by route lock. */
    RouteMailbox general;                     /* General records and legacy binding-observation copies. */
    RouteMailbox service;                     /* Non-binding tag-zero service records for native channel handlers. */
    unsigned char *scratch;                   /* Worker-only record storage. */
    unsigned char *encoded;                   /* Worker-only framing of a tag-zero declaration. */
    RouteTypedQueue *typed;                   /* Owned immutable array; each FIFO is protected by route lock. */
    unsigned typed_count;                     /* Immutable number of typed queues. */
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
int routewaitcount(StyxBidirRoute *route, size_t *count, unsigned timeout);


/* EN/RU: Dispatcher-only copy of scratch to queue; 1 published/0 stopped or allocation failure; no capacity wait.
 * Только dispatcher: копия scratch в queue, 1 принято/0 stop; без ожидания свободного места.
 */
int routepublishtyped(StyxBidirRoute *route, RouteTypedQueue *queue, unsigned long tag, size_t size);


/* EN/RU: Attach one native retirement callback after storage/control setup; 1/0, constructor only.
 * Подписаться после setup storage/control; 1/0, только constructor, без guest/I/O.
 */
int routeretirementattach(StyxBidirRoute *route);


/* EN/RU: Detach callback route access before disposal; no worker/caller may still use route.
 * Отделить route от callback до disposal, только construction rollback либо joined lifecycle.
 */
void routeretirementdetach(StyxBidirRoute *route);
#endif
