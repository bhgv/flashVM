/* Actual queue/ports with immutable native identities, never managed Heap mocks. */
#include "retirement_queue.h"
#include "channel_sync.h"
#include "thread_port.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define WORKERS 8 /* Concurrent native producers with independent tokens. */
#define ROUNDS 2000 /* Each producer publishes this many distinct native identities. */


/* Identity remains immutable and allocated until all threads/consumer finish. */
typedef struct Key
{
    unsigned int index; /* Exact once-only observation by the sole consumer. */
} Key;


/* One private producer borrows the shared queue and a distinct identity span. */
typedef struct Producer
{
    DVM_RetirementQueue *queue; /* Stable owner until every join completes. */
    Key *keys; /* Borrowed immutable keys through join. */
} Producer;

static __thread unsigned int refuse_at; /* Serial allocation-failure ordinal,0 disables. */
static __thread int tracking; /* Count private native allocations during failure test. */
static __thread int live_allocations; /* Must return to0 after every failed create. */
static __thread int depth; /* Actual queue-mutex depth on this OS thread. */
void __real_fvm_port_sync_lock(FVM_PortSync *sync);
void __real_fvm_port_sync_unlock(FVM_PortSync *sync);
void *__real_malloc(size_t bytes);
void *__real_calloc(size_t count, size_t bytes);
void __real_free(void *value);


/* Explicit semantic stop before unsafe continuation, even under negative locks. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        ssize_t result = write(2, message, strlen(message));
        (void)result;
        result = write(2, "\n", 1);
        (void)result;
        _Exit(10);
    }
}


/* Track actual queue mutex, not an independent model of queue state. */
void __wrap_fvm_port_sync_lock(FVM_PortSync *sync)
{
    __real_fvm_port_sync_lock(sync);
    depth++;
}


/* Clear diagnostic depth before actual unlock. */
void __wrap_fvm_port_sync_unlock(FVM_PortSync *sync)
{
    check(depth == 1, "FAIL retire lock balance");
    depth--;
    __real_fvm_port_sync_unlock(sync);
}


/* Queue metadata operations must not allocate under exclusion. */
void *__wrap_malloc(size_t bytes)
{
    check(!depth, "FAIL retire allocation under lock");
    void *value;
    if (refuse_at && --refuse_at == 0)
    {
        return NULL;
    }
    value = __real_malloc(bytes);
    if (tracking && value)
    {
        live_allocations++;
    }
    return value;
}


/* Initialization allocates only before native owner publication. */
void *__wrap_calloc(size_t count, size_t bytes)
{
    check(!depth, "FAIL retire allocation under lock");
    void *value;
    if (refuse_at && --refuse_at == 0)
    {
        return NULL;
    }
    value = __real_calloc(count, bytes);
    if (tracking && value)
    {
        live_allocations++;
    }
    return value;
}


/* Native destruction occurs only after queue unlock/quiescence. */
void __wrap_free(void *value)
{
    check(!depth, "FAIL retire free under lock");
    if (tracking && value)
    {
        live_allocations--;
    }
    __real_free(value);
}


