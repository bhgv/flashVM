/* Copyright (c) 2026 bhgv
 * EN: Server-only retirement subscription faults and late admitted callbacks, outside production code.
 * RU: Отказы server retirement subscription и поздние admitted callbacks только в fixture.
 */
#include "route/route.h"
#include <stdio.h>
#include <stdlib.h>
#ifndef STYX_ACTIVE_IO_CASE
#define STYX_ACTIVE_IO_CASE 0 /* Ordinary native route subscription. */
#endif
static unsigned refused; /* Constructor owner only, single injected refusal. */


/* EN/RU: Test source pins original callback state until explicit post-server-free notification delivery.
 * Test source удерживает original callback state до явной post-server-free delivery.
 */
typedef struct Deferred
{
    void (*receive)(void *); /* Original source-only cancellation callback. */
    void (*release)(void *); /* Original independent source pin release. */
    void *state;             /* Retained cancellation source, never a server/route pointer. */
    unsigned noticed;        /* Atomic admission count of delayed notices. */
    unsigned released;       /* Atomic marker that source tap was detached. */
} Deferred;
static Deferred deferred; /* Case 12 alone holds a delayed source subscription. */


/* EN/RU: Native admission marker, no original state access until test delivery after server free.
 * Native admission marker, без original state access до test delivery после server free.
 */
static void delayed(void *state)
{
    Deferred *notice = state; /* Static native fixture storage. */
    __atomic_add_fetch(&notice->noticed, 1, __ATOMIC_RELEASE);
}


/* EN/RU: Keep original state pinned for already-admitted test callbacks after actual unsubscribe.
 * Удержать original state для admitted test callbacks после настоящего unsubscribe.
 */
static void delayed_release(void *state)
{
    Deferred *notice = state; /* Static native fixture storage. */
    __atomic_store_n(&notice->released, 1, __ATOMIC_RELEASE);
}


/* EN/RU: Deliver duplicate admitted notices after server disposal, then release final independent source pin.
 * Выдать duplicate admitted notices после server disposal, затем освободить отдельный source pin.
 */
void activelate(void)
{
    if (!deferred.state || !__atomic_load_n(&deferred.noticed, __ATOMIC_ACQUIRE) ||
        !__atomic_load_n(&deferred.released, __ATOMIC_ACQUIRE))
    {
        fputs("active resource I/O: missing deferred subscription ownership\n", stderr);
        exit(2);
    }
    deferred.receive(deferred.state);
    deferred.receive(deferred.state);
    deferred.release(deferred.state);
    deferred.state = NULL;
}


/* EN/RU: Original sticky subscription, callback state adopted only on success.
 * Исходная sticky subscription, callback state принят только при успехе.
 */
extern struct DVM_CancelTap *__real_styxbidirroutewatch(StyxBidirRoute *, void (*)(void *), void *, void (*)(void *));


/* EN/RU: Delay case 12; refuse first case 13 call without adoption; otherwise forward unchanged.
 * Отложить case 12; отказать первому call case 13 без adoption; прочие calls без изменений.
 */
struct DVM_CancelTap *__wrap_styxbidirroutewatch(StyxBidirRoute *route, void (*receive)(void *),
    void *state, void (*release)(void *))
{
    if (STYX_ACTIVE_IO_CASE == 13 && !refused++)
    {
        return NULL;
    }
    if (STYX_ACTIVE_IO_CASE == 12)
    {
        deferred.receive = receive;
        deferred.release = release;
        deferred.state = state;
        return __real_styxbidirroutewatch(route, delayed, &deferred, delayed_release);
    }
    return __real_styxbidirroutewatch(route, receive, state, release);
}
