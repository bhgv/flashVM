/* Actual native root metadata with opaque native keys, never a simulated heap. */
#include "root_registry.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define WORKERS 8 /* OS writers racing first publication and table updates. */
#define PAIRS 2000 /* Remove/add pairs while one base registration persists. */


/* Immutable native payload; registry operations inspect only its address. */
typedef struct Key
{
    unsigned int index; /* Callback oracle; stable through every join. */
} Key;


/* Parent-owned barrier and keys stay live until all threads join. */
typedef struct Work
{
    Key keys[WORKERS]; /* Native stack addresses, no managed object pointers. */
    int ready; /* Workers before first registration, protected by gate. */
    int initialized; /* Successful concurrent initial adds, protected by gate. */
    int phase; /* Parent opens initialization then work, protected by gate. */
} Work;


/* One thread borrows parent-owned work and its distinct key. */
typedef struct Writer
{
    Work *work; /* Borrowed through join. */
    Key *key; /* Immutable native identity. */
} Writer;

static FVM_PortSync *gate; /* Immutable after thread creation; excluded from root-lock tracking. */
static __thread int root_depth; /* Actual port root-mutex depth for this OS thread. */
static __thread int fail_next; /* Native allocation refusal only in serial controller test. */
void __real_fvm_port_sync_lock(FVM_PortSync *mutex);
void __real_fvm_port_sync_unlock(FVM_PortSync *mutex);
void *__real_malloc(size_t bytes);


/* Write without allocating or invoking cleanup while a negative lock is held.
 * message is borrowed; terminate at exact semantic10 before unsafe continuation. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        ssize_t result;
        const char *prefix = "FAIL native root registry: ";
        result = write(2, prefix, strlen(prefix));
        (void)result;
        result = write(2, message, strlen(message));
        (void)result;
        result = write(2, "\n", 1);
        (void)result;
        _Exit(10);
    }
}


/* Track actual root port locks, not the independent test start barrier. */
void __wrap_fvm_port_sync_lock(FVM_PortSync *mutex)
{
    __real_fvm_port_sync_lock(mutex);
    if (mutex != gate)
    {
        root_depth++;
    }
}


/* Clear diagnostic state before the real unlock; no table work follows it. */
void __wrap_fvm_port_sync_unlock(FVM_PortSync *mutex)
{
    if (mutex != gate)
    {
        check(root_depth == 1, "balanced root mutex");
        root_depth--;
    }
    __real_fvm_port_sync_unlock(mutex);
}


/* Root metadata allocation must occur outside its mutex. Refusal happens before
 * allocation, publication or any referent access. No signal is a control. */
void *__wrap_malloc(size_t bytes)
{
    check(!root_depth, "allocation under root mutex");
    if (fail_next)
    {
        fail_next = 0;
        return NULL;
    }
    return __real_malloc(bytes);
}


/* Bounded nonallocating traversal oracle; counts are controller-private. */
static void visit(void *value, void *argument)
{
    Key *key = value;
    unsigned int *counts = argument;
    check(key && key->index < WORKERS, "visitor key lifetime");
    counts[key->index]++;
    check(counts[key->index] <= 2, "visitor duplicate bound");
}


/* Exercise duplicate registration/removal and native allocation failure in a
 * fresh serial process when requested. key outlives every registry occurrence. */
static void serial(int fresh)
{
    Key key = {0};
    check(dvm_runtime_root_add(&key) && dvm_runtime_root_count(&key) == 1,
        "initial registration");
    if (fresh)
    {
        fail_next = 1;
        check(!dvm_runtime_root_add(&key) && !fail_next && dvm_runtime_root_count(&key) == 1,
            "allocation refusal preserves existing registration");
    }
    check(dvm_runtime_root_add(&key) && dvm_runtime_root_count(&key) == 2,
        "duplicate registration");
    check(dvm_runtime_root_remove(&key) && dvm_runtime_root_count(&key) == 1,
        "duplicate removal preserves other occurrence");
    check(dvm_runtime_root_remove(&key) && !dvm_runtime_root_count(&key) &&
        !dvm_runtime_root_remove(&key), "last removal and absent refusal");
}


