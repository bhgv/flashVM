/* Copyright (c) 2026 bhgv
 * EN: Deterministic native service-admission stall and thread-local allocation refusals, no fake wire success.
 * RU: Детерминированный native service-admission stall и thread-local allocation refusals без fake wire success.
 */
#include "scope.h"
#include "cancel.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Protect actual native service admission checkpoint. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Main releases the test-held sender, never broker callback. */
static int armed;       /* Next receiver confirmation is held before native control admission. */
static int entered;     /* Sender reached held confirmation. */
static int released;    /* Native host explicitly releases the test-held sender. */
static __thread int malloc_failed; /* Refuse next allocation only on the arming native thread. */
static __thread int calloc_failed; /* Refuse next handler-context allocation on this native thread. */


/* EN/RU: Original allocator, normal allocation ownership unchanged.
 * Исходный allocator, обычный allocation ownership не меняется.
 */
extern void *__real_malloc(size_t);


/* EN/RU: Original zeroed allocator, normal allocation ownership unchanged.
 * Исходный zeroed allocator, обычный ownership не меняется.
 */
extern void *__real_calloc(size_t, size_t);


/* EN/RU: Original bounded native control admission, copied frame only on successful return.
 * Исходный bounded native control admission, frame copied лишь при successful return.
 */
extern int __real_styxbidircontrolsendwait(StyxBidirControl *, unsigned long, const void *, size_t, unsigned, DVM_Cancel *);


/* EN/RU: Arm one actual allocation refusal on caller thread; kind0 malloc, kind1 calloc.
 * Включить один настоящий allocation refusal на caller thread; kind0 malloc, kind1 calloc.
 */
void operationallocation(int kind)
{
    if (kind)
    {
        calloc_failed = 1;
    }
    else
    {
        malloc_failed = 1;
    }
}


/* EN/RU: Refuse only armed native allocation, all remaining paths call the original allocator.
 * Отказать лишь armed native allocation, остальные paths используют original allocator.
 */
void *__wrap_malloc(size_t size)
{
    if (malloc_failed)
    {
        malloc_failed = 0;
        return NULL;
    }
    return __real_malloc(size);
}


/* EN/RU: Refuse only armed native zeroed allocation, state ownership stays with refused caller.
 * Отказать лишь armed zeroed allocation, state ownership остаётся у refused caller.
 */
void *__wrap_calloc(size_t count, size_t size)
{
    if (calloc_failed)
    {
        calloc_failed = 0;
        return NULL;
    }
    return __real_calloc(count, size);
}


/* EN/RU: Arm before sending RX-change request, no scheduler polling or production timeout mutation.
 * Включить до RX-change request, без scheduler polling либо изменения production timeout.
 */
void operationgatearm(void)
{
    pthread_mutex_lock(&lock);
    armed = 1;
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Wait for original native sender to reach held ACK/NACK, then release; never blocks the broker.
 * Ждать native sender в held ACK/NACK и отпустить; broker не блокируется.
 */
void operationgaterelease(void)
{
    pthread_mutex_lock(&lock);
    while (!entered)
    {
        pthread_cond_wait(&changed, &lock);
    }
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Hold only native service worker before original admission, then perform actual original Styx I/O.
 * Удержать лишь native service worker до original admission, затем настоящее original Styx I/O.
 */
int __wrap_styxbidircontrolsendwait(StyxBidirControl *control, unsigned long tag, const void *bytes,
    size_t size, unsigned timeout, DVM_Cancel *cancel)
{
    pthread_mutex_lock(&lock);
    int reply = !tag && size == 7 && (!memcmp(bytes, "rx-ok:4", 7) || !memcmp(bytes, "rx-no:4", 7));
    if (armed && reply)
    {
        armed = 0;
        entered = 1;
        pthread_cond_broadcast(&changed);
        while (!released)
        {
            pthread_cond_wait(&changed, &lock);
        }
    }
    pthread_mutex_unlock(&lock);
    return __real_styxbidircontrolsendwait(control, tag, bytes, size, timeout, cancel);
}
