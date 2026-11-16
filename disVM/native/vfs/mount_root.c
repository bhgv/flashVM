/* Copyright (c) 2026 bhgv
 * EN: Detached native fid retirement keeps captured provider destruction free of RPC waits.
 * RU: Detached native fid retirement исключает RPC wait из destruction captured provider.
 */
#include "mount_internal.h"
#include <pthread.h>
#include <stdlib.h>


/* EN/RU: Worker holds native fid only, no provider/client callback context or guest pointers.
 * Worker удерживает только native fid, без provider/callback context и guest pointers. */
struct StyxRootLife
{
    pthread_mutex_t lock; /* Protect one-shot retirement request. */
    pthread_cond_t ready; /* Native retirement wakeup, never a guest wait. */
    int stopping; /* Control consumed; worker may retire root. */
    StyxFid *root; /* Owned root plus its independent client pin. */
};


/* EN/RU: Wait for final factory retirement, then consume fid/client pin outside all locks.
 * Ждать final factory retirement, затем поглотить fid/client pin вне locks. */
static void *retire(void *opaque)
{
    StyxRootLife *life = opaque;
    pthread_mutex_lock(&life->lock);
    while (!life->stopping)
    {
        pthread_cond_wait(&life->ready, &life->lock);
    }
    pthread_mutex_unlock(&life->lock);
    styxclientclunk(life->root);
    pthread_cond_destroy(&life->ready);
    pthread_mutex_destroy(&life->lock);
    free(life);
    return NULL;
}


/* EN/RU: See mount_internal.h; all fallible native setup precedes ownership transfer.
 * См. mount_internal.h; все fallible native setup шаги до ownership transfer. */
StyxRootLife *styxmountrootlife(StyxFid *root)
{
    if (!root)
    {
        return NULL;
    }
    StyxRootLife *life = calloc(1, sizeof(*life));
    if (!life)
    {
        return NULL;
    }
    if (pthread_mutex_init(&life->lock, NULL))
    {
        free(life);
        return NULL;
    }
    if (pthread_cond_init(&life->ready, NULL))
    {
        pthread_mutex_destroy(&life->lock);
        free(life);
        return NULL;
    }
    life->root = root;
    pthread_attr_t attr;
    pthread_t worker;
    int initialized = !pthread_attr_init(&attr);
    int valid = initialized && !pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (!valid || pthread_create(&worker, &attr, retire, life))
    {
        if (initialized)
        {
            pthread_attr_destroy(&attr);
        }
        pthread_cond_destroy(&life->ready);
        pthread_mutex_destroy(&life->lock);
        free(life);
        return NULL;
    }
    pthread_attr_destroy(&attr);
    return life;
}


/* EN/RU: See mount_internal.h; after unlock caller must never access the consumed life context.
 * См. mount_internal.h; после unlock caller больше не обращается к consumed life context. */
void styxmountrootretire(StyxRootLife *life)
{
    if (life)
    {
        pthread_mutex_lock(&life->lock);
        life->stopping = 1;
        pthread_cond_signal(&life->ready);
        pthread_mutex_unlock(&life->lock);
    }
}
