/* Copyright (c) 2026 bhgv
 * EN: Native condition waits are woken by per-operation cancellation, never guest cooperation.
 * RU: Native condition waits будятся per-operation cancellation, без участия guest.
 */
#include "gate.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: Static fixture storage outlives all native callbacks, including nonwaiting unsubscribe races.
 * Static fixture storage живёт дольше всех native callbacks, включая unsubscribe races.
 */
typedef struct Gate
{
    int entered;   /* Native operation subscribed, under lock. */
    int cancelled; /* Only this operation's cancellation callback, under lock. */
    int released;  /* Main permits independent operation, under lock. */
} Gate;
static Gate gates[2]; /* Server scope[0] and independent caller scope[1]. */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Protects both gates. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Native readiness, cancellation and explicit release. */


/* EN/RU: Short native callback wakes one static gate; no provider/transport/VM calls or joins.
 * Короткий native callback будит один static gate, без provider/transport/VM calls либо joins.
 */
static void cancelled(void *state)
{
    Gate *gate = state; /* Static native state, never a provider or guest pointer. */
    pthread_mutex_lock(&lock);
    gate->cancelled = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
}


/* EN/RU: See gate.h; adopt no caller pointers beyond subscription; scope owner keeps cancellation source alive.
 * См. gate.h; без хранения caller pointers вне subscription; scope owner удерживает cancel source.
 */
int activegate(unsigned index)
{
    DVM_Cancel *source = dvm_cancel_current(); /* Borrowed under this native operation's scope. */
    if (index > 1 || !source)
    {
        fputs("active resource I/O: missing native cancellation scope\n", stderr);
        exit(2);
    }
    Gate *gate = &gates[index]; /* Static per-operation gate. */
    DVM_CancelTap *tap = dvm_cancel_subscribe(source, cancelled, gate, NULL); /* Pins only this source. */
    if (!tap)
    {
        fputs("active resource I/O: gate subscription failed\n", stderr);
        exit(2);
    }
    pthread_mutex_lock(&lock);
    gate->entered = 1;
    pthread_cond_broadcast(&changed);
    while (!gate->cancelled && !gate->released)
    {
        pthread_cond_wait(&changed, &lock);
    }
    int result = !gate->cancelled; /* Main release is distinct from cancellation. */
    pthread_mutex_unlock(&lock);
    dvm_cancel_unsubscribe(tap);
    return result;
}


/* EN/RU: See gate.h; external real-peer process timeout bounds a broken native provider test.
 * См. gate.h; process timeout real-peer fixture ограничивает сломанный native provider test.
 */
void activeentered(void)
{
    pthread_mutex_lock(&lock);
    while (!gates[0].entered || !gates[1].entered)
    {
        pthread_cond_wait(&changed, &lock);
    }
    pthread_mutex_unlock(&lock);
}


/* EN/RU: See gate.h; independent waiter remains blocked until this explicit test release.
 * См. gate.h; независимый waiter остаётся blocked до данного явного test release.
 */
void activeindependent(void)
{
    pthread_mutex_lock(&lock);
    if (!gates[0].cancelled || gates[1].cancelled || gates[1].released)
    {
        fputs("active resource I/O: cancellation escaped its operation\n", stderr);
        exit(2);
    }
    gates[1].released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
}
