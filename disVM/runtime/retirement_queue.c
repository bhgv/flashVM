/* Bounded native metadata, with no managed callbacks inside exclusion. */
#include "retirement_queue.h"
#include "channel_sync.h"
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>


/* One slot is either free, producer-reserved or consumer-ready. */
typedef struct RetirementSlot
{
    void *value; /* Identity only; no referent access. */
    unsigned long bytes; /* Reserved direct extent. */
    unsigned long generation; /* Token generation for reserved state only. */
    unsigned int next; /* Free-list link while free. */
    int state; /* 0 free,1 reserved,2 ready. */
} RetirementSlot;


/* Stable native owner; all mutable metadata protected by sync. */
struct DVM_RetirementQueue
{
    FVM_PortSync *sync; /* Owned metadata mutex. */
    RetirementSlot *slots; /* Owned fixed native slot storage. */
    unsigned int *ring; /* Commit-order FIFO of ready slot indexes. */
    unsigned int capacity; /* Immutable initialized slot capacity. */
    unsigned int free_head; /* capacity means no free slot. */
    unsigned int head; /* Oldest committed ring position. */
    unsigned int ready; /* Ready entries, excluding reservations. */
    unsigned int reserved; /* Producer reservations, excluding ready entries. */
    unsigned long bytes; /* Sum across all nonfree slots. */
    unsigned long byte_limit; /* Immutable direct extent budget. */
    unsigned long generation; /* Increment only, never wrap. */
    int accepting; /* New reservations permitted. */
};


/* Private initialization only; failure cleans native resources with no users. */
DVM_RetirementQueue *dvm_retirement_create(unsigned int capacity, unsigned long byte_limit)
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
    queue->ring = calloc(capacity, sizeof(*queue->ring));
    queue->sync = fvm_port_sync_create();
    if (!queue->slots || !queue->ring || !queue->sync)
    {
        if (queue->sync)
        {
            fvm_port_sync_destroy(queue->sync);
        }
        free(queue->slots);
        free(queue->ring);
        free(queue);
        return NULL;
    }
    queue->capacity = capacity;
    queue->byte_limit = byte_limit;
    queue->accepting = 1;
    for (index = 0; index < capacity; index++)
    {
        queue->slots[index].next = index+1;
    }
    return queue;
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
    if (!queue->accepting)
    {
        result = DVM_RETIRE_CLOSED;
    }
    else if (queue->free_head == queue->capacity)
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
    fvm_port_sync_unlock(queue->sync);
    return result;
}


/* Caller consumes reservation only after publication succeeds; ready FIFO is
 * commit order, not reservation order. No referent ownership operation occurs. */
int dvm_retirement_commit_ready(DVM_RetirementQueue *queue,
    DVM_RetirementReservation *token, void *value, int *became_ready)
{
    RetirementSlot *slot;
    if (!queue || !value)
    {
        return 0;
    }
    fvm_port_sync_lock(queue->sync);
    slot = reserved_slot(queue, token);
    if (slot)
    {
        slot->value = value;
        slot->state = 2;
        queue->ring[(queue->head+queue->ready)%queue->capacity] = token->slot;
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
    fvm_port_sync_unlock(queue->sync);
    return slot != NULL;
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
    fvm_port_sync_unlock(queue->sync);
    return slot != NULL;
}


/* Read the head identity for the single consumer. Producers only append. */
void *dvm_retirement_peek(DVM_RetirementQueue *queue)
{
    if (!queue)
    {
        return NULL;
    }
    fvm_port_sync_lock(queue->sync);
    void *value = queue->ready ? queue->slots[queue->ring[queue->head]].value : NULL;
    fvm_port_sync_unlock(queue->sync);
    return value;
}


/* Rotate ready metadata without borrowing capacity or withdrawing roots. */
void dvm_retirement_rotate(DVM_RetirementQueue *queue)
{
    fvm_port_sync_lock(queue->sync);
    if (queue->ready > 1)
    {
        unsigned int tail = (queue->head + queue->ready) % queue->capacity;
        queue->ring[tail] = queue->ring[queue->head];
        queue->head = (queue->head + 1) % queue->capacity;
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
        index = queue->ring[queue->head];
        slot = &queue->slots[index];
        *value = slot->value;
        *bytes = slot->bytes;
        queue->head = (queue->head+1)%queue->capacity;
        queue->ready--;
        queue->bytes -= slot->bytes;
        slot->value = NULL;
        slot->bytes = 0;
        slot->generation = 0;
        slot->state = 0;
        slot->next = queue->free_head;
        queue->free_head = index;
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
    free(queue->ring);
    free(queue);
    return 1;
}
