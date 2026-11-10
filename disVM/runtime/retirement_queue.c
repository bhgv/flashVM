/* Bounded native metadata, with no managed callbacks inside exclusion. */
#include "retirement_queue.h"
#include "channel_sync.h"
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>


/* One slot is either free, producer-reserved or consumer-ready. */
typedef struct RetirementSlot
{
    void *value; /* Identity only; no referent access. */
    unsigned long bytes; /* Reserved direct extent. */
    unsigned long generation; /* Token generation for reserved state only. */
    unsigned int next; /* Free-list link or hash chain link, depending on state. */
    unsigned int ready_next; /* FIFO successor slot plus one, or zero. */
    DVM_HeapKey key; /* Only populated in indexed ready slots. */
    int state; /* 0 free,1 reserved,2 ready. */
} RetirementSlot;


/* Stable native owner; all mutable metadata protected by sync. */
struct DVM_RetirementQueue
{
    FVM_PortSync *sync; /* Owned metadata mutex. */
    RetirementSlot *slots; /* Owned native slot storage. */
    unsigned int *buckets; /* Optional chained index. Zero means no entry. */
    unsigned int capacity; /* Current slot capacity under sync. */
    unsigned int peak_capacity; /* Largest allocated slot count since creation. */
    unsigned int initial_capacity; /* Capacity restored after a burst drains. */
    unsigned int max_capacity; /* Maximum slot count, including native metadata budget. */
    unsigned int free_head; /* capacity means no free slot. */
    unsigned int head; /* Oldest ready slot plus one, or zero. */
    unsigned int tail; /* Newest ready slot plus one, or zero. */
    unsigned int ready; /* Ready entries, excluding reservations. */
    unsigned int reserved; /* Producer reservations, excluding ready entries. */
    unsigned long bytes; /* Sum across all nonfree slots. */
    unsigned long byte_limit; /* Immutable direct extent budget. */
    unsigned long generation; /* Increment only, never wrap. */
    int accepting; /* New reservations permitted. */
};


/* Private initialization only; failure cleans native resources with no users. */
static DVM_RetirementQueue *create_queue(unsigned int capacity, unsigned long byte_limit, int indexed)
{
    DVM_RetirementQueue *queue;
    unsigned int index;
    if (!capacity || capacity > UINT_MAX/2 || capacity > SIZE_MAX/sizeof(RetirementSlot) ||
        capacity > SIZE_MAX/sizeof(unsigned int))
    {
        return NULL;
    }
    queue = calloc(1, sizeof(*queue));
    if (!queue)
    {
        return NULL;
    }
    queue->slots = calloc(capacity, sizeof(*queue->slots));
    queue->buckets = indexed ? calloc(capacity, sizeof(*queue->buckets)) : NULL;
    queue->sync = fvm_port_sync_create();
    if (!queue->slots || !queue->sync || (indexed && !queue->buckets))
    {
        if (queue->sync)
        {
            fvm_port_sync_destroy(queue->sync);
        }
        free(queue->slots);
        free(queue->buckets);
        free(queue);
        return NULL;
    }
    queue->capacity = capacity;
    queue->peak_capacity = capacity;
    queue->initial_capacity = capacity;
    queue->max_capacity = capacity;
    queue->byte_limit = byte_limit;
    queue->accepting = 1;
    for (index = 0; index < capacity; index++)
    {
        queue->slots[index].next = index+1;
    }
    return queue;
}


DVM_RetirementQueue *dvm_retirement_create(unsigned int capacity, unsigned long byte_limit)
{
    return create_queue(capacity, byte_limit, 0);
}


DVM_RetirementQueue *dvm_retirement_create_indexed(unsigned int capacity, unsigned long byte_limit)
{
    return create_queue(capacity, byte_limit, 1);
}


/* EN: Create a FIFO with no policy limit on pending release bytes.
 * RU: Создать FIFO без заданного лимита байтов ожидающих освобождений. */
DVM_RetirementQueue *dvm_retirement_create_growing(unsigned int capacity)
{
    DVM_RetirementQueue *queue = create_queue(capacity, ULONG_MAX, 0);
    if (queue)
    {
        size_t maximum = SIZE_MAX / sizeof(RetirementSlot);
        queue->max_capacity = maximum < UINT_MAX / 2 ? maximum : UINT_MAX / 2;
    }
    return queue;
}


/* Grow an indexed queue up to a metadata budget equal to byte_limit.
 * The initial allocation and ownership contract match create_indexed. */
