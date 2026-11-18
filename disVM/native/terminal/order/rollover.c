/* Copyright (c) 2026 bhgv
 * EN: Native selected-counter rollover proposals and exact ACK/NACK, no generation field in data packets.
 * RU: Предложения сброса выбранного счётчика и точные ACK/NACK; без поколения в пакетах данных.
 */
#include "internal.h"


/* EN/RU: See internal.h; ready RX old prefix must be consumed with no accepted future data left behind.
 * См. internal.h; old RX prefix должен быть consumed без оставшихся future data.
 */
int orderready(OrderTag *tag, unsigned direction, unsigned long long operation)
{
    return tag && tag->pending[direction] && tag->operation[direction] == operation &&
        (direction == STYX_ORDER_TX || (tag->rx_consumed && tag->consumed == tag->boundary[direction] &&
            !tag->ready_count && !tag->held_count));
}


/* EN/RU: Check proposed last old RX number without accepting a boundary behind already admitted data.
 * Проверить last old RX number, не принимать boundary позади уже admitted data.
 */
static int boundary(OrderTag *tag, unsigned long long last)
{
    if ((tag->rx_consumed && tag->consumed > last) || (tag->tail && tag->tail->ordinal > last))
    {
        return 0;
    }
    for (OrderRecord *record = tag->held; record; record = record->next)
    {
        if (record->ordinal > last)
        {
            return 0;
        }
    }
    return 1;
}


/* EN/RU: See order.h; proposals are per selected tag/direction, no unrelated counter is suspended.
 * См. order.h; proposal лишь selected tag/direction, unrelated counters не приостанавливаются.
 */
int styxorderprepare(StyxOrder *order, unsigned long tag, unsigned direction,
    unsigned long long operation, unsigned long long last)
{
    if (!order || tag > FVM_SRECORD_U32 || direction > STYX_ORDER_TX || !operation)
    {
        return 0;
    }
    pthread_mutex_lock(&order->lock);
    OrderTag *entry = ordertag(order, tag, 0); /* Missing/unused counters are not silently reset. */
    int valid = !order->closed && entry && entry->used[direction] && !entry->pending[direction] && operation > entry->last[direction];
    if (valid)
    {
        valid = direction == STYX_ORDER_TX ? entry->tx_spooled && entry->tx_last == last : boundary(entry, last);
    }
    if (valid)
    {
        entry->pending[direction] = 1;
        entry->operation[direction] = operation;
        entry->last[direction] = operation;
        entry->boundary[direction] = last;
        pthread_cond_broadcast(&order->changed);
    }
    pthread_mutex_unlock(&order->lock);
    return valid;
}


/* EN/RU: See order.h; query does not consume data or trigger local/remote protocol work.
 * См. order.h; query не потребляет data и не запускает protocol work.
 */
int styxorderready(StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long operation)
{
    if (!order || tag > FVM_SRECORD_U32 || direction > STYX_ORDER_TX || !operation)
    {
        return 0;
    }
    pthread_mutex_lock(&order->lock);
    int ready = !order->closed && orderready(ordertag(order, tag, 0), direction, operation);
    pthread_mutex_unlock(&order->lock);
    return ready;
}


/* EN/RU: See order.h; ACK never resets a not-ready RX prefix or changes another counter's state.
 * См. order.h; ACK не сбрасывает not-ready RX prefix и не меняет state другого counter.
 */
int styxorderconfirm(StyxOrder *order, unsigned long tag, unsigned direction,
    unsigned long long operation, int accepted)
{
    if (!order || tag > FVM_SRECORD_U32 || direction > STYX_ORDER_TX || !operation || (accepted != 0 && accepted != 1))
    {
        return 0;
    }
    pthread_mutex_lock(&order->lock);
    OrderTag *entry = ordertag(order, tag, 0); /* Borrowed selected directional operation history. */
    int valid = !order->closed && entry;
    if (valid && entry->done[direction] == operation)
    {
        valid = entry->accepted[direction] == accepted;
    }
    else
    {
        valid = valid && entry->pending[direction] && entry->operation[direction] == operation &&
            (!accepted || orderready(entry, direction, operation));
        if (valid)
        {
            if (accepted)
            {
                entry->exhausted[direction] = 0;
                if (direction == STYX_ORDER_TX)
                {
                    entry->transmit = 0;
                    entry->tx_spooled = 0;
                }
                else
                {
                    entry->expected = 0;
                    entry->rx_consumed = 0;
                }
            }
            entry->pending[direction] = 0;
            entry->done[direction] = operation;
            entry->accepted[direction] = accepted;
            pthread_cond_broadcast(&order->changed);
        }
    }
    pthread_mutex_unlock(&order->lock);
    return valid;
}
