/* Native count contention and last-release publication; not a managed heap/GC test. */
#include "disvm/refcount.h"
#include "channel_sync.h"
#include "thread_port.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define THREADS 8 /* Independent OS owners sharing the same two native counters. */
#define ROUNDS 20000 /* Per-owner balanced retain/drop pairs with one permanent root. */


/* All non-count shared test fields use sync, or a documented final-release edge. */
typedef struct Shared
{
    FVM_PortSync *sync;        /* Protect startup barrier and terminal winner counts. */
    unsigned long heap;       /* Native unsigned count, accessed through real API. */
    int type;                 /* Signed Type count, accessed through real API. */
    int ready;                /* Number waiting at the private start predicate. */
    int start;                /* Release all created threads only after setup. */
    int last_heap;            /* Number of LAST outcomes in final-drop phase. */
    int message_frees;         /* Actual native messages retired by their sole LAST owner. */
    int last_type;            /* Number of signed LAST outcomes. */
    int payload_heap[THREADS]; /* Each owner writes its slot before unsigned release. */
    int payload_type[THREADS]; /* Each owner writes its slot before signed release. */
} Shared;


/* Stable native argument, borrowed from main until every OS thread joins. */
typedef struct Worker
{
    Shared *shared; /* Native fixture state, never a managed payload. */
    int index;     /* Unique payload slot owned by this thread. */
    int final;     /* Nonzero selects one final owned-reference release. */
} Worker;


/* A violated boundary is semantic status10, never assert or a signal oracle. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL refcount predicate: %s\n", message);
        exit(10);
    }
}


/* Start together, then contend on real counts. The final unsigned/signed winner
 * reads plain payload only AFTER its acquire/release RMW observes every prior
 * release. No startup/join/mutex edge is used for that payload read. */
static void execute(void *argument)
{
    Worker *worker = argument;
    Shared *shared = worker->shared;
    int index;
    fvm_port_sync_lock(shared->sync);
    shared->ready++;
    fvm_port_sync_signal_all(shared->sync);
    while (!shared->start)
    {
        fvm_port_sync_wait(shared->sync);
    }
    fvm_port_sync_unlock(shared->sync);
    if (!worker->final)
    {
        for (index = 0; index < ROUNDS; index++)
        {
            check(dvm_refcount_retain_u(&shared->heap) == DVM_REFCOUNT_LIVE, "contended unsigned retain");
            check(dvm_refcount_retain_i(&shared->type) == DVM_REFCOUNT_LIVE, "contended signed retain");
            check(dvm_refcount_release_u(&shared->heap) == DVM_REFCOUNT_LIVE, "root lost during unsigned stress");
            check(dvm_refcount_release_i(&shared->type) == DVM_REFCOUNT_LIVE, "root lost during signed stress");
        }
        return;
    }
    shared->payload_heap[worker->index] = 100 + worker->index;
    if (dvm_refcount_release_u(&shared->heap) == DVM_REFCOUNT_LAST)
    {
        for (index = 0; index < THREADS; index++)
        {
            check(shared->payload_heap[index] == 100 + index, "last unsigned release missed published payload");
        }
        fvm_port_sync_lock(shared->sync);
        shared->last_heap++;
        fvm_port_sync_unlock(shared->sync);
    }
    shared->payload_type[worker->index] = 200 + worker->index;
    if (dvm_refcount_release_i(&shared->type) == DVM_REFCOUNT_LAST)
    {
        for (index = 0; index < THREADS; index++)
        {
            check(shared->payload_type[index] == 200 + index, "last signed release missed published payload");
        }
        fvm_port_sync_lock(shared->sync);
        shared->last_type++;
        fvm_port_sync_unlock(shared->sync);
    }
}


/* main owns shared through joins; final selects one-reference-per-thread mode.
 * Plain initial stores precede thread publication and never race API access. */