DVM_RetirementQueue *dvm_retirement_create_growing_indexed(unsigned int capacity,
    unsigned long byte_limit)
{
    DVM_RetirementQueue *queue = create_queue(capacity, byte_limit, 1);
    if (queue)
    {
        unsigned long maximum = byte_limit / (sizeof(RetirementSlot) + sizeof(unsigned int));
        if (maximum > SIZE_MAX / sizeof(RetirementSlot))
        {
            maximum = SIZE_MAX / sizeof(RetirementSlot);
        }
        if (maximum > UINT_MAX / 2)
        {
            maximum = UINT_MAX / 2;
        }
        if (maximum > capacity)
        {
            queue->max_capacity = maximum;
        }
    }
    return queue;
}


/* Return the bucket for a key in the locked indexed queue. */
static unsigned int *key_bucket(DVM_RetirementQueue *queue, DVM_HeapKey key)
{
    uint64_t hash = key.generation * UINT64_C(11400714819323198485);
    hash ^= (uint64_t)(key.address / sizeof(void *));
    return &queue->buckets[(hash ^ (hash >> 32)) % queue->capacity];
}


/* Return the link that names this key, or the terminal zero link. */
static unsigned int *key_link(DVM_RetirementQueue *queue, DVM_HeapKey key)
{
    unsigned int *link = key_bucket(queue, key);
    while (*link)
    {
        RetirementSlot *slot = &queue->slots[*link - 1];
        if (slot->key.address == key.address && slot->key.generation == key.generation)
        {
            break;
        }
        link = &slot->next;
    }
    return link;
}


/* Enter and return with the metadata mutex held. Allocate and release native
 * storage outside it, then recheck that the requested resize still applies.
 * Existing token indices and committed FIFO order survive growth. */
static int resize_queue(DVM_RetirementQueue *queue, unsigned int capacity)
{
    unsigned int old_capacity = queue->capacity;
    int growing = capacity > old_capacity;
    int indexed = queue->buckets != NULL;
    fvm_port_sync_unlock(queue->sync);
    RetirementSlot *slots = calloc(capacity, sizeof(*slots));
    unsigned int *buckets = indexed ? calloc(capacity, sizeof(*buckets)) : NULL;
    int allocated = slots && (!indexed || buckets);
    fvm_port_sync_lock(queue->sync);
    if (allocated && queue->accepting && queue->capacity == old_capacity &&
        (growing ? queue->free_head == old_capacity : !queue->ready && !queue->reserved))
    {
        unsigned int used_capacity = growing ? old_capacity : 0;
        memcpy(slots, queue->slots, used_capacity * sizeof(*slots));
        for (unsigned int index = used_capacity; index < capacity; index++)
        {
            slots[index].next = index + 1;
        }
        RetirementSlot *old_slots = queue->slots;
        unsigned int *old_buckets = queue->buckets;
        queue->slots = slots;
        queue->buckets = buckets;
        queue->capacity = capacity;
        if (capacity > queue->peak_capacity)
        {
            queue->peak_capacity = capacity;
        }
        queue->free_head = used_capacity;
        for (unsigned int link = indexed ? queue->head : 0; link; link = slots[link - 1].ready_next)
        {
            RetirementSlot *slot = &slots[link - 1];
            unsigned int *bucket = key_bucket(queue, slot->key);
            slot->next = *bucket;
            *bucket = link;
        }
        slots = old_slots;
        buckets = old_buckets;
    }
    fvm_port_sync_unlock(queue->sync);
    free(slots);
    free(buckets);
    fvm_port_sync_lock(queue->sync);
    return allocated;
}


/* Double a full queue within its metadata ceiling, under the queue mutex. */
static int grow_full_queue(DVM_RetirementQueue *queue)
{
    if (queue->capacity == queue->max_capacity)
    {
        return 0;
    }
    unsigned int capacity = queue->capacity * 2;
    if (capacity > queue->max_capacity)
    {
        capacity = queue->max_capacity;
    }
    return resize_queue(queue, capacity);
}


/* Release burst storage after the final take or cancellation. The mutex
 * excludes publication, and no live reservation can refer to old slots. */
static void shrink_empty_queue(DVM_RetirementQueue *queue)
{
    if (queue->accepting && !queue->ready && !queue->reserved &&
        queue->capacity > queue->initial_capacity)
    {
        (void)resize_queue(queue, queue->initial_capacity);
    }
}


/* Validate a caller-private token while metadata is locked. No pointer reads
 * beyond token/queue storage; referent lifetime is entirely caller-owned. */