/* Serial capability/budget/lifecycle checks, before any negative worker launch. */
static void boundaries(void)
{
    DVM_RetirementQueue *queue = dvm_retirement_create(2, 10);
    DVM_RetirementQueue *foreign = dvm_retirement_create(2, 10);
    DVM_RetirementReservation a = {0};
    DVM_RetirementReservation b = {0};
    DVM_RetirementReservation empty = {0};
    DVM_RetirementReservation stale;
    DVM_RetirementStats stats;
    unsigned int refusal;
    int first = 1;
    int second = 2;
    void *value = &first;
    unsigned long bytes = 99;
    tracking = 1;
    for (refusal = 1; refusal <= 4; refusal++)
    {
        refuse_at = refusal;
        check(!dvm_retirement_create(2, 10) && !live_allocations && !refuse_at,
            "FAIL retire create rollback");
    }
    tracking = 0;
    check(queue && foreign && !dvm_retirement_create(0, 10), "FAIL retire init");
    check(dvm_retirement_reserve(queue, 6, &a) == DVM_RETIRE_OK, "FAIL retire reserve");
    check(!dvm_retirement_destroy(&queue), "FAIL retire pending destroy");
    check(!dvm_retirement_commit(queue, &a, NULL) && a.queue == queue,
        "FAIL retire null publication");
    check(dvm_retirement_reserve(queue, ~0UL, &b) == DVM_RETIRE_BYTES && !b.queue,
        "FAIL retire overflow refusal");
    check(dvm_retirement_reserve(queue, 5, &b) == DVM_RETIRE_BYTES && !b.queue,
        "FAIL retire byte refusal");
    check(!dvm_retirement_commit(foreign, &a, &first), "FAIL retire foreign token");
    stale = a;
    check(dvm_retirement_cancel(queue, &a), "FAIL retire cancel");
    check(dvm_retirement_statistics(queue, &stats) && !stats.bytes && !stats.reserved,
        "FAIL retire cancel budget");
    check(dvm_retirement_reserve(queue, 6, &a) == DVM_RETIRE_OK, "FAIL retire reuse");
    check(!dvm_retirement_cancel(queue, &stale), "FAIL retire stale token");
    check(dvm_retirement_reserve(queue, 4, &b) == DVM_RETIRE_OK, "FAIL retire second reserve");
    check(dvm_retirement_reserve(queue, 0, &empty) == DVM_RETIRE_FULL, "FAIL retire capacity refusal");
    check(!dvm_retirement_take(queue, &value, &bytes) && value == &first && bytes == 99,
        "FAIL retire unpublished take");
    dvm_retirement_close(queue);
    check(dvm_retirement_reserve(queue, 0, &empty) == DVM_RETIRE_CLOSED, "FAIL retire closed admission");
    int edge = 0;
    check(dvm_retirement_commit_ready(queue, &b, &second, &edge) && edge &&
        dvm_retirement_commit_ready(queue, &a, &first, &edge),
        "FAIL retire accepted close commit");
    check(dvm_retirement_statistics(queue, &stats) && stats.ready == 2 && !stats.reserved && stats.bytes == 10,
        "FAIL retire ready publication");
    check(!edge, "FAIL retire duplicate wake on nonempty queue");
    check(!dvm_retirement_destroy(&queue), "FAIL retire ready destroy");
    check(dvm_retirement_peek(queue) == &second, "FAIL retire initial FIFO head");
    dvm_retirement_rotate(queue);
    check(dvm_retirement_peek(queue) == &first &&
        dvm_retirement_statistics(queue, &stats) && stats.ready == 2 && stats.bytes == 10,
        "FAIL retire full rotation");
    dvm_retirement_rotate(queue);
    check(dvm_retirement_take(queue, &value, &bytes) && value == &second && bytes == 4,
        "FAIL retire commit FIFO");
    check(dvm_retirement_take(queue, &value, &bytes) && value == &first && bytes == 6,
        "FAIL retire final FIFO");
    check(dvm_retirement_destroy(&queue) && dvm_retirement_destroy(&foreign), "FAIL retire destroy");
}


/* A duplicate key is refused without consuming its reservation. */
static void indexed_case(void)
{
    DVM_RetirementQueue *queue = dvm_retirement_create_indexed(2, 8);
    DVM_RetirementReservation first = {0}, second = {0};
    int value;
    DVM_HeapKey key = {(uintptr_t)&value, 1}, next = {(uintptr_t)&value, 2};
    check(queue && dvm_retirement_reserve(queue, 4, &first) == DVM_RETIRE_OK &&
        dvm_retirement_reserve(queue, 4, &second) == DVM_RETIRE_OK &&
        dvm_retirement_commit_key(queue, &first, &value, key, NULL) &&
        !dvm_retirement_commit_key(queue, &second, &value, key, NULL) &&
        dvm_retirement_commit_key(queue, &second, &value, next, NULL),
        "FAIL indexed retirement duplicate or generation");
    dvm_retirement_rotate(queue);
    void *out;
    unsigned long bytes;
    check(dvm_retirement_contains_key(queue, key) && dvm_retirement_contains_key(queue, next) &&
        dvm_retirement_take(queue, &out, &bytes) && !dvm_retirement_contains_key(queue, next) &&
        dvm_retirement_contains_key(queue, key) && dvm_retirement_take(queue, &out, &bytes) &&
        !dvm_retirement_contains_key(queue, key) && dvm_retirement_destroy(&queue),
        "FAIL indexed retirement removal");
}


