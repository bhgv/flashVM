/* Copyright (c) 2026 bhgv
 * EN: Independent native selected-tag waits and one-record copied receives advance consumed watermarks.
 * RU: Независимые native-ожидания по тегам и поштучное чтение с отметкой выданного номера.
 */
#include "internal.h"


/* EN/RU: See order.h; SMALL does not expose ordinal, consume payload or move the activation checkpoint.
 * См. order.h; SMALL не выдаёт ordinal, не потребляет payload и не двигает activation checkpoint.
 */
int styxordernext(StyxOrder *order, unsigned long tag, unsigned long long *ordinal,
    void *bytes, size_t capacity, size_t *size)
{
    if (!order || tag > FVM_SRECORD_U32 || !ordinal || !size || (!bytes && capacity))
    {
        return STYX_ORDER_END;
    }
    pthread_mutex_lock(&order->lock);
    OrderTag *entry = ordertag(order, tag, 0); /* Borrowed under native engine lock. */
    OrderRecord *record = entry ? entry->head : NULL; /* Only selected queue is examined. */
    int result = order->closed ? STYX_ORDER_END : STYX_ORDER_EMPTY;
    if (record)
    {
        *size = record->size;
        result = STYX_ORDER_SMALL;
        if (record->size <= capacity)
        {
            if (record->size)
            {
                memcpy(bytes, record->bytes, record->size);
            }
            *ordinal = record->ordinal;
            entry->consumed = record->ordinal;
            entry->rx_consumed = 1;
            entry->head = record->next;
            if (!entry->head)
            {
                entry->tail = NULL;
            }
            entry->ready_count--;
            free(record);
            result = STYX_ORDER_RECORD;
            pthread_cond_broadcast(&order->changed);
        }
    }
    pthread_mutex_unlock(&order->lock);
    return result;
}


/* EN/RU: See order.h; no allocation/polling or common FIFO head can block unrelated tag readiness.
 * См. order.h; без allocation/polling, общий FIFO head не блокирует чужую readiness.
 */
int styxorderwait(StyxOrder *order, unsigned long tag, unsigned timeout)
{
    if (!order || tag > FVM_SRECORD_U32 || timeout > 120000)
    {
        return 0;
    }
    struct timespec deadline; /* One original monotonic wait deadline. */
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
    pthread_mutex_lock(&order->lock);
    OrderTag *entry = ordertag(order, tag, 0); /* Rechecked after native wake, missing tag is allowed. */
    while ((!entry || !entry->ready_count) && !order->closed && timeout)
    {
        if (pthread_cond_timedwait(&order->changed, &order->lock, &deadline))
        {
            break;
        }
        entry = ordertag(order, tag, 0);
    }
    int ready = entry && entry->ready_count; /* Confirmed messages remain readable after owner retirement. */
    pthread_mutex_unlock(&order->lock);
    return ready;
}


/* EN/RU: See order.h; old boundary is satisfied only by actually copied ready records, never held packets.
 * См. order.h; old boundary достигается лишь реально copied ready records, не held packets.
 */
int styxorderreached(StyxOrder *order, unsigned long tag, unsigned long long boundary)
{
    if (!order || tag > FVM_SRECORD_U32)
    {
        return 0;
    }
    pthread_mutex_lock(&order->lock);
    OrderTag *entry = ordertag(order, tag, 0); /* Borrowed selected counter. */
    int reached = entry && entry->rx_consumed && entry->consumed >= boundary;
    pthread_mutex_unlock(&order->lock);
    return reached;
}