static RetirementSlot *reserved_slot(DVM_RetirementQueue *queue,
    const DVM_RetirementReservation *token)
{
    RetirementSlot *slot;
    if (!token || token->queue != queue || token->slot >= queue->capacity || !token->generation)
    {
        return NULL;
    }
    slot = &queue->slots[token->slot];
    return slot->state == 1 && slot->generation == token->generation ? slot : NULL;
}


/* Reserve slot and bytes atomically; failed admission changes neither token nor queue. */
DVM_RetirementResult dvm_retirement_reserve(DVM_RetirementQueue *queue,
    unsigned long bytes, DVM_RetirementReservation *token)
{
    DVM_RetirementResult result = DVM_RETIRE_OK;
    RetirementSlot *slot;
    if (!queue || !token || token->queue || token->generation)
    {
        return DVM_RETIRE_INVALID;
    }
    fvm_port_sync_lock(queue->sync);
    for (;;)
    {
        if (!queue->accepting)
        {
            result = DVM_RETIRE_CLOSED;
        }
        else if (queue->free_head == queue->capacity && queue->capacity == queue->max_capacity)
        {
            result = DVM_RETIRE_FULL;
        }
        else if (bytes > queue->byte_limit-queue->bytes)
        {
            result = DVM_RETIRE_BYTES;
        }
        else if (queue->generation == ULONG_MAX)
        {
            result = DVM_RETIRE_EXHAUSTED;
        }
        else if (queue->free_head == queue->capacity)
        {
            if (grow_full_queue(queue))
            {
                continue;
            }
            result = DVM_RETIRE_FULL;
        }
        else
        {
            token->slot = queue->free_head;
            slot = &queue->slots[token->slot];
            queue->free_head = slot->next;
            slot->state = 1;
            slot->bytes = bytes;
            slot->generation = ++queue->generation;
            queue->reserved++;
            queue->bytes += bytes;
            token->queue = queue;
            token->generation = slot->generation;
        }
        break;
    }
    fvm_port_sync_unlock(queue->sync);
    return result;
}


/* Caller consumes reservation only after publication succeeds; ready FIFO is
 * commit order, not reservation order. No referent ownership operation occurs. */
int dvm_retirement_commit_key(DVM_RetirementQueue *queue,
    DVM_RetirementReservation *token, void *value, DVM_HeapKey key, int *became_ready)
{
    RetirementSlot *slot;
    if (!queue || !value)
    {
        return 0;
    }
    fvm_port_sync_lock(queue->sync);
    if ((queue->buckets && (!key.generation || !key.address)) ||
        (!queue->buckets && key.generation))
    {
        fvm_port_sync_unlock(queue->sync);
        return 0;
    }
    slot = reserved_slot(queue, token);
    unsigned int *link = slot && queue->buckets ? key_link(queue, key) : NULL;
    if (link && *link)
    {
        slot = NULL;
    }
    if (slot)
    {
        if (link)
        {
            slot->key = key;
            slot->next = 0;
            *link = token->slot + 1;
        }
        slot->value = value;
        slot->state = 2;
        unsigned int ready_link = token->slot + 1;
        if (queue->tail)
        {
            queue->slots[queue->tail - 1].ready_next = ready_link;
        }
        else
        {
            queue->head = ready_link;
        }
        queue->tail = ready_link;
        queue->reserved--;
        if (became_ready)
        {
            *became_ready = queue->ready == 0;
        }
        queue->ready++;
        token->queue = NULL;
        token->generation = 0;
        token->slot = 0;
    }
    int success = slot != NULL;
    fvm_port_sync_unlock(queue->sync);
    return success;
}


int dvm_retirement_commit_ready(DVM_RetirementQueue *queue,
    DVM_RetirementReservation *token, void *value, int *became_ready)
{
    return dvm_retirement_commit_key(queue, token, value, (DVM_HeapKey){0}, became_ready);
}


/* Preserve the ordinary commit API for producers without a wake callback. */
int dvm_retirement_commit(DVM_RetirementQueue *queue,
    DVM_RetirementReservation *token, void *value)
{
    return dvm_retirement_commit_ready(queue, token, value, NULL);
}


/* Release a reserved slot under lock; clear all identities before reuse. */
int dvm_retirement_cancel(DVM_RetirementQueue *queue, DVM_RetirementReservation *token)
{
    RetirementSlot *slot;
    if (!queue)
    {
        return 0;
    }
    fvm_port_sync_lock(queue->sync);
    slot = reserved_slot(queue, token);
    if (slot)
    {
        queue->bytes -= slot->bytes;
        queue->reserved--;
        slot->bytes = 0;
        slot->generation = 0;
        slot->state = 0;
        slot->next = queue->free_head;
        queue->free_head = token->slot;
        token->queue = NULL;
        token->generation = 0;
        token->slot = 0;
    }
    int success = slot != NULL;
    if (success)
    {
        shrink_empty_queue(queue);
    }
    fvm_port_sync_unlock(queue->sync);
    return success;
}


