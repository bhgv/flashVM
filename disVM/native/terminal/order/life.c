/* Copyright (c) 2026 bhgv
 * EN: Native per-namespace engine lifetime and explicit first-counter agreement, no owned threads.
 * RU: Жизненный цикл очередей одного пространства имён и явное начальное значение счётчика; без собственных потоков.
 */
#include "internal.h"


/* EN/RU: See internal.h; caller holds registry lock, allocation failure changes no existing tag.
 * См. internal.h; registry lock у caller, allocation failure не меняет existing tag.
 */
OrderTag *ordertag(StyxOrder *order, unsigned long tag, int create)
{
    OrderTag *entry = order->tags; /* Borrowed registry cursor. */
    while (entry && entry->tag != tag)
    {
        entry = entry->next;
    }
    if (!entry && create)
    {
        entry = calloc(1, sizeof(*entry));
        if (entry)
        {
            entry->tag = tag;
            entry->next = order->tags;
            order->tags = entry;
        }
    }
    return entry;
}


/* EN/RU: See order.h; publish only fully initialized native synchronization and bounds.
 * См. order.h; публиковать лишь полностью initialized native synchronization/bounds.
 */
StyxOrder *styxordercreate(unsigned limit, unsigned long long window)
{
    if (!limit || limit > FVM_SRECORD_BODY_MAX || !window || window >= (1ULL << 63))
    {
        return NULL;
    }
    StyxOrder *order = calloc(1, sizeof(*order)); /* Private until every native resource initialized. */
    if (!order)
    {
        return NULL;
    }
    if (pthread_mutex_init(&order->lock, NULL))
    {
        free(order);
        return NULL;
    }
    pthread_condattr_t attributes; /* Temporary native monotonic wait configuration. */
    int initialized = !pthread_condattr_init(&attributes); /* Attribute disposal guard. */
    int ready = initialized && !pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) &&
        !pthread_cond_init(&order->changed, &attributes);
    if (initialized)
    {
        pthread_condattr_destroy(&attributes);
    }
    if (!ready)
    {
        pthread_mutex_destroy(&order->lock);
        free(order);
        return NULL;
    }
    order->limit = limit;
    order->window = window;
    return order;
}


/* EN/RU: See order.h; default directions stay independent when the other direction was already seeded.
 * См. order.h; directions независимы, даже если другой direction уже seeded.
 */
int styxorderstart(StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long first)
{
    if (!order || tag > FVM_SRECORD_U32 || direction > STYX_ORDER_TX)
    {
        return 0;
    }
    pthread_mutex_lock(&order->lock);
    OrderTag *entry = order->closed ? NULL : ordertag(order, tag, 1); /* Owned registry entry or refusal. */
    int valid = entry && !entry->used[direction] && !entry->pending[direction];
    if (valid)
    {
        entry->used[direction] = 1;
        if (direction == STYX_ORDER_RX)
        {
            entry->expected = first;
        }
        else
        {
            entry->transmit = first;
        }
    }
    pthread_mutex_unlock(&order->lock);
    return valid;
}


/* EN/RU: See order.h; close is idempotent, retained ready packets remain independently drainable.
 * См. order.h; close idempotent, retained ready packets дочитываются независимо.
 */
void styxorderclose(StyxOrder *order)
{
    if (order)
    {
        pthread_mutex_lock(&order->lock);
        order->closed = 1;
        pthread_cond_broadcast(&order->changed);
        pthread_mutex_unlock(&order->lock);
    }
}


/* EN/RU: Consume an owned record chain without callbacks or external/native object access.
 * Поглотить owned record chain без callbacks либо external object access.
 */
static void clear(OrderRecord *record)
{
    while (record)
    {
        OrderRecord *next = record->next; /* Owned continuation transferred before free. */
        free(record);
        record = next;
    }
}


/* EN/RU: See order.h; all users retire before final queue/registry disposal, no workers to join.
 * См. order.h; все users завершаются до queue/registry disposal, workers нет.
 */
void styxorderfree(StyxOrder *order)
{
    if (order)
    {
        styxorderclose(order);
        while (order->tags)
        {
            OrderTag *entry = order->tags; /* Transfer owned registry node. */
            order->tags = entry->next;
            clear(entry->head);
            clear(entry->held);
            free(entry);
        }
        pthread_cond_destroy(&order->changed);
        pthread_mutex_destroy(&order->lock);
        free(order);
    }
}
