/* Copyright (c) 2026 bhgv
 * EN: Native cancellation FIFO growth, deduplication, order and shrinking.
 * RU: Рост FIFO отмен, устранение повторов, порядок и сжатие памяти.
 */
#include "host_cancellations.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRODUCERS 4 /* Native writers sharing the caller's queue mutex. */
#define REQUESTS 250 /* Unique IDs contributed by each writer. */

static pthread_mutex_t ledger = PTHREAD_MUTEX_INITIALIZER; /* Protect queue and expected order. */
static DVM_HostCancellations queue; /* Zero-initialized native request owner. */
static int expected[PRODUCERS * REQUESTS]; /* Exact publication order under ledger. */
static size_t published; /* Number of distinct expected publications. */


/* EN: Stop on a failed native invariant. message is borrowed.
 * RU: Завершить при нарушении инварианта. message заимствовано. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL host cancellation FIFO: %s\n", message);
        exit(1);
    }
}


/* EN: Publish a distinct range twice under ledger. argument owns its ID until join.
 * Return NULL after all native requests have been accepted.
 * RU: Опубликовать диапазон дважды под ledger. ID в argument живёт до join.
 * Вернуть NULL после приёма всех запросов. */
static void *produce(void *argument)
{
    int identity = *(int *)argument;
    for (int index = 0; index < REQUESTS; index++)
    {
        int pid = identity * REQUESTS + index + 1;
        pthread_mutex_lock(&ledger);
        dvm_host_cancellations_post(&queue, pid);
        dvm_host_cancellations_post(&queue, pid);
        expected[published++] = pid;
        pthread_mutex_unlock(&ledger);
    }
    return NULL;
}


/* EN: Join producers, verify FIFO, middle removal, shrinking and reuse.
 * Return zero only after all native storage and mutex resources are released.
 * RU: Завершить производителей, проверить FIFO, удаление, сжатие и повторный приём.
 * Вернуть ноль после освобождения всей памяти и мьютекса. */
int main(void)
{
    pthread_t threads[PRODUCERS];
    int identities[PRODUCERS];
    for (int index = 0; index < PRODUCERS; index++)
    {
        identities[index] = index;
        check(!pthread_create(&threads[index], NULL, produce, &identities[index]),
            "start producer");
    }
    for (int index = 0; index < PRODUCERS; index++)
    {
        check(!pthread_join(threads[index], NULL), "join producer");
    }
    pthread_mutex_lock(&ledger);
    check(queue.count == PRODUCERS * REQUESTS && published == queue.count &&
        queue.capacity >= queue.count, "all requests accepted once beyond 64 slots");
    check(dvm_host_cancellations_find(&queue, 999999) == queue.count, "absent PID");
    size_t middle = queue.count / 2;
    int removed = expected[middle];
    dvm_host_cancellations_remove(&queue, middle);
    memmove(expected + middle, expected + middle + 1,
        (published - middle - 1) * sizeof(*expected));
    published--;
    check(dvm_host_cancellations_find(&queue, removed) == queue.count, "middle PID removed");
    size_t original_capacity = queue.capacity;
    int shrunk = 0;
    for (size_t index = 0; index < published; index++)
    {
        check(queue.count && queue.pids[0] == expected[index], "global FIFO order");
        dvm_host_cancellations_remove(&queue, 0);
        shrunk |= queue.capacity && queue.capacity < original_capacity;
    }
    check(shrunk && !queue.count && !queue.capacity && !queue.pids,
        "partial shrink and complete storage release");
    dvm_host_cancellations_post(&queue, 7);
    dvm_host_cancellations_post(&queue, 7);
    check(queue.count == 1, "duplicates coalesce");
    dvm_host_cancellations_remove(&queue, 0);
    dvm_host_cancellations_post(&queue, 7);
    check(queue.count == 1 && queue.pids[0] == 7, "consumed PID can be requested again");
    dvm_host_cancellations_post(&queue, 8);
    dvm_host_cancellations_clear(&queue);
    check(!queue.count && !queue.capacity && !queue.pids, "shutdown clears pending work");
    pthread_mutex_unlock(&ledger);
    check(!pthread_mutex_destroy(&ledger), "destroy ledger mutex");
    puts("PASS 1000 host cancellations: concurrent FIFO,duplicates,middle removal,shrink,reuse,shutdown");
    return 0;
}
