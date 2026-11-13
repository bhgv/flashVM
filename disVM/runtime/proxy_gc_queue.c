/* Copyright (c) 2026 bhgv
 * EN: Thread-safe native lease slots with allocation-free collector retirement.
 * RU: Thread-safe native lease slots с allocation-free collector retirement.
 */
#include "proxy_gc_queue.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


static unsigned long created;   /* Atomic cumulative successful queue allocations. */
static unsigned long queues;    /* Atomic native metadata allocations alive. */
static unsigned long tokens;    /* Atomic managed tokens not yet finalized. */
static unsigned long pending;   /* Atomic release obligations waiting for broker. */
static unsigned long enqueued;  /* Atomic cumulative collector-origin releases. */
static unsigned long drained;   /* Atomic obligations handed to brokers. */
static unsigned long discarded; /* Atomic pending obligations revoked on close. */


/* EN/RU: Allocate fixed native storage before publication; one queue owner initially.
 * Выделить fixed native storage до публикации; исходно один queue owner. */
DvmProxyLeaseQueue *dvm_proxy_lease_queue_new(size_t limit)
{
    if (!limit || limit > DVM_PROXY_LEASE_LIMIT)
    {
        return NULL;
    }
    DvmProxyLeaseQueue *queue = calloc(1, sizeof(*queue));
    if (!queue || pthread_mutex_init(&queue->mutex, NULL))
    {
        free(queue);
        return NULL;
    }
    queue->thread = pthread_self();
    queue->references = 1;
    queue->next = 1;
    queue->limit = limit;
    __atomic_fetch_add(&queues, 1, __ATOMIC_RELAXED);
    __atomic_fetch_add(&created, 1, __ATOMIC_RELAXED);
    return queue;
}


/* EN/RU: Revoke all leases; collector may call this on the final connection owner.
 * Отозвать leases; GC может вызвать это для последнего connection owner. */
void dvm_proxy_lease_queue_close(DvmProxyLeaseQueue *queue)
{
    pthread_mutex_lock(&queue->mutex);
    queue->closed = 1;
    for (size_t index = 0; index < queue->limit; index++)
    {
        if (queue->entries[index].state == DVM_PROXY_LEASE_PENDING)
        {
            queue->entries[index].state = DVM_PROXY_LEASE_FREE;
            queue->live--;
            __atomic_fetch_sub(&pending, 1, __ATOMIC_RELAXED);
            __atomic_fetch_add(&discarded, 1, __ATOMIC_RELAXED);
        }
    }
    pthread_mutex_unlock(&queue->mutex);
}


/* EN/RU: Drop one native owner after its last queue use; no managed operations.
 * Снять native owner после последнего queue use; без managed operations. */
void dvm_proxy_lease_queue_drop(DvmProxyLeaseQueue *queue)
{
    pthread_mutex_lock(&queue->mutex);
    if (!queue->references)
    {
        abort();
    }
    int last = --queue->references == 0;
    if (last && (!queue->closed || queue->live))
    {
        abort();
    }
    pthread_mutex_unlock(&queue->mutex);
    if (last)
    {
        pthread_mutex_destroy(&queue->mutex);
        free(queue);
        __atomic_fetch_sub(&queues, 1, __ATOMIC_RELAXED);
    }
}


/* EN/RU: Reserve one exact token slot before remote ownership changes.
 * Зарезервировать точный token slot до изменения remote ownership. */
uint64_t dvm_proxy_lease_new(DvmProxyLeaseQueue *queue, DvmRemoteHandle owner, int active, size_t *slot)
{
    uint64_t serial = 0;
    pthread_mutex_lock(&queue->mutex);
    if (pthread_equal(queue->thread, pthread_self()) && !queue->closed && queue->next && queue->live < queue->limit &&
        queue->references != SIZE_MAX)
    {
        for (size_t index = 0; index < queue->limit; index++)
        {
            DvmProxyLeaseEntry *entry = &queue->entries[index];
            if (entry->state == DVM_PROXY_LEASE_FREE)
            {
                serial = queue->next;
                queue->next = serial == UINT64_MAX ? 0 : serial + 1;
                *entry = (DvmProxyLeaseEntry){serial, owner, active ? DVM_PROXY_LEASE_ACTIVE : DVM_PROXY_LEASE_STAGED};
                *slot = index;
                queue->live++;
                queue->references++;
                __atomic_fetch_add(&tokens, 1, __ATOMIC_RELAXED);
                break;
            }
        }
    }
    pthread_mutex_unlock(&queue->mutex);
    return serial;
}


/* EN/RU: DVM_PROXY_LEASE_ACTIVE read is any-thread; mutations are exact creating-broker only.
 * DVM_PROXY_LEASE_ACTIVE read доступен любому thread; mutation — лишь исходному broker. */
