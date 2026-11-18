/* Copyright (c) 2026 bhgv
 * EN: Checked complete packets reorder independently per tag, duplicates never become repeated client messages.
 * RU: Проверенные пакеты упорядочиваются независимо по тегам; дубликаты не выдаются клиентам повторно.
 */
#include "internal.h"


/* EN/RU: Admit one contiguous copied node under engine lock, never wrap implicitly at MAX.
 * Принять contiguous copied node под lock, никогда не переходить через MAX автоматически.
 */
static void ready(OrderTag *tag, OrderRecord *record)
{
    record->next = NULL;
    if (tag->tail)
    {
        tag->tail->next = record;
    }
    else
    {
        tag->head = record;
    }
    tag->tail = record;
    tag->ready_count++;
    if (tag->expected == ~0ULL)
    {
        tag->exhausted[STYX_ORDER_RX] = 1;
    }
    else
    {
        tag->expected++;
    }
}


/* EN/RU: See order.h; CRC/framing errors refuse packet before registry or queue mutation.
 * См. order.h; CRC/framing errors отвергают packet до registry/queue mutation.
 */
int styxorderfeed(StyxOrder *order, const void *packet, size_t size)
{
    FVM_SOrderedRecord view; /* Private checked borrowed packet view. */
    size_t used; /* Exact one-packet extent. */
    if (!order || fvm_sordered_read(packet, size, order->limit, &view, &used) != FVM_S_OK || used != size)
    {
        return STYX_ORDER_REFUSED;
    }
    unsigned long long number = ((unsigned long long)view.ordinal.high << 32) | view.ordinal.low; /* Portable codec limbs. */
    pthread_mutex_lock(&order->lock);
    OrderTag *tag = order->closed ? NULL : ordertag(order, view.record.tag, 1); /* Independent tag registry entry. */
    int result = order->closed ? STYX_ORDER_REFUSED : !tag ? STYX_ORDER_NOMEM : STYX_ORDER_ACCEPTED;
    if (result == STYX_ORDER_ACCEPTED && (tag->exhausted[STYX_ORDER_RX] || number < tag->expected))
    {
        result = STYX_ORDER_DUPLICATE;
    }
    if (result == STYX_ORDER_ACCEPTED && tag->pending[STYX_ORDER_RX] && number > tag->boundary[STYX_ORDER_RX])
    {
        result = STYX_ORDER_REFUSED;
    }
    if (result == STYX_ORDER_ACCEPTED && number - tag->expected > order->window)
    {
        result = STYX_ORDER_WINDOW;
    }
    OrderRecord **at = tag ? &tag->held : NULL; /* Protected insertion point in this tag's future records. */
    if (result == STYX_ORDER_ACCEPTED)
    {
        while (*at && (*at)->ordinal < number)
        {
            at = &(*at)->next;
        }
        if (*at && (*at)->ordinal == number)
        {
            result = (*at)->size == view.record.size && (!view.record.size ||
                !memcmp((*at)->bytes, view.record.payload, view.record.size)) ? STYX_ORDER_DUPLICATE : STYX_ORDER_CONFLICT;
        }
    }
    if (result == STYX_ORDER_ACCEPTED && (view.record.size > (size_t)-1 - sizeof(OrderRecord) ||
        tag->ready_count == (size_t)-1 || tag->held_count >= (size_t)-1 - tag->ready_count))
    {
        result = STYX_ORDER_NOMEM;
    }
    if (result == STYX_ORDER_ACCEPTED)
    {
        OrderRecord *record = malloc(sizeof(*record) + view.record.size); /* Private exact payload copy. */
        if (!record)
        {
            result = STYX_ORDER_NOMEM;
        }
        else
        {
            record->ordinal = number;
            record->size = view.record.size;
            if (record->size)
            {
                memcpy(record->bytes, view.record.payload, record->size);
            }
            tag->used[STYX_ORDER_RX] = 1;
            if (number == tag->expected)
            {
                ready(tag, record);
                while (!tag->exhausted[STYX_ORDER_RX] && tag->held && tag->held->ordinal == tag->expected)
                {
                    OrderRecord *next = tag->held; /* Transfer one contiguous future record. */
                    tag->held = next->next;
                    tag->held_count--;
                    ready(tag, next);
                }
            }
            else
            {
                record->next = *at;
                *at = record;
                tag->held_count++;
            }
            pthread_cond_broadcast(&order->changed);
        }
    }
    pthread_mutex_unlock(&order->lock);
    return result;
}
