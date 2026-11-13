/* Copyright (c) 2026 bhgv
 * EN: Platform mutex/condition ownership and bounded acquisition.
 * RU: Владение платформенными mutex/condition и ограниченный захват.
 */


/* FlashVM: Portable platform adapter interface or implementation: channel sync.
 * File: ports/android/channel_sync.c.
 */

#define _POSIX_C_SOURCE 200809L
#include "../channel_sync.h"
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>


/* Define FVM_PortSync struct and its representation. */
struct FVM_PortSync
{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
};


/* Create port sync. */
FVM_PortSync *fvm_port_sync_create(void)
{
    pthread_condattr_t attributes;
    int status;
    FVM_PortSync *s = (FVM_PortSync *)malloc(sizeof(*s));
    if (!s)
    {
        return 0;
    }
    if (pthread_mutex_init(&s->mutex, 0))
    {
        free(s);
        return 0;
    }
    if (pthread_condattr_init(&attributes))
    {
        pthread_mutex_destroy(&s->mutex);
        free(s);
        return 0;
    }
    status = pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC);
    if (!status)
    {
        status = pthread_cond_init(&s->condition, &attributes);
    }
    pthread_condattr_destroy(&attributes);
    if (status)
    {
        pthread_mutex_destroy(&s->mutex);
        free(s);
        return 0;
    }
    return s;
}


/* Destroy port sync. */
void fvm_port_sync_destroy(FVM_PortSync *s)
{

    /* Cleanup / Освобождение ресурсов. */
    pthread_cond_destroy(&s->condition);
    pthread_mutex_destroy(&s->mutex);
    free(s);
}


/* Acquire port sync. */
void fvm_port_sync_lock(FVM_PortSync *s)
{
    pthread_mutex_lock(&s->mutex);
}


/* EN: Try borrowed sync once. Return one owning its mutex, zero without
 * ownership on contention or port error; never wait or touch the condition.
 * RU: Однократно захватить sync. Один означает владение мьютексом, ноль —
 * конфликт либо ошибку порта; ожидания и обращения к condition нет. */
int fvm_port_sync_try_lock(FVM_PortSync *s)
{
    return pthread_mutex_trylock(&s->mutex) == 0;
}


/* Release port sync. */
void fvm_port_sync_unlock(FVM_PortSync *s)
{
    pthread_mutex_unlock(&s->mutex);
}


/* Wait for port sync. */
void fvm_port_sync_wait(FVM_PortSync *s)
{
    pthread_cond_wait(&s->condition, &s->mutex);
}


/* Wait with the mutex held; the platform returns it on wake or timeout. */
int fvm_port_sync_wait_ms(FVM_PortSync *s, unsigned long milliseconds)
{
    struct timespec deadline;
    int status;

    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return 0;
    }
    deadline.tv_sec += milliseconds / 1000;
    deadline.tv_nsec += (milliseconds % 1000) * 1000000;
    if (deadline.tv_nsec >= 1000000000)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000;
    }
    status = pthread_cond_timedwait(&s->condition, &s->mutex, &deadline);
    return !status || status == ETIMEDOUT;
}


/* Notify port sync all. */
void fvm_port_sync_signal_all(FVM_PortSync *s)
{
    pthread_cond_broadcast(&s->condition);
}

static pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER;


/* Acquire port registry. */
void fvm_port_registry_lock(void)
{
    pthread_mutex_lock(&registry_mutex);
}


/* Release port registry. */
void fvm_port_registry_unlock(void)
{
    pthread_mutex_unlock(&registry_mutex);
}
