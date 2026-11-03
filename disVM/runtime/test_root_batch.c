/* Real registry groups, opaque native keys, eight joined OS owners. */
#include "root_batch.h"
#include "root_registry.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WORKERS 8 /* Independent OS owners; no VM or fabricated Heap. */
#define PAIRS 2000 /* Publish/clear cycles per OS owner. */
#define WIDTH 4 /* Occurrences committed atomically by each owner. */

static FVM_PortSync *gate; /* Parent-owned start barrier, alive through joins. */
static int ready; /* Barrier counter protected by gate. */
static int go; /* Parent release flag protected by gate. */
static __thread int no_allocate; /* Monitor actual allocation attempts. */
static __thread int fail_after = -1; /* Deterministic controller-only calloc refusal. */
void *__real_malloc(size_t bytes);
void *__real_calloc(size_t count, size_t bytes);


/* Native keys are immutable and independently live through all joined users. */
typedef struct Key
{
    unsigned int index; /* Independent visitor oracle index. */
} Key;


/* One caller exclusively owns its batch; keys are borrowed from parent. */
typedef struct Work
{
    Key *key; /* Stable immutable identity. */
    DVM_RootBatch *batch; /* Reserved before entering allocation-free interval. */
} Work;


/* Stop at semantic 10, never under malformed-list traversal or freed metadata. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        const char *prefix = "FAIL root batch: ";
        ssize_t written = write(2, prefix, strlen(prefix));
        (void)written;
        written = write(2, message, strlen(message));
        (void)written;
        written = write(2, "\n", 1);
        (void)written;
        _Exit(10);
    }
}


/* Observe allocator attempts rather than success: no warmed-pool loophole. */
void *__wrap_malloc(size_t bytes)
{
    check(!no_allocate, "publication allocation attempt");
    return __real_malloc(bytes);
}


/* Refuse each actual reservation allocation without altering existing roots. */
void *__wrap_calloc(size_t count, size_t bytes)
{
    check(!no_allocate, "publication allocation attempt");
    if (fail_after == 0)
    {
        fail_after = -1;
        return NULL;
    }
    if (fail_after > 0)
    {
        fail_after--;
    }
    return __real_calloc(count, bytes);
}


/* Existing root visitor runs under actual registry exclusion, no allocation. */
static void visit(void *value, void *argument)
{
    Key *key = value;
    unsigned int *counts = argument;
    check(key && key->index < WORKERS, "visitor key lifetime");
    counts[key->index]++;
}


/* Deterministic controls run before OS workers so semantic failures are exact.
 * Native keys never enter a collector process; this tests metadata only. */
static void serial(void)
{
    Key key = {0};
    void *keys[WIDTH] = {&key, &key, &key, &key};
    void *bad[2] = {&key, NULL};
    DVM_RootBatch *batch = dvm_runtime_root_batch_create(WIDTH);
    DVM_RootBatch *other = dvm_runtime_root_batch_create(WIDTH);
    unsigned int counts[WORKERS] = {0};
    int index;
    check(batch && other && !dvm_runtime_root_batch_create(0) &&
        !dvm_runtime_root_batch_create((size_t)-1), "reservation geometry");
    for (index = 0; index < 2; index++)
    {
        fail_after = index;
        check(!dvm_runtime_root_batch_create(WIDTH) && fail_after == -1 &&
            !dvm_runtime_root_count(&key), "reservation failure rollback");
    }
    no_allocate = 1;
    check(!dvm_runtime_root_batch_publish(batch, bad, 2) && !dvm_runtime_root_count(&key),
        "invalid vector unchanged");
    check(!dvm_runtime_root_batch_publish(batch, keys, WIDTH + 1) &&
        !dvm_runtime_root_batch_publish(batch, NULL, 1), "oversize/null vector refusal");
    check(dvm_runtime_root_batch_publish(batch, keys, WIDTH) &&
        dvm_runtime_root_count(&key) == WIDTH, "complete publication");
    check(!dvm_runtime_root_remove(&key) && dvm_runtime_root_count(&key) == WIDTH,
        "ordinary remove preserves batch");
    check(!dvm_runtime_root_batch_destroy(&batch) && batch &&
        !dvm_runtime_root_batch_publish(batch, keys, WIDTH), "live reservation refusal");
    check(dvm_runtime_root_batch_publish(other, keys, WIDTH) &&
        dvm_runtime_root_count(&key) == 2 * WIDTH, "independent duplicate batches");
    check(dvm_runtime_root_batch_remove(batch, 1) &&
        !dvm_runtime_root_batch_remove(batch, 1) &&
        !dvm_runtime_root_batch_remove(batch, WIDTH) &&
        dvm_runtime_root_count(&key) == 2 * WIDTH - 1,
        "single occurrence withdrawal");
    check(dvm_runtime_root_batch_clear(batch) && dvm_runtime_root_count(&key) == WIDTH,
        "exact batch removal");
    dvm_runtime_roots_visit(visit, counts);
    check(counts[0] == WIDTH, "visitor remaining batch");
    check(dvm_runtime_root_batch_clear(other) && !dvm_runtime_root_count(&key),
        "complete clear");
    check(!dvm_runtime_root_batch_clear(batch) &&
        dvm_runtime_root_batch_publish(batch, NULL, 0) &&
        !dvm_runtime_root_batch_destroy(&batch) && dvm_runtime_root_batch_clear(batch),
        "empty group pins reservation");
    no_allocate = 0;
    check(dvm_runtime_root_add(&key), "ordinary occurrence");
    no_allocate = 1;
    check(dvm_runtime_root_batch_publish(batch, keys, WIDTH) &&
        dvm_runtime_root_remove(&key) && !dvm_runtime_root_remove(&key) &&
        dvm_runtime_root_count(&key) == WIDTH && dvm_runtime_root_batch_clear(batch),
        "mixed occurrence separation");
    no_allocate = 0;
    check(dvm_runtime_root_batch_destroy(&batch) && !batch &&
        dvm_runtime_root_batch_destroy(&other) && !other, "reservation teardown");
}


