/* Native sidecar identity, lifetime and contention; no managed payload access. */
#include "disvm/guard_registry.h"
#include "disvm/native_context.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>

void dvm_test_guard_registry_faults(void); /* Separate native allocation algorithm fixture. */

#define THREADS 8 /* Independent native TLS owners, all joined before teardown. */
#define ROUNDS 2000 /* Each writer alternates between two canonical keys. */


/* Native payload/key storage stays alive throughout every lease and join. */
typedef struct Object
{
    int value; /* Guard-protected native counter. */
    int shadow; /* Paired invariant, protected by the SAME canonical guard. */
    unsigned char padding[504]; /* 512-byte stride forces both keys into one hash bucket. */
} Object;


/* Shared startup gates make all first-capture leases overlap before work. */
typedef struct Fixture
{
    DVM_GuardRegistry *registry; /* Caller-owned service outlives every worker. */
    FVM_PortSync *sync; /* Native start/arrival gate, not held for guard work. */
    Object objects[2]; /* Distinct caller-owned keys, never treated as Heap headers. */
    int start; /* Gate-protected simultaneous first-capture release. */
    int ready; /* Gate-protected number of workers holding both leases. */
    int go; /* Gate-protected permission to begin payload writes. */
    DVM_ObjectGuard *guards[THREADS][2]; /* Each worker publishes its own row before ready. */
} Fixture;


/* Private worker record remains live through its native thread join. */
typedef struct Worker
{
    Fixture *fixture; /* Shared fixture borrowed through join. */
    int index; /* Disjoint guard-result row. */
} Worker;


/* Semantic10 before unsafe continuation; signals/timeouts are failures. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL guard registry: %s\n", message);
        exit(10);
    }
}


/* Bind bare native TLS only; no managed admission, root or heap access. */
static DVM_NativeContext *bind(void)
{
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    check(context && dvm_native_context_bind(context), "native context");
    return context;
}


/* Retire only current unguarded context after every borrowed guard is released. */
static void unbind(DVM_NativeContext *context)
{
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "context retirement");
}


/* Acquire two caller-protected native identities concurrently, then keep leases
 * alive until guarded writes finish. Parking holds no guards or registry locks. */
static void write_objects(void *argument)
{
    Worker *worker = argument;
    Fixture *fixture = worker->fixture;
    DVM_NativeContext *context = bind();
    DVM_GuardLease *leases[2];
    int index;
    fvm_port_sync_lock(fixture->sync);
    while (!fixture->start)
    {
        fvm_port_sync_wait(fixture->sync);
    }
    fvm_port_sync_unlock(fixture->sync);
    for (index = 0; index < 2; index++)
    {
        leases[index] = dvm_guard_registry_acquire(fixture->registry, &fixture->objects[index]);
        check(leases[index] != NULL, "worker capture");
        fixture->guards[worker->index][index] = dvm_guard_registry_guard(leases[index]);
    }
    fvm_port_sync_lock(fixture->sync);
    fixture->ready++;
    fvm_port_sync_signal_all(fixture->sync);
    while (!fixture->go)
    {
        fvm_port_sync_wait(fixture->sync);
    }
    fvm_port_sync_unlock(fixture->sync);
    for (index = 0; index < ROUNDS; index++)
    {
        int which = index % 2;
        DVM_ObjectGuard *guard = fixture->guards[worker->index][which];
        DVM_ObjectResult result;
        while ((result = dvm_object_guard_try_enter(guard)) == DVM_OBJECT_BUSY)
        {
            check(dvm_object_guard_wait_available(guard) == DVM_OBJECT_AVAILABLE, "native parking");
        }
        check(result == DVM_OBJECT_ACQUIRED, "worker claims canonical guard");
        check(fixture->objects[which].value == fixture->objects[which].shadow, "paired payload before write");
        fixture->objects[which].value++;
        fixture->objects[which].shadow++;
        check(dvm_object_guard_leave(guard), "worker releases guard");
    }
    for (index = 0; index < 2; index++)
    {
        check(dvm_guard_registry_release(&leases[index]) && !leases[index], "worker lease retirement");
    }
    unbind(context);
}


