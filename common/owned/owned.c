#include "owned.h"
#include <errno.h>
#include <limits.h>

/* Initialize borrowed guard; roll back metadata if condition allocation fails. */
int fvm_owned_init(FvmOwned *guard)
{
    int error;
    if (!guard)
    {
        return EINVAL;
    }
    error = pthread_mutex_init(&guard->metadata, NULL);
    if (error)
    {
        return error;
    }
    error = pthread_cond_init(&guard->changed, NULL);
    if (error)
    {
        pthread_mutex_destroy(&guard->metadata);
        return error;
    }
    guard->owner = 0;
    guard->depth = 0;
    guard->waiters = 0;
    return 0;
}

/* Park competitors on a condition; the same sequential operation may reenter.
 * Caller must disable native cancellation for the entire operation lifetime. */
int fvm_owned_enter(FvmOwned *guard, uint64_t token)
{
    int error;
    int cancellation;
    if (!guard || !token)
    {
        return EINVAL;
    }
    error = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancellation);
    if (error)
    {
        return error;
    }
    if (cancellation != PTHREAD_CANCEL_DISABLE)
    {
        pthread_setcancelstate(cancellation, NULL);
        return EINVAL;
    }
    error = pthread_mutex_lock(&guard->metadata);
    if (!error)
    {
        while (guard->owner && guard->owner != token)
        {
            if (guard->waiters == UINT_MAX)
            {
                error = EOVERFLOW;
                break;
            }
            guard->waiters++;
            error = pthread_cond_wait(&guard->changed, &guard->metadata);
            guard->waiters--;
            if (error)
            {
                break;
            }
        }
        if (!error)
        {
            if (guard->depth == UINT_MAX)
            {
                error = EOVERFLOW;
            }
            else
            {
                guard->owner = token;
                guard->depth++;
            }
        }
        pthread_mutex_unlock(&guard->metadata);
    }
    return error;
}

/* Attempt one recursive entry without parking a native thread on payload work.
 * Caller retains guard lifetime and cancellation discipline. Busy/overflow leave
 * every metadata field unchanged; successful entries require one normal leave. */
int fvm_owned_try_enter(FvmOwned *guard, uint64_t token)
{
    int error;
    int cancellation;
    if (!guard || !token)
    {
        return EINVAL;
    }
    error = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancellation);
    if (error)
    {
        return error;
    }
    if (cancellation != PTHREAD_CANCEL_DISABLE)
    {
        pthread_setcancelstate(cancellation, NULL);
        return EINVAL;
    }
    error = pthread_mutex_lock(&guard->metadata);
    if (error)
    {
        return error;
    }
    if (guard->owner && guard->owner != token)
    {
        error = EBUSY;
    }
    else if (guard->depth == UINT_MAX)
    {
        error = EOVERFLOW;
    }
    else
    {
        guard->owner = token;
        guard->depth++;
    }
    pthread_mutex_unlock(&guard->metadata);
    return error;
}

/* Release one owner level; notify all competitors only after final publication. */
int fvm_owned_leave(FvmOwned *guard, uint64_t token)
{
    int error;
    if (!guard)
    {
        return EINVAL;
    }
    error = pthread_mutex_lock(&guard->metadata);
    if (error)
    {
        return error;
    }
    if (!token || guard->owner != token)
    {
        error = EPERM;
    }
    else
    {
        guard->depth--;
        if (!guard->depth)
        {
            guard->owner = 0;
            error = pthread_cond_broadcast(&guard->changed);
        }
    }
    pthread_mutex_unlock(&guard->metadata);
    return error;
}

/* Copy borrowed diagnostic scalars under the short metadata mutex only. */
int fvm_owned_inspect(FvmOwned *guard, uint64_t *owner, unsigned *depth, unsigned *waiters)
{
    int error;
    if (!guard || !owner || !depth || !waiters)
    {
        return EINVAL;
    }
    error = pthread_mutex_lock(&guard->metadata);
    if (error)
    {
        return error;
    }
    *owner = guard->owner;
    *depth = guard->depth;
    *waiters = guard->waiters;
    return pthread_mutex_unlock(&guard->metadata);
}

/* Destroy only externally quiescent storage; failed busy checks preserve it. */
int fvm_owned_destroy(FvmOwned *guard)
{
    int error;
    if (!guard)
    {
        return EINVAL;
    }
    error = pthread_mutex_lock(&guard->metadata);
    if (error)
    {
        return error;
    }
    if (guard->owner || guard->waiters)
    {
        pthread_mutex_unlock(&guard->metadata);
        return EBUSY;
    }
    pthread_mutex_unlock(&guard->metadata);
    error = pthread_cond_destroy(&guard->changed);
    if (error)
    {
        return error;
    }
    return pthread_mutex_destroy(&guard->metadata);
}