/* A full queue is a bounded immediate refusal. Retry outside its lock; producer
 * owns token and identity until commit succeeds, including a cancelled draft. */
static void produce(void *argument)
{
    Producer *producer = argument;
    unsigned int index;
    for (index = 0; index < ROUNDS; index++)
    {
        DVM_RetirementReservation token = {0};
        DVM_RetirementResult result;
        do
        {
            result = dvm_retirement_reserve(producer->queue, 1, &token);
            if (result == DVM_RETIRE_FULL)
            {
                fvm_port_thread_sleep_us(1);
            }
        } while (result == DVM_RETIRE_FULL);
        check(result == DVM_RETIRE_OK, "FAIL retire producer reserve");
        if (index % 7 == 0)
        {
            check(dvm_retirement_cancel(producer->queue, &token), "FAIL retire producer cancel");
            do
            {
                result = dvm_retirement_reserve(producer->queue, 1, &token);
                if (result == DVM_RETIRE_FULL)
                {
                    fvm_port_thread_sleep_us(1);
                }
            } while (result == DVM_RETIRE_FULL);
            check(result == DVM_RETIRE_OK, "FAIL retire producer re-reserve");
        }
        check(dvm_retirement_commit(producer->queue, &token, &producer->keys[index]),
            "FAIL retire producer commit");
    }
}


/* Join every producer before native identity/queue lifetime ends. Sole consumer
 * checks all16000 immutable identities exactly once, then exact empty metadata. */
int main(int argc, char **argv)
{
    DVM_RetirementQueue *queue;
    DVM_RetirementStats stats;
    Producer producers[WORKERS];
    FVM_PortThread *threads[WORKERS];
    Key *keys;
    unsigned char *seen;
    unsigned int index;
    unsigned int received = 0;
    boundaries();
    indexed_case();
    if (argc > 1 && !strcmp(argv[1], "--serial-only"))
    {
        puts("PASS retire serial boundaries");
        return 0;
    }
    queue = dvm_retirement_create(64, 64);
    keys = calloc(WORKERS*ROUNDS, sizeof(*keys));
    seen = calloc(WORKERS*ROUNDS, 1);
    check(queue && keys && seen, "FAIL retire native fixture storage");
    for (index = 0; index < WORKERS*ROUNDS; index++)
    {
        keys[index].index = index;
    }
    for (index = 0; index < WORKERS; index++)
    {
        producers[index].queue = queue;
        producers[index].keys = keys+index*ROUNDS;
        threads[index] = fvm_port_thread_start(produce, &producers[index]);
        check(threads[index] != NULL, "FAIL retire producer start");
    }
    while (received < WORKERS*ROUNDS)
    {
        void *value;
        unsigned long bytes;
        if (dvm_retirement_take(queue, &value, &bytes))
        {
            Key *key = value;
            check(bytes == 1 && (uintptr_t)key >= (uintptr_t)keys &&
                (uintptr_t)key < (uintptr_t)(keys+WORKERS*ROUNDS) &&
                ((uintptr_t)key-(uintptr_t)keys)%sizeof(*keys) == 0,
                "FAIL retire native identity extent");
            check(key->index < WORKERS*ROUNDS, "FAIL retire native identity index");
            check(!seen[key->index], "FAIL retire duplicate identity");
            seen[key->index] = 1;
            received++;
        }
        else
        {
            fvm_port_thread_sleep_us(1);
        }
    }
    for (index = 0; index < WORKERS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    dvm_retirement_close(queue);
    check(dvm_retirement_statistics(queue, &stats) && !stats.ready && !stats.reserved && !stats.bytes,
        "FAIL retire joined metadata");
    check(dvm_retirement_destroy(&queue), "FAIL retire joined destroy");
    free(keys);
    free(seen);
    puts("PASS retirement queue: 8 OS producers, 16000 exact identities, reserve/cancel/commit/take");
    return 0;
}
