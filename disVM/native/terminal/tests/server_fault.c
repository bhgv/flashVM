/* Copyright (c) 2026 bhgv
 * EN: Deterministic native server construction/TX gates; production bytes still use original 9P.
 * RU: Детерминированные gates создания/TX server; production bytes проходят исходный 9P.
 */
#include "client.h"
#include <pthread.h>
#include <string.h>
#include <errno.h>
#ifndef STYX_SERVER_CASE
#define STYX_SERVER_CASE 0 /* No injected fault. */
#endif
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Protects test-only gate state. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Native checkpoint, never a sleep. */
static int armed; /* Main-owned admission published under lock. */
static int entered; /* First injected TX call has reached the gate. */
static int released; /* Main permits the original TX callback to continue. */
#if STYX_SERVER_CASE == 9
static pthread_t owner; /* Only the constructor caller receives the injected thread failure. */
#endif


/* EN/RU: Arm one native fixture fault before server creation, no transport or guest call.
 * Включить один native fault fixture до создания server, без transport/guest call.
 */
void servergatearm(void)
{
    pthread_mutex_lock(&lock);
    armed = 1;
#if STYX_SERVER_CASE == 9
    owner = pthread_self();
#endif
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Native checkpoint for the first blocked TX call; fixture process timeout bounds a broken test.
 * Native checkpoint первого заблокированного TX call; timeout fixture ограничивает зависший тест.
 */
void servergatewait(void)
{
    pthread_mutex_lock(&lock);
    while (!entered)
    {
        pthread_cond_wait(&changed, &lock);
    }
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Release a blocked native TX callback after the independent server worker has joined.
 * Разрешить native TX callback после join независимого worker server.
 */
void servergaterelease(void)
{
    pthread_mutex_lock(&lock);
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
}
#if STYX_SERVER_CASE == 6 || STYX_SERVER_CASE == 7


/* EN/RU: Original precise native Twrite implementation, borrowed buffers/token through return.
 * Исходный precise native Twrite, buffers/token заимствованы до возврата.
 */
extern void __real_styxclientwritecancel(StyxFid *, unsigned long long, const void *, size_t, StyxCancel *, StyxTransfer *);


/* EN/RU: Gate one actual native write or report its confirmed prefix as uncertain, never replay it.
 * Задержать один настоящий write либо отметить подтверждённый prefix uncertain, без replay.
 */
void __wrap_styxclientwritecancel(StyxFid *fid, unsigned long long offset, const void *bytes, size_t size,
    StyxCancel *cancel, StyxTransfer *result)
{
    pthread_mutex_lock(&lock);
    int inject = armed && !entered; /* Only the first admitted terminal TX is affected. */
    if (inject)
    {
        entered = 1;
        pthread_cond_broadcast(&changed);
        while (STYX_SERVER_CASE == 6 && !released)
        {
            pthread_cond_wait(&changed, &lock);
        }
    }
    pthread_mutex_unlock(&lock);
    __real_styxclientwritecancel(fid, offset, bytes, size, cancel, result);
    if (inject && STYX_SERVER_CASE == 7)
    {
        result->success = 0;
        result->uncertain = 1;
    }
}
#endif
#if STYX_SERVER_CASE == 9


/* EN/RU: Original native thread constructor, no ownership transfer on a refused call.
 * Исходный конструктор native thread, при отказе ownership не переносится.
 */
extern int __real_pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);


/* EN/RU: Refuse exactly one constructor thread creation on its main caller, preserving other workers.
 * Отклонить один constructor thread на main caller, сохранив остальные workers.
 */
int __wrap_pthread_create(pthread_t *thread, const pthread_attr_t *attributes, void *(*entry)(void *), void *state)
{
    pthread_mutex_lock(&lock);
    int refuse = armed && pthread_equal(pthread_self(), owner); /* Owner-local deterministic failure. */
    if (refuse)
    {
        armed = 0;
    }
    pthread_mutex_unlock(&lock);
    return refuse ? EAGAIN : __real_pthread_create(thread, attributes, entry, state);
}
#endif