static void contend(Shared *shared, int final)
{
    Worker workers[THREADS];
    FVM_PortThread *threads[THREADS];
    int index;
    shared->heap = final ? THREADS : 1;
    shared->type = final ? THREADS : 1;
    shared->ready = 0;
    shared->start = 0;
    for (index = 0; index < THREADS; index++)
    {
        workers[index].shared = shared;
        workers[index].index = index;
        workers[index].final = final;
        threads[index] = fvm_port_thread_start(execute, &workers[index]);
        check(threads[index] != NULL, "native thread creation");
    }
    fvm_port_sync_lock(shared->sync);
    while (shared->ready != THREADS)
    {
        fvm_port_sync_wait(shared->sync);
    }
    shared->start = 1;
    fvm_port_sync_signal_all(shared->sync);
    fvm_port_sync_unlock(shared->sync);
    for (index = 0; index < THREADS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    if (final)
    {
        check(!dvm_refcount_load_u(&shared->heap) && !dvm_refcount_load_i(&shared->type), "final count nonzero");
        check(shared->last_heap == 1 && shared->last_type == 1, "last release not unique");
        check(dvm_refcount_retain_u(&shared->heap) == DVM_REFCOUNT_REFUSED, "unsigned resurrection");
        check(dvm_refcount_retain_i(&shared->type) == DVM_REFCOUNT_REFUSED, "signed resurrection");
    }
    else
    {
        check(dvm_refcount_load_u(&shared->heap) == 1 && dvm_refcount_load_i(&shared->type) == 1, "count conservation");
        check(dvm_refcount_release_u(&shared->heap) == DVM_REFCOUNT_LAST, "unsigned root release");
        check(dvm_refcount_release_i(&shared->type) == DVM_REFCOUNT_LAST, "signed root release");
    }
}


/* Native message lifetime is separate from managed Heap layout. */
typedef struct Message
{
    unsigned long references; /* Retained before argument publication. */
    Shared *statistics;       /* Main keeps this native observer alive through join. */
    int value;                /* Immutable after worker publication. */
} Message;


/* Consume one message owner. Cache observer while alive; after successful drop
 * there is NO message access unless LAST granted sole retirement responsibility. */
static void drop_message(Message *message)
{
    Shared *shared = message->statistics;
    DVM_RefcountResult result = dvm_refcount_release_u(&message->references);
    check(result != DVM_REFCOUNT_REFUSED, "native message drop");
    if (result == DVM_REFCOUNT_LAST)
    {
        free(message);
        fvm_port_sync_lock(shared->sync);
        shared->message_frees++;
        fvm_port_sync_unlock(shared->sync);
    }
}


/* argument transfers an already-retained owner. No message read follows drop. */
static void consume_message(void *argument)
{
    Message *message = argument;
    check(message->value == 42, "native handoff payload");
    drop_message(message);
}


/* Repeated actual reclamation, not merely a counter reaching zero in permanent
 * storage. Sender and worker may each win LAST; no raw pointer is reacquired. */
static void handoffs(Shared *shared)
{
    int index;
    for (index = 0; index < 200; index++)
    {
        Message *message = calloc(1, sizeof(*message));
        FVM_PortThread *thread;
        check(message != NULL, "native message allocation");
        message->references = 1;
        message->statistics = shared;
        message->value = 42;
        check(dvm_refcount_retain_u(&message->references) == DVM_REFCOUNT_LIVE, "native handoff retain");
        thread = fvm_port_thread_start(consume_message, message);
        check(thread != NULL, "native handoff start");
        drop_message(message);
        fvm_port_thread_join(thread);
    }
    check(shared->message_frees == 200, "native messages not freed exactly once");
}


/* Test null/zero/overflow/negative boundaries before contended positive cases.
 * No object allocation/reclamation is implied by the count storage itself. */
int main(void)
{
    unsigned long heap = 0;
    int type = 0;
    Shared shared;
    check(!dvm_refcount_load_u(NULL) && !dvm_refcount_load_i(NULL), "null load");
    check(dvm_refcount_retain_u(NULL) == DVM_REFCOUNT_REFUSED &&
        dvm_refcount_release_u(NULL) == DVM_REFCOUNT_REFUSED &&
        dvm_refcount_retain_i(NULL) == DVM_REFCOUNT_REFUSED &&
        dvm_refcount_release_i(NULL) == DVM_REFCOUNT_REFUSED, "null operation");
    check(dvm_refcount_retain_u(&heap) == DVM_REFCOUNT_REFUSED && heap == 0, "zero retain");
    check(dvm_refcount_release_u(&heap) == DVM_REFCOUNT_REFUSED && heap == 0, "zero release");
    heap = ULONG_MAX;
    check(dvm_refcount_retain_u(&heap) == DVM_REFCOUNT_REFUSED && heap == ULONG_MAX, "unsigned overflow");
    check(dvm_refcount_release_u(&heap) == DVM_REFCOUNT_LIVE && heap == ULONG_MAX - 1, "unsigned maximum release");
    check(dvm_refcount_retain_u(&heap) == DVM_REFCOUNT_LIVE && heap == ULONG_MAX, "unsigned maximum retain");
    check(dvm_refcount_retain_i(&type) == DVM_REFCOUNT_REFUSED &&
        dvm_refcount_release_i(&type) == DVM_REFCOUNT_REFUSED && type == 0, "signed zero");
    type = -1;
    check(dvm_refcount_load_i(&type) == -1 && dvm_refcount_retain_i(&type) == DVM_REFCOUNT_REFUSED &&
        dvm_refcount_release_i(&type) == DVM_REFCOUNT_REFUSED && type == -1, "negative count");
    type = INT_MIN;
    check(dvm_refcount_retain_i(&type) == DVM_REFCOUNT_REFUSED &&
        dvm_refcount_release_i(&type) == DVM_REFCOUNT_REFUSED && type == INT_MIN, "minimum count");
    type = INT_MAX;
    check(dvm_refcount_retain_i(&type) == DVM_REFCOUNT_REFUSED && type == INT_MAX, "signed overflow");
    check(dvm_refcount_release_i(&type) == DVM_REFCOUNT_LIVE && type == INT_MAX - 1, "signed maximum release");
    memset(&shared, 0, sizeof(shared));
    shared.sync = fvm_port_sync_create();
    check(shared.sync != NULL, "native predicate allocation");
    contend(&shared, 0);
    contend(&shared, 1);
    handoffs(&shared);
    fvm_port_sync_destroy(shared.sync);
    printf("PASS native refcounts: %s, 8 threads, 160000 balanced pairs/type, unique last release, payload publication, 200 real handoff frees\n", dvm_refcount_backend());
    return 0;
}
