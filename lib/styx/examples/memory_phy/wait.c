/* Copyright (c) 2026 bhgv
 * EN: Linux no-descriptor wait/wake: a binary futex latch prevents pre-wait lost
 * wakeups; relative kernel waits share one CLOCK_MONOTONIC deadline.
 * RU: Linux wait/wake без fd: binary futex latch исключает pre-wait lost wakeup;
 * относительные kernel waits используют один CLOCK_MONOTONIC deadline.
 * standalone-reviewed: the 32-bit word is a binary notification, not a queue ordinal.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "internal.h"
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <errno.h>


/* EN/RU: See internal.h; store before kernel wake so notification survives entry races.
 * См. internal.h; store до kernel wake сохраняет уведомление при гонке входа.
 */
void memory_notify(MemoryEnd *end)
{
    __atomic_store_n(&end->event, 1, __ATOMIC_RELEASE);
    syscall(SYS_futex, &end->event, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, 0); /* Never wait for the remote owner. */
}


/* EN: state is an endpoint pinned by the caller. Publish sticky local work before
 * notifying; no mutex/descriptor/protocol operation is permitted on this path.
 * RU: state — удерживаемый endpoint. Опубликовать sticky local work до notify;
 * на этом пути нет mutex/fd/protocol operations.
 */
void memory_wake(void *state)
{
    MemoryEnd *end = state;
    __atomic_store_n(&end->wake, 1, __ATOMIC_RELEASE);
    memory_notify(end);
}


/* EN: Borrow end/wanted; inspect persistent ring/peer state and consume one sticky
 * local wake. Return readiness bits; one native I/O owner serializes this function.
 * RU: end/wanted заимствованы; проверить persistent ring/peer state и поглотить
 * sticky local wake. Вернуть readiness bits; вызовы сериализует один I/O owner.
 */
static unsigned readiness(MemoryEnd *end, unsigned wanted)
{
    MemoryLink *link = end->link;
    pthread_mutex_lock(&link->lock);
    unsigned side = end->side;
    unsigned ready = 0;
    if ((wanted & PHY_READ) && (link->used[side] || !link->open[1 - side]))
    {
        ready |= PHY_READ; /* Includes explicit EOF readiness after a drained peer disconnect. */
    }
    if ((wanted & PHY_WRITE) && link->open[1 - side] && link->used[1 - side] < MEMORY_CAPACITY)
    {
        ready |= PHY_WRITE;
    }
    if (!link->open[1 - side])
    {
        ready |= PHY_HANGUP; /* Does not discard a still-readable incoming prefix. */
    }
    pthread_mutex_unlock(&link->lock);
    if (__atomic_exchange_n(&end->wake, 0, __ATOMIC_ACQ_REL))
    {
        ready |= PHY_WAKE;
    }
    return ready;
}


/* EN: state/wanted/timeout/ready implement PhyOps.wait. timeout=-1 waits indefinitely,
 * 0 polls, positive ms bound all retries by one monotonic deadline. Return PHY_OK
 * with readiness (possibly zero at timeout), otherwise PHY_ERROR without publication.
 * RU: state/wanted/timeout/ready реализуют PhyOps.wait. timeout=-1 — бесконечно,
 * 0 — poll, positive ms ограничивает все повторы одним monotonic deadline.
 * Вернуть PHY_OK с readiness (возможно ноль при timeout), иначе PHY_ERROR без output.
 */
int memory_wait(void *state, unsigned wanted, int timeout, unsigned *ready)
{
    MemoryEnd *end = state;
    struct timespec deadline = {0, 0};
    if (timeout >= 0)
    {
        if (clock_gettime(CLOCK_MONOTONIC, &deadline))
        {
            return PHY_ERROR;
        }
        deadline.tv_sec += timeout / 1000;
        deadline.tv_nsec += (timeout % 1000) * 1000000L;
        if (deadline.tv_nsec >= 1000000000L)
        {
            deadline.tv_sec++;
            deadline.tv_nsec -= 1000000000L;
        }
    }
    for (;;)
    {
        __atomic_store_n(&end->event, 0, __ATOMIC_RELEASE); /* Reset before inspecting persistent predicates, never after them. */
        unsigned observed = readiness(end, wanted);
        if (observed || timeout == 0)
        {
            *ready = observed;
            return PHY_OK;
        }
        struct timespec remaining;
        struct timespec *wait_time = NULL;
        if (timeout > 0)
        {
            struct timespec now;
            if (clock_gettime(CLOCK_MONOTONIC, &now))
            {
                return PHY_ERROR;
            }
            remaining.tv_sec = deadline.tv_sec - now.tv_sec;
            remaining.tv_nsec = deadline.tv_nsec - now.tv_nsec;
            if (remaining.tv_nsec < 0)
            {
                remaining.tv_sec--;
                remaining.tv_nsec += 1000000000L;
            }
            if (remaining.tv_sec < 0 || (!remaining.tv_sec && !remaining.tv_nsec))
            {
                *ready = 0;
                return PHY_OK;
            }
            wait_time = &remaining;
        }
        int result = syscall(SYS_futex, &end->event, FUTEX_WAIT_PRIVATE, 0, wait_time, NULL, 0);
        if (result < 0 && errno != EAGAIN && errno != EINTR && errno != ETIMEDOUT)
        {
            return PHY_ERROR;
        }
        /* A notifier between predicate check and kernel entry changes event to one:
         * the kernel refuses to sleep on expected zero. Recheck predicates after every return. */
    }
}