/* Read the head identity for the single consumer. Producers only append. */
void *dvm_retirement_peek(DVM_RetirementQueue *queue)
{
    if (!queue)
    {
        return NULL;
    }
    fvm_port_sync_lock(queue->sync);
    void *value = queue->head ? queue->slots[queue->head - 1].value : NULL;
    fvm_port_sync_unlock(queue->sync);
    return value;
}


int dvm_retirement_contains_key(DVM_RetirementQueue *queue, DVM_HeapKey key)
{
    if (!queue || !key.generation)
    {
        return 0;
    }
    fvm_port_sync_lock(queue->sync);
    int found = queue->buckets && *key_link(queue, key) != 0;
    fvm_port_sync_unlock(queue->sync);
    return found;
}


/* Rotate ready metadata without borrowing capacity or withdrawing roots. */
void dvm_retirement_rotate(DVM_RetirementQueue *queue)
{
    fvm_port_sync_lock(queue->sync);
    if (queue->ready > 1)
    {
        unsigned int old_head = queue->head;
        RetirementSlot *slot = &queue->slots[old_head - 1];
        queue->head = slot->ready_next;
        slot->ready_next = 0;
        queue->slots[queue->tail - 1].ready_next = old_head;
        queue->tail = old_head;
    }
    fvm_port_sync_unlock(queue->sync);
}


/* Detach before caller runs any root removal/destructor. Empty output unchanged. */
int dvm_retirement_take(DVM_RetirementQueue *queue, void **value, unsigned long *bytes)
{
    unsigned int index;
    RetirementSlot *slot;
    int result = 0;
    if (!queue || !value || !bytes)
    {
        return 0;
    }
    fvm_port_sync_lock(queue->sync);
    if (queue->ready)
    {
        index = queue->head - 1;
        slot = &queue->slots[index];
        if (queue->buckets)
        {
            unsigned int *link = key_link(queue, slot->key);
            *link = slot->next;
            slot->key = (DVM_HeapKey){0};
        }
        *value = slot->value;
        *bytes = slot->bytes;
        queue->head = slot->ready_next;
        slot->ready_next = 0;
        if (!queue->head)
        {
            queue->tail = 0;
        }
        queue->ready--;
        queue->bytes -= slot->bytes;
        slot->value = NULL;
        slot->bytes = 0;
        slot->generation = 0;
        slot->state = 0;
        slot->next = queue->free_head;
        queue->free_head = index;
        shrink_empty_queue(queue);
        result = 1;
    }
    fvm_port_sync_unlock(queue->sync);
    return result;
}


/* Stop new reservations; accepted tokens keep their explicit commit/cancel duty. */
void dvm_retirement_close(DVM_RetirementQueue *queue)
{
    if (queue)
    {
        fvm_port_sync_lock(queue->sync);
        queue->accepting = 0;
        fvm_port_sync_unlock(queue->sync);
    }
}


/* Copy coherent metadata only; no source/strong-owner snapshot is implied. */
int dvm_retirement_statistics(DVM_RetirementQueue *queue, DVM_RetirementStats *stats)
{
    if (!queue || !stats)
    {
        return 0;
    }
    fvm_port_sync_lock(queue->sync);
    stats->ready = queue->ready;
    stats->reserved = queue->reserved;
    stats->bytes = queue->bytes;
    stats->accepting = queue->accepting;
    stats->capacity = queue->capacity;
    stats->peak_capacity = queue->peak_capacity;
    fvm_port_sync_unlock(queue->sync);
    return 1;
}


/* Quiescent lifetime operation; native resources free only after unlock. */
int dvm_retirement_destroy(DVM_RetirementQueue **owner)
{
    DVM_RetirementQueue *queue;
    if (!owner || !*owner)
    {
        return 0;
    }
    queue = *owner;
    fvm_port_sync_lock(queue->sync);
    if (queue->ready || queue->reserved || queue->bytes)
    {
        fvm_port_sync_unlock(queue->sync);
        return 0;
    }
    *owner = NULL;
    fvm_port_sync_unlock(queue->sync);
    fvm_port_sync_destroy(queue->sync);
    free(queue->slots);
    free(queue->buckets);
    free(queue);
    return 1;
}
