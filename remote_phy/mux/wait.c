/* Copyright (c) 2026 bhgv
 * EN: Linux native generation wakeups: no producer mutex and no wake-before-wait race.
 * RU: Linux native generation wakeups: без producer mutex и без wake-before-wait race.
 */
#define _GNU_SOURCE
#include "internal.h"
#include <sys/syscall.h>
#include <linux/futex.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>


/* EN/RU: Publish a generation before kernel wake; caller pins channel storage, no mutex/wait.
 * Опубликовать generation перед kernel wake; storage pinned, без mutex/wait. */
void muxsignalchannel(MuxChannel *channel)
{
    __atomic_add_fetch(&channel->generation, 1, __ATOMIC_RELEASE);
    syscall(SYS_futex, &channel->generation, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, 0);
}


/* EN/RU: Signal both endpoint readiness predicates; caller holds hub lock or native owner ref.
 * Пробудить оба endpoint readiness predicates; caller держит hub lock либо owner ref. */
void muxsignal(PhyMux *hub)
{
    muxsignalchannel(&hub->channels[0]);
    muxsignalchannel(&hub->channels[1]);
}


/* EN/RU: Sleep only if generation still matches; relative timeout uses CLOCK_MONOTONIC in futex.
 * 0 wake/race/interruption/timeout, -1 unexpected kernel error; borrowed optional timeout.
 * Ждать только при совпадении generation; relative monotonic timeout; 0 normal/-1 kernel error. */
int muxsleep(MuxChannel *channel, unsigned generation, const struct timespec *timeout)
{
    int status = (int)syscall(SYS_futex, &channel->generation, FUTEX_WAIT_PRIVATE, generation, timeout, NULL, 0);
    return status == 0 || errno == EAGAIN || errno == EINTR || errno == ETIMEDOUT ? 0 : -1;
}
