/* FlashVM: Portable platform adapter interface or implementation: thread.
 * File: ports/linux/thread.c.
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#include "../thread_port.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>


/* Define FVM_PortThread struct and its representation. */
struct FVM_PortThread
{
    pthread_t thread;
    FVM_PortThreadFn fn;
    void *arg;
};


/* Compute and return thread entry. */
static void *thread_entry(void *arg)
{
    FVM_PortThread *t = (FVM_PortThread *)arg;
    t->fn(t->arg);
    return 0;
}


/* Compute and return port thread start. */
FVM_PortThread *fvm_port_thread_start(FVM_PortThreadFn fn, void *arg)
{
    FVM_PortThread *t;
    if (!fn)
    {
        return 0;
    }
    t = (FVM_PortThread *)malloc(sizeof(*t));
    if (!t)
    {
        return 0;
    }

    /* Computation / Вычисление и обновление состояния. */
    t->fn = fn;
    t->arg = arg;
    if (pthread_create(&t->thread, 0, thread_entry, t) != 0)
    {
        free(t);
        return 0;
    }
    return t;
}


/* Join port thread. */
void fvm_port_thread_join(FVM_PortThread *t)
{
    if (t)
    {
        pthread_join(t->thread, 0);
        free(t);
    }
}


/* Test whether port thread current. */
int fvm_port_thread_is_current(FVM_PortThread *t)
{
    return t && pthread_equal(pthread_self(), t->thread);
}


/* Sleep for port thread us. */
void fvm_port_thread_sleep_us(unsigned long us)
{
    struct timespec delay, remaining;
    /* Bound each time_t conversion even on 32-bit ports. */
    while (us)
    {
        unsigned long chunk = us > 1000000UL ? 1000000UL : us;
        delay.tv_sec = (time_t)(chunk / 1000000UL);
        delay.tv_nsec = (long)(chunk % 1000000UL) * 1000L;
        while (nanosleep(&delay, &remaining) != 0 && errno == EINTR)
        {
            delay = remaining;
        }
        us -= chunk;
    }
}
