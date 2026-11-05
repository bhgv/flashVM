/* FlashVM: Portable platform adapter interface or implementation: channel sync.
 * File: ports/mac/channel_sync.c.
 */

#include "../channel_sync.h"
#include <pthread.h>
#include <errno.h>
#include <time.h>
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
    if (pthread_cond_init(&s->condition, 0))
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
    struct timespec duration;
    int status;

    duration.tv_sec = milliseconds / 1000;
    duration.tv_nsec = (milliseconds % 1000) * 1000000;
    status = pthread_cond_timedwait_relative_np(&s->condition, &s->mutex, &duration);
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