/* Serial source-control predicates precede threads; aliases are not touched
 * after a failed lifetime predicate. Native objects outlive all registry users. */
int main(void)
{
    Fixture fixture = {0};
    Worker workers[THREADS];
    FVM_PortThread *threads[THREADS];
    DVM_NativeContext *context;
    DVM_GuardLease *first;
    DVM_GuardLease *alias;
    DVM_GuardLease *other;
    DVM_ObjectGuard *guard;
    int index;
    dvm_test_guard_registry_faults();
    context = bind();
    fixture.registry = dvm_guard_registry_create();
    fixture.sync = fvm_port_sync_create();
    check(fixture.registry && fixture.sync, "registry creation");
    check(!dvm_guard_registry_acquire(NULL, &fixture) &&
        !dvm_guard_registry_acquire(fixture.registry, NULL), "null identity refusal");
    first = dvm_guard_registry_acquire(fixture.registry, &fixture.objects[0]);
    alias = dvm_guard_registry_acquire(fixture.registry, &fixture.objects[0]);
    other = dvm_guard_registry_acquire(fixture.registry, &fixture.objects[1]);
    check(first && alias && other, "serial captures");
    guard = dvm_guard_registry_guard(first);
    check(guard == dvm_guard_registry_guard(alias), "same identity shares guard");
    check(guard != dvm_guard_registry_guard(other), "distinct identity has distinct guard");
    check(dvm_guard_registry_entries(fixture.registry) == 2 &&
        !dvm_guard_registry_destroy(&fixture.registry) && fixture.registry, "populated destroy refusal");
    check(dvm_object_guard_try_enter(guard) == DVM_OBJECT_ACQUIRED, "serial guard claim");
    check(!dvm_guard_registry_acquire(fixture.registry, &fixture.objects[1]) &&
        !dvm_guard_registry_release(&first) && first, "held guard forbids lease changes");
    check(dvm_object_guard_leave(guard), "serial guard release");
    check(dvm_guard_registry_release(&first) && !first &&
        dvm_guard_registry_entries(fixture.registry) == 2, "alias retains canonical node");
    check(dvm_guard_registry_guard(alias) == guard, "surviving alias guard stable");
    check(dvm_guard_registry_release(&alias) && dvm_guard_registry_release(&other) &&
        !dvm_guard_registry_entries(fixture.registry), "serial retirement empties registry");
    for (index = 0; index < THREADS; index++)
    {
        workers[index].fixture = &fixture;
        workers[index].index = index;
        threads[index] = fvm_port_thread_start(write_objects, &workers[index]);
        check(threads[index] != NULL, "worker creation");
    }
    fvm_port_sync_lock(fixture.sync);
    fixture.start = 1;
    fvm_port_sync_signal_all(fixture.sync);
    while (fixture.ready != THREADS)
    {
        fvm_port_sync_wait(fixture.sync);
    }
    check(dvm_guard_registry_entries(fixture.registry) == 2, "concurrent captures canonicalized");
    for (index = 1; index < THREADS; index++)
    {
        check(fixture.guards[index][0] == fixture.guards[0][0] &&
            fixture.guards[index][1] == fixture.guards[0][1], "concurrent guard identity");
    }
    fixture.go = 1;
    fvm_port_sync_signal_all(fixture.sync);
    fvm_port_sync_unlock(fixture.sync);
    for (index = 0; index < THREADS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    for (index = 0; index < 2; index++)
    {
        check(fixture.objects[index].value == THREADS * ROUNDS / 2 &&
            fixture.objects[index].shadow == fixture.objects[index].value, "joined payload exact");
    }
    check(!dvm_guard_registry_entries(fixture.registry) &&
        dvm_guard_registry_destroy(&fixture.registry) && !fixture.registry, "joined registry retirement");
    fvm_port_sync_destroy(fixture.sync);
    unbind(context);
    puts("PASS guard registry:8 OS writers,16000 guarded native writes, canonical identity, complete retirement");
    return 0;
}