/* Allocate on this native owner before the barrier; publish/clear are allocation
 * free. Parent never touches the batch; all keys outlive the final thread join. */
static void writer(void *argument)
{
    Work *work = argument;
    void *keys[WIDTH] = {work->key, work->key, work->key, work->key};
    int index;
    work->batch = dvm_runtime_root_batch_create(WIDTH);
    check(work->batch != NULL, "OS owner reserves batch");
    fvm_port_sync_lock(gate);
    ready++;
    fvm_port_sync_signal_all(gate);
    while (!go)
    {
        fvm_port_sync_wait(gate);
    }
    fvm_port_sync_unlock(gate);
    no_allocate = 1;
    for (index = 0; index < PAIRS; index++)
    {
        check(dvm_runtime_root_batch_publish(work->batch, keys, WIDTH) &&
            dvm_runtime_root_batch_clear(work->batch), "OS owner group cycle");
    }
    no_allocate = 0;
    check(dvm_runtime_root_batch_destroy(&work->batch), "OS owner destroys reservation");
}


/* Return 0 after exact atomic visitor observations and all OS joins. Controls
 * select --serial-only, never treating a timeout/crash as evidence. */
int main(int argc, char **argv)
{
    Key keys[WORKERS];
    Work work[WORKERS] = {{0}};
    FVM_PortThread *threads[WORKERS];
    unsigned int counts[WORKERS];
    int index;
    int scan;
    if (argc > 1 && !strcmp(argv[1], "--serial-only"))
    {
        serial();
        puts("PASS root batch serial: exact occurrences, refusal, no allocation");
        return 0;
    }
    gate = fvm_port_sync_create();
    check(gate != NULL, "start gate");
    for (index = 0; index < WORKERS; index++)
    {
        keys[index].index = (unsigned int)index;
        work[index].key = &keys[index];
        threads[index] = fvm_port_thread_start(writer, &work[index]);
        check(threads[index] != NULL, "start OS owner");
    }
    fvm_port_sync_lock(gate);
    while (ready != WORKERS)
    {
        fvm_port_sync_wait(gate);
    }
    go = 1;
    fvm_port_sync_signal_all(gate);
    fvm_port_sync_unlock(gate);
    for (scan = 0; scan < 1000; scan++)
    {
        memset(counts, 0, sizeof(counts));
        dvm_runtime_roots_visit(visit, counts);
        for (index = 0; index < WORKERS; index++)
        {
            check(counts[index] == 0 || counts[index] == WIDTH, "visitor sees whole groups");
        }
    }
    for (index = 0; index < WORKERS; index++)
    {
        fvm_port_thread_join(threads[index]);
        check(!work[index].batch && !dvm_runtime_root_count(&keys[index]), "joined roots retired");
    }
    fvm_port_sync_destroy(gate);
    serial();
    puts("PASS root batch: 8 OS owners,16000 cycles,1000 atomic traversals, all joins");
    return 0;
}