/* Race first initialization then real table/free-list reuse. No VM or managed
 * pointer enters worker. Every registration is removed before key lifetime ends. */
static void writer(void *argument)
{
    Writer *owner = argument;
    Work *work = owner->work;
    int index;
    fvm_port_sync_lock(gate);
    work->ready++;
    fvm_port_sync_signal_all(gate);
    while (work->phase < 1)
    {
        fvm_port_sync_wait(gate);
    }
    fvm_port_sync_unlock(gate);
    check(dvm_runtime_root_add(owner->key), "concurrent initial add");
    fvm_port_sync_lock(gate);
    work->initialized++;
    fvm_port_sync_signal_all(gate);
    while (work->phase < 2)
    {
        fvm_port_sync_wait(gate);
    }
    fvm_port_sync_unlock(gate);
    for (index = 0; index < PAIRS; index++)
    {
        check(dvm_runtime_root_remove(owner->key), "worker removes occurrence");
        check(dvm_runtime_root_add(owner->key), "worker republishes occurrence");
    }
    check(dvm_runtime_root_remove(owner->key), "worker final removal");
    check(!root_depth, "worker leaves root mutex");
}


/* argv --serial-only reserves deterministic pre-thread semantic controls.
 * Normal run tests a cold registry, all joins and exact final registration counts. */
int main(int argc, char **argv)
{
    Work work = {0};
    Writer writers[WORKERS];
    FVM_PortThread *threads[WORKERS];
    unsigned int counts[WORKERS];
    int index;
    int scan;
    check(!dvm_runtime_root_add(NULL) && !dvm_runtime_root_remove(NULL) &&
        !dvm_runtime_root_count(NULL), "null operations refuse unchanged");
    if (argc > 1 && !strcmp(argv[1], "--serial-only"))
    {
        serial(1);
        puts("PASS native root registry: duplicate/refusal/allocation rollback");
        return 0;
    }
    gate = fvm_port_sync_create();
    check(gate != NULL, "start gate allocation");
    for (index = 0; index < WORKERS; index++)
    {
        work.keys[index].index = (unsigned int)index;
        writers[index].work = &work;
        writers[index].key = &work.keys[index];
        threads[index] = fvm_port_thread_start(writer, &writers[index]);
        check(threads[index] != NULL, "native writer starts");
    }
    fvm_port_sync_lock(gate);
    while (work.ready != WORKERS)
    {
        fvm_port_sync_wait(gate);
    }
    work.phase = 1;
    fvm_port_sync_signal_all(gate);
    while (work.initialized != WORKERS)
    {
        fvm_port_sync_wait(gate);
    }
    for (index = 0; index < WORKERS; index++)
    {
        check(dvm_runtime_root_count(&work.keys[index]) == 1, "cold initial root count");
        check(dvm_runtime_root_add(&work.keys[index]), "parent base registration");
    }
    work.phase = 2;
    fvm_port_sync_signal_all(gate);
    fvm_port_sync_unlock(gate);
    for (scan = 0; scan < 1000; scan++)
    {
        memset(counts, 0, sizeof(counts));
        dvm_runtime_roots_visit(visit, counts);
        for (index = 0; index < WORKERS; index++)
        {
            check(counts[index] >= 1 && counts[index] <= 2, "coherent locked traversal");
        }
    }
    for (index = 0; index < WORKERS; index++)
    {
        fvm_port_thread_join(threads[index]);
        check(dvm_runtime_root_count(&work.keys[index]) == 1 &&
            dvm_runtime_root_remove(&work.keys[index]) && !dvm_runtime_root_count(&work.keys[index]),
            "joined final registration removal");
    }
    serial(0);
    fvm_port_sync_destroy(gate);
    check(!root_depth, "controller leaves root mutex");
    puts("PASS native root registry: cold init, 8 OS writers, 16000 pairs, 1000 traversals, complete joins");
    return 0;
}
