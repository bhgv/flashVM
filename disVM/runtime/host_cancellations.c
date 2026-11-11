/* Copyright (c) 2026 bhgv
 * EN: Deduplicated native cancellation FIFO with growing and shrinking storage.
 * RU: Нативная FIFO отмен без повторов, с ростом и сжатием памяти.
 */
#include "host_cancellations.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 64 /* Initial allocation, never an admission limit. */


/* EN: Search borrowed queue under caller's lock. Return count when absent.
 * RU: Найти pid под блокировкой вызывающего. При отсутствии вернуть count. */
size_t dvm_host_cancellations_find(const DVM_HostCancellations *queue, int pid)
{
    size_t index;
    for (index = 0; index < queue->count && queue->pids[index] != pid; index++)
    {
    }
    return index;
}


/* EN: Grow before publishing pid. Deduplication preserves the oldest request.
 * Queue and positive pid are borrowed under caller's lock. OOM is fatal.
 * RU: Выделить память до публикации pid, сохраняя первый запрос при повторе.
 * Очередь и положительный pid заимствованы под блокировкой. Нехватка памяти фатальна. */
void dvm_host_cancellations_post(DVM_HostCancellations *queue, int pid)
{
    if (dvm_host_cancellations_find(queue, pid) < queue->count)
    {
        return;
    }
    if (queue->count == queue->capacity)
    {
        if (queue->capacity > SIZE_MAX / (2 * sizeof(*queue->pids)))
        {
            fprintf(stderr, "disvm: host cancellation queue size overflow\n");
            abort();
        }
        size_t capacity = queue->capacity ? queue->capacity * 2 : INITIAL_CAPACITY;
        int *pids = realloc(queue->pids, capacity * sizeof(*pids));
        if (!pids)
        {
            fprintf(stderr, "disvm: out of memory queuing host cancellation\n");
            abort();
        }
        queue->pids = pids;
        queue->capacity = capacity;
    }
    queue->pids[queue->count++] = pid;
}


/* EN: Clear borrowed queue under caller's lock. No managed references are held.
 * RU: Очистить заимствованную очередь под блокировкой. Управляемых ссылок нет. */
void dvm_host_cancellations_clear(DVM_HostCancellations *queue)
{
    free(queue->pids);
    memset(queue, 0, sizeof(*queue));
}


/* EN: Remove valid index, retain FIFO order and release unused native capacity.
 * Caller holds the queue lock. A shrink allocation failure preserves old storage.
 * RU: Удалить допустимый индекс, сохранить FIFO и освободить лишнюю память.
 * Мьютекс удерживается вызывающим. Отказ сжатия сохраняет прежнюю память. */
void dvm_host_cancellations_remove(DVM_HostCancellations *queue, size_t index)
{
    if (index >= queue->count)
    {
        fprintf(stderr, "disvm: invalid host cancellation queue removal\n");
        abort();
    }
    queue->count--;
    memmove(queue->pids + index, queue->pids + index + 1,
        (queue->count - index) * sizeof(*queue->pids));
    if (!queue->count)
    {
        dvm_host_cancellations_clear(queue);
    }
    else if (queue->capacity > INITIAL_CAPACITY && queue->count <= queue->capacity / 4)
    {
        size_t capacity = queue->capacity / 2;
        int *pids = realloc(queue->pids, capacity * sizeof(*pids));
        if (pids)
        {
            queue->pids = pids;
            queue->capacity = capacity;
        }
    }
}
