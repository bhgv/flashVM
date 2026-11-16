/* Copyright (c) 2026 bhgv
 * EN: Bounded independent native event subscribers sharing one explicit view policy.
 * RU: Ограниченные независимые native подписчики с общей явной политикой view.
 */
#ifndef STYX_SUBSCRIPTIONS_H
#define STYX_SUBSCRIPTIONS_H
#include "events_provider.h"
typedef struct StyxSubscriptions StyxSubscriptions; /* Owned factory authority; caller pins an owner across every call. */


/* EN/RU: Copied factory admission status; active includes setup and retiring callbacks.
 * Копия admission status; active включает setup и завершающиеся callbacks. */
typedef struct StyxSubscriptionInfo
{
    size_t limit; /* Maximum simultaneous allocated subscriptions,1..1024. */
    size_t active; /* Slots held until source policy/callback retirement. */
    int closed; /* New subscriptions permanently refused; existing streams remain independent. */
} StyxSubscriptionInfo;


/* EN/RU: Retain borrowed authorized journal, adopt policy.state only on success; owned result/NULL.
 * Policy callbacks are shared/thread-safe, including simultaneous encoding on independent readers.
 * No subscription or reserved path is created; limit1..1024 bounds per-source buffers/fds.
 * Удержать authorized journal, state принять только при успехе; свой result/NULL, limit1..1024. */
StyxSubscriptions *styxsubscriptions(DVM_Changes *journal, const StyxEventPolicy *policy, size_t limit);


/* EN/RU: Retain borrowed live factory;1/0, no callbacks or I/O.
 * Удержать borrowed live factory;1/0, без callbacks/I/O. */
int styxsubscriptionsretain(StyxSubscriptions *factory);


/* EN/RU: Consume owner; last owner closes admission without waiting for subscribers/publishers.
 * Поглотить owner; последний закрывает admission без ожидания subscribers/publishers. */
void styxsubscriptionsfree(StyxSubscriptions *factory);


/* EN/RU: Permanently close admission nonwaiting; already admitted setup may return a live stream.
 * Existing streams continue until their own finish/free; no subscriber pointers are retained here.
 * Навсегда закрыть admission без ожидания; admitted setup/существующие streams остаются независимыми. */
void styxsubscriptionsclose(StyxSubscriptions *factory);


/* EN/RU: Copy status under native mutex;1/0, invalid leaves output unchanged.
 * Копировать status под native mutex;1/0, invalid не меняет output. */
int styxsubscriptionsinfo(StyxSubscriptions *factory, StyxSubscriptionInfo *output);


/* EN/RU: Return owned independent inbox/NULL; no wait for capacity/events, no guest work.
 * Native allocation/subscription invokes policy.allow for initial snapshot; failures release slot.
 * Each source retains its own journal cursor, bounded queue, cache and readiness descriptor.
 * Вернуть свой независимый inbox/NULL; без ожидания места/events, failure освобождает slot. */
StyxEvents *styxsubscribe(StyxSubscriptions *factory);
#endif
