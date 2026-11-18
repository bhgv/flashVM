/* Copyright (c) 2026 bhgv
 * EN: Native directional profile readiness with one monotonic deadline, no record polling.
 * RU: Native-готовность направления с единым monotonic deadline, без опроса записей.
 */
#include "internal.h"


/* EN/RU: See broker.h; read only one direction, never derive TX permission from RX readiness.
 * См. broker.h; проверить лишь одно направление, не выводить TX-права из готовности RX.
 */
int styxorderedready(StyxOrdered *peer, unsigned direction)
{
    if (!peer || direction > STYX_ORDER_TX)
    {
        return 0;
    }
    pthread_mutex_lock(&peer->lock);
    int ready = !peer->reason && peer->active[direction]; /* Coherent local activation. */
    pthread_mutex_unlock(&peer->lock);
    return ready;
}


/* EN/RU: See broker.h; unrelated wakes do not restart timeout, and a timeout never releases a pending offer.
 * См. broker.h; посторонние пробуждения не продлевают timeout, истечение не снимает pending-предложение.
 */
int styxorderedwait(StyxOrdered *peer, unsigned direction, unsigned timeout)
{
    if (!peer || direction > STYX_ORDER_TX || timeout > 120000)
    {
        return 0;
    }
    struct timespec deadline; /* One original monotonic budget. */
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return 0;
    }
    deadline.tv_sec += timeout / 1000;
    deadline.tv_nsec += (long)(timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    pthread_mutex_lock(&peer->lock);
    while (!peer->reason && !peer->active[direction] && timeout)
    {
        if (pthread_cond_timedwait(&peer->changed, &peer->lock, &deadline))
        {
            break;
        }
    }
    int ready = !peer->reason && peer->active[direction]; /* Timeout leaves all negotiation state intact. */
    pthread_mutex_unlock(&peer->lock);
    return ready;
}



/* EN/RU: See broker.h; exact operation predicate avoids sleep-based assumptions about NACK arrival.
 * См. broker.h; точная проверка операции не требует предположений о времени прихода NACK.
 */
int styxorderedofferwait(StyxOrdered *peer, unsigned long long operation, unsigned timeout)
{
    if (!peer || !operation || timeout > 120000)
    {
        return 0;
    }
    struct timespec deadline; /* One original operation budget. */
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return 0;
    }
    deadline.tv_sec += timeout / 1000;
    deadline.tv_nsec += (long)(timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    pthread_mutex_lock(&peer->lock);
    while (!peer->reason && orderedvalue(peer->offer.operation) == operation && peer->tx_pending && timeout)
    {
        if (pthread_cond_timedwait(&peer->changed, &peer->lock, &deadline))
        {
            break;
        }
    }
    int ready = !peer->reason && orderedvalue(peer->offer.operation) == operation &&
        !peer->tx_pending && orderedvalue(peer->answer.operation) == operation; /* Resolution, not implicit acceptance. */
    pthread_mutex_unlock(&peer->lock);
    return ready;
}


/* EN/RU: See broker.h; wait only native root services, preserving ready notices after retirement.
 * См. broker.h; ждать лишь native root-службы, сохраняя готовые уведомления после завершения.
 */
int styxorderedservicewait(StyxOrdered *peer, unsigned timeout)
{
    if (!peer || timeout > 120000)
    {
        return 0;
    }
    struct timespec deadline; /* Original total monotonic budget. */
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return 0;
    }
    deadline.tv_sec += timeout / 1000;
    deadline.tv_nsec += (long)(timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    pthread_mutex_lock(&peer->lock);
    while (!peer->services && !peer->reason && timeout)
    {
        if (pthread_cond_timedwait(&peer->changed, &peer->lock, &deadline))
        {
            break;
        }
    }
    int ready = peer->services != NULL; /* Accepted native notices are drainable after stop. */
    pthread_mutex_unlock(&peer->lock);
    return ready;
}



/* EN/RU: See broker.h; accepted v1 profiles are immutable and remain inspectable after an orderly EOF.
 * См. broker.h; принятые v1-профили неизменяемы и доступны для наблюдения после корректного EOF.
 */
int styxorderedagreed(StyxOrdered *peer, unsigned direction)
{
    if (!peer || direction > STYX_ORDER_TX)
    {
        return 0;
    }
    pthread_mutex_lock(&peer->lock);
    int agreed = peer->active[direction]; /* Historical agreement, never current send authority. */
    pthread_mutex_unlock(&peer->lock);
    return agreed;
}