int dvm_proxy_lease_action(DvmProxyLeaseQueue *queue, size_t slot, uint64_t serial, int action)
{
    int success = 0;
    pthread_mutex_lock(&queue->mutex);
    if (!queue->closed && slot < queue->limit && queue->entries[slot].serial == serial)
    {
        DvmProxyLeaseEntry *entry = &queue->entries[slot];
        if (action == 0)
        {
            success = entry->state == DVM_PROXY_LEASE_ACTIVE;
        }
        else if (pthread_equal(queue->thread, pthread_self()))
        {
            if (action == 1 && entry->state == DVM_PROXY_LEASE_STAGED)
            {
                entry->state = DVM_PROXY_LEASE_ACTIVE;
                success = 1;
            }
            else if (action == 2 && entry->state == DVM_PROXY_LEASE_ACTIVE)
            {
                entry->state = DVM_PROXY_LEASE_RELEASED;
                success = 1;
            }
        }
    }
    pthread_mutex_unlock(&queue->mutex);
    return success;
}


/* EN/RU: Bind a caller image to immutable token metadata, not mutable Image fields.
 * Связать caller image с immutable token metadata, не mutable Image fields. */
int dvm_proxy_lease_matches(DvmProxyLeaseQueue *queue, size_t slot, uint64_t serial, DvmRemoteHandle owner)
{
    int matched = 0;
    pthread_mutex_lock(&queue->mutex);
    if (!queue->closed && slot < queue->limit)
    {
        DvmProxyLeaseEntry *entry = &queue->entries[slot];
        matched = entry->serial == serial && entry->state == DVM_PROXY_LEASE_ACTIVE &&
                  entry->owner.session == owner.session && entry->owner.slot == owner.slot &&
                  entry->owner.generation == owner.generation;
    }
    pthread_mutex_unlock(&queue->mutex);
    return matched;
}


/* EN/RU: Retire exactly one finalized token; pending slot outlives managed payload.
 * Завершить ровно один token; pending slot переживает managed payload. */
void dvm_proxy_lease_retire(DvmProxyLeaseQueue *queue, size_t slot, uint64_t serial)
{
    pthread_mutex_lock(&queue->mutex);
    if (slot >= queue->limit || queue->entries[slot].serial != serial ||
        queue->entries[slot].state == DVM_PROXY_LEASE_FREE || queue->entries[slot].state == DVM_PROXY_LEASE_PENDING)
    {
        abort();
    }
    DvmProxyLeaseEntry *entry = &queue->entries[slot];
    if (entry->state == DVM_PROXY_LEASE_ACTIVE && !queue->closed)
    {
        entry->state = DVM_PROXY_LEASE_PENDING;
        __atomic_fetch_add(&pending, 1, __ATOMIC_RELAXED);
        __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
    }
    else
    {
        entry->state = DVM_PROXY_LEASE_FREE;
        queue->live--;
    }
    pthread_mutex_unlock(&queue->mutex);
    __atomic_fetch_sub(&tokens, 1, __ATOMIC_RELAXED);
    dvm_proxy_lease_queue_drop(queue);
}


/* EN/RU: Transfer one pending Release obligation to the original broker.
 * Передать pending Release obligation исходному broker. */
int dvm_proxy_lease_pop(DvmProxyLeaseQueue *queue, DvmRemoteHandle *owner)
{
    int found = 0;
    pthread_mutex_lock(&queue->mutex);
    if (!queue->closed && pthread_equal(queue->thread, pthread_self()))
    {
        for (size_t index = 0; index < queue->limit; index++)
        {
            if (queue->entries[index].state == DVM_PROXY_LEASE_PENDING)
            {
                *owner = queue->entries[index].owner;
                queue->entries[index].state = DVM_PROXY_LEASE_FREE;
                queue->live--;
                __atomic_fetch_sub(&pending, 1, __ATOMIC_RELAXED);
                __atomic_fetch_add(&drained, 1, __ATOMIC_RELAXED);
                found = 1;
                break;
            }
        }
    }
    pthread_mutex_unlock(&queue->mutex);
    return found;
}


/* EN/RU: Report joined counters without allocation or touching managed values.
 * Вывести joined counters без allocation и доступа к managed values. */
void dvm_proxy_lease_statistics(void)
{
    if (!__atomic_load_n(&created, __ATOMIC_RELAXED))
    {
        return;
    }
    fprintf(stderr,
            "PROXYGC queues=%lu tokens=%lu pending=%lu enqueued=%lu drained=%lu discarded=%lu\n",
            __atomic_load_n(&queues, __ATOMIC_RELAXED),
            __atomic_load_n(&tokens, __ATOMIC_RELAXED),
            __atomic_load_n(&pending, __ATOMIC_RELAXED),
            __atomic_load_n(&enqueued, __ATOMIC_RELAXED),
            __atomic_load_n(&drained, __ATOMIC_RELAXED),
            __atomic_load_n(&discarded, __ATOMIC_RELAXED));
}
