/* Copyright (c) 2026 bhgv
 * EN: Refuse one UI relay retirement subscription, preserving unadopted native callback state.
 * RU: Отклонить одну retirement subscription UI relay, сохранив caller state при отказе.
 */
#include "route/route.h"
#include <stdio.h>
#include <stdlib.h>
#ifndef STYX_UI_BRIDGE_CASE
#define STYX_UI_BRIDGE_CASE 11 /* Single constructor subscription refusal. */
#endif
static unsigned refused; /* Constructor owner only; no other routewatch caller in this fixture. */


/* EN/RU: Original sticky route subscription; borrowed callback/state until successful adoption.
 * Исходная sticky route subscription, callback/state borrowed до успешного adoption.
 */
extern struct DVM_CancelTap *__real_styxbidirroutewatch(StyxBidirRoute *, void (*)(void *), void *, void (*)(void *));


/* EN/RU: Retain callback state until a test-admitted notification is explicitly delivered after free.
 * Удержать callback state до test-admitted notification, явно выдаваемого после free.
 */
typedef struct Deferred
{
    void (*receive)(void *); /* Original relay native wake. */
    void (*release)(void *); /* Original independent fence disposal. */
    void *state; /* Original fence retained through late callback. */
    unsigned noticed; /* Atomic native notification count. */
    unsigned released; /* Atomic tap retirement marker. */
} Deferred;
static Deferred deferred; /* One private subscription in late-delivery case 13. */


/* EN/RU: Admit test notice, delaying original state access; native nonwaiting, no route/VM I/O.
 * Допустить test notice, отложив original state access; native без ожидания/route/VM I/O.
 */
static void delayed(void *state)
{
    Deferred *notice = state; /* Static fixture storage. */
    __atomic_add_fetch(&notice->noticed, 1, __ATOMIC_RELEASE);
}


/* EN/RU: Preserve original context for the already-admitted test callback, mark subscription detached.
 * Удержать original context для admitted test callback, отметить detached subscription.
 */
static void delayed_release(void *state)
{
    Deferred *notice = state; /* Static fixture storage. */
    __atomic_store_n(&notice->released, 1, __ATOMIC_RELEASE);
}


/* EN/RU: Invoke delayed duplicates after bridge, channels and route are freed, then release private fence.
 * Выдать delayed duplicates после free bridge/channels/route, затем освободить private fence.
 */
void terminalbridgelate(void)
{
    if (!deferred.state || !__atomic_load_n(&deferred.noticed, __ATOMIC_ACQUIRE) ||
        !__atomic_load_n(&deferred.released, __ATOMIC_ACQUIRE))
    {
        fputs("terminal relay: missing delayed callback ownership\n", stderr);
        exit(2);
    }
    deferred.receive(deferred.state);
    deferred.receive(deferred.state);
    deferred.release(deferred.state);
    deferred.state = NULL;
}


/* EN/RU: Case 11 refuses first constructor call without adoption; case 13 retains a delayed native notice.
 * Case 11 отклоняет первый constructor call без adoption; case 13 удерживает delayed native notice.
 */
struct DVM_CancelTap *__wrap_styxbidirroutewatch(StyxBidirRoute *route, void (*receive)(void *),
    void *state, void (*release)(void *))
{
    if (STYX_UI_BRIDGE_CASE == 11 && !refused++)
    {
        return NULL;
    }
    if (STYX_UI_BRIDGE_CASE == 13)
    {
        deferred.receive = receive;
        deferred.release = release;
        deferred.state = state;
        return __real_styxbidirroutewatch(route, delayed, &deferred, delayed_release);
    }
    return __real_styxbidirroutewatch(route, receive, state, release);
}
