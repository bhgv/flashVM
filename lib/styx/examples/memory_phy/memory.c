/* Copyright (c) 2026 bhgv
 * EN: Construct/adopt two endpoint capabilities and retire their shared allocation.
 * RU: Создать/передать две endpoint capabilities и освободить shared allocation.
 * standalone-reviewed: failed factories never leave a half-owned carrier behind.
 */
#include "internal.h"
#include <stdlib.h>


/* EN/RU: See internal.h; counters carry no synchronization or permission meaning.
 * См. internal.h; counters не являются синхронизацией или правами.
 */
void memory_count(unsigned long *counter, unsigned long amount)
{
    __atomic_add_fetch(counter, amount, __ATOMIC_RELAXED); /* Observations only, not publication of ring contents. */
}


/* EN: state is an adopted MemoryEnd. Disconnect once through the owner-serialized
 * wrapper; persistent peer state wakes a reader/writer without closing a descriptor.
 * RU: state — принятый MemoryEnd. Owner-serialized wrapper вызывает disconnect один раз;
 * persistent peer state будит reader/writer без закрытия дескриптора.
 */
void memory_disconnect(void *state)
{
    MemoryEnd *end = state;
    MemoryLink *link = end->link;
    pthread_mutex_lock(&link->lock);
    link->open[end->side] = 0;
    pthread_mutex_unlock(&link->lock);
    memory_notify(&link->ends[0]); /* Peer must observe EOF after draining already accepted bytes. */
    memory_notify(&link->ends[1]);
}


/* EN: Consume endpoint state after its I/O/wake callers retire. The other side
 * can still run; the last reference owns mutex destruction and allocation release.
 * RU: Поглотить state после завершения его I/O/wake callers. Другая сторона ещё
 * может работать; последняя ссылка уничтожает mutex и shared allocation.
 */
void memory_destroy(void *state)
{
    MemoryEnd *end = state;
    MemoryLink *link = end->link;
    memory_count(&link->stats->destroyed, 1);
    pthread_mutex_lock(&link->lock);
    int last = --link->references == 0;
    pthread_mutex_unlock(&link->lock);
    if (last)
    {
        pthread_mutex_destroy(&link->lock); /* Both endpoints and all wake users have retired by contract. */
        free(link);
    }
}


/* EN/RU: See memory.h; phycreate adopts callback state only on success.
 * См. memory.h; phycreate принимает callback state только при успехе.
 */
int memory_pair(RemotePhy **left, RemotePhy **right, MemoryStats *stats, size_t cut_after)
{
    if (!left || !right || left == right || !stats)
    {
        return 0;
    }
    *left = NULL;
    *right = NULL;
    MemoryLink *link = calloc(1, sizeof(*link)); /* Two tiny incoming rings, not an unbounded message queue. */
    if (!link)
    {
        return 0;
    }
    if (pthread_mutex_init(&link->lock, NULL))
    {
        free(link);
        return 0;
    }
    link->references = 2;
    link->stats = stats;
    for (unsigned side = 0; side < 2; side++)
    {
        link->open[side] = 1;
        link->ends[side].link = link;
        link->ends[side].side = side;
    }
    link->ends[0].cut_after = cut_after; /* Default zero keeps the normal driver fault-free. */
    static const PhyOps ops = {memory_read, memory_write, memory_wait, memory_wake, NULL, NULL,
                               memory_disconnect, memory_destroy};
    PhyInfo info = {PHY_STREAM | PHY_ORDERED | PHY_RELIABLE, 0, 3, 2, 1}; /* Raw packets must not claim this stream contract. */
    RemotePhy *first = phycreate(&ops, &link->ends[0], &info);
    if (!first)
    {
        memory_destroy(&link->ends[0]);
        memory_destroy(&link->ends[1]); /* Shared allocation survives until the second cleanup. */
        return 0;
    }
    RemotePhy *second = phycreate(&ops, &link->ends[1], &info);
    if (!second)
    {
        phyfree(first); /* Constructor failure consumed no state for the second endpoint. */
        memory_destroy(&link->ends[1]);
        return 0;
    }
    *left = first;
    *right = second; /* Publication transfers two independent owner obligations to the caller. */
    return 1;
}
