/* Real native guards/TLS/world fences; managed heap mutation is not enabled. */
#include "disvm/object_guard.h"
#include "disvm/native_context.h"
#include "disvm/native_owner.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define THREADS 8 /* Actual OS owners; no logical scheduler is simulated. */
#define ROUNDS 2000 /* Per-thread writes split between two independent objects. */


/* Mutable native object kept alive by main through every joined borrower. */
typedef struct Object
{
    DVM_ObjectGuard guard; /* Initialized inline metadata, not a Heap ABI field. */
    int value;             /* Protected mutable payload. */
    int shadow;            /* Independent paired invariant detects unguarded updates. */
} Object;


/* Thread arguments/results live through join; fixture sync protects handshakes. */
typedef struct Fixture
{
    Object objects[2];       /* Distinct guards may be held simultaneously. */
    FVM_PortSync *sync;      /* Native test handshake, separate from guard stripes. */
    int independent;         /* Second owner acquired object1 while main held object0. */
    int returned;            /* Waiter observed availability before its try-enter retry. */
    int claimed;             /* Waiter eventually acquired after a real park. */
} Fixture;


/* Stable native worker argument, borrowed through its OS join. */
typedef struct Worker
{
    Fixture *fixture; /* Main retains object storage for the complete call. */
    int index;        /* Select one of the two mutable objects. */
} Worker;


/* Semantic status10 is required by isolated removal controls; signals/compiler
 * failures never count as successful guard detection. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL object guard predicate: %s\n", message);
        exit(10);
    }
}


/* Bind native TLS without registering managed roots. Main joins before freeing
 * any object argument, so raw native storage remains borrowed/live throughout. */
static DVM_NativeContext *bind_context(void)
{
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    check(context && dvm_native_context_bind(context), "context bind");
    return context;
}


/* Every real guard must be gone before TLS can retire; no hidden owner transfer. */
static void retire_context(DVM_NativeContext *context)
{
    check(!dvm_native_context_object_count(context), "guard accounting remained");
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "context retirement");
}


/* Observe foreign release refusal, independent-object ownership and a genuine
 * predicate park. Main's controlled native wait has NO managed world attached. */
static void waiter(void *argument)
{
    Fixture *fixture = argument;
    DVM_NativeContext *context = bind_context();
    check(dvm_object_guard_try_enter(&fixture->objects[1].guard) == DVM_OBJECT_ACQUIRED, "independent guard");
    check(!dvm_object_guard_leave(&fixture->objects[0].guard), "foreign release accepted");
    check(dvm_object_guard_leave(&fixture->objects[1].guard), "independent release");
    fvm_port_sync_lock(fixture->sync);
    fixture->independent = 1;
    fvm_port_sync_signal_all(fixture->sync);
    fvm_port_sync_unlock(fixture->sync);
    check(dvm_object_guard_try_enter(&fixture->objects[0].guard) == DVM_OBJECT_BUSY, "busy guard");
    check(!dvm_native_context_object_count(context), "failed try leaked accounting");
    check(dvm_object_guard_wait_available(&fixture->objects[0].guard) == DVM_OBJECT_AVAILABLE, "wait availability kind");
    check(!dvm_native_context_object_count(context), "wait claimed ownership");
    fvm_port_sync_lock(fixture->sync);
    fixture->returned = 1;
    fvm_port_sync_unlock(fixture->sync);
    for (;;)
    {
        DVM_ObjectResult result = dvm_object_guard_try_enter(&fixture->objects[0].guard);
        if (result == DVM_OBJECT_ACQUIRED)
        {
            break;
        }
        check(result == DVM_OBJECT_BUSY, "retry refused");
        check(dvm_object_guard_wait_available(&fixture->objects[0].guard) == DVM_OBJECT_AVAILABLE, "retry wait");
    }
    check(dvm_object_guard_leave(&fixture->objects[0].guard), "waiter release");
    fixture->claimed = 1; /* Parent reads only after this thread joins. */
    retire_context(context);
}


/* Execute protected plain read/write pairs from actual competing OS contexts.
 * Wait does not grant ownership; every wake retries the CAS before payload use. */
static void mutate(void *argument)
{
    Worker *worker = argument;
    Object *object = &worker->fixture->objects[worker->index % 2];
    DVM_NativeContext *context = bind_context();
    int index;
    for (index = 0; index < ROUNDS; index++)
    {
        for (;;)
        {
            DVM_ObjectResult result = dvm_object_guard_try_enter(&object->guard);
            if (result == DVM_OBJECT_ACQUIRED)
            {
                break;
            }
            check(result == DVM_OBJECT_BUSY, "mutator try refused");
            check(dvm_object_guard_wait_available(&object->guard) == DVM_OBJECT_AVAILABLE, "mutator park");
        }
        check(object->shadow == (object->value ^ 0x5a5a), "mutable pair torn");
        object->value++;
        object->shadow = object->value ^ 0x5a5a;
        check(dvm_object_guard_leave(&object->guard), "mutator unlock");
    }
    retire_context(context);
}


/* This hook is never installed while a guard is held; it models a custom native
 * safepoint registration only, not the runtime or a garbage collector. */
static int poll_hook(void *argument)
{
    (void)argument;
    return 1;
}


/* Refuse world/scheduler/TLS transitions while guard metadata is held. These
 * checks use actual NativeWorld/HeapOwner transitions, never fake field stores. */
static void fences(DVM_NativeContext *context, DVM_ObjectGuard *guard)
{
    DVM_NativeWorld *world = dvm_world_create_serial(0);
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    int token;
    check(world != NULL, "world creation");
    check(dvm_object_guard_try_enter(guard) == DVM_OBJECT_ACQUIRED, "fence bare acquisition");
    check(!dvm_native_context_poll(), "held guard reached poll");
    check(!dvm_native_context_unbind(context), "held guard unbound TLS");
    check(!dvm_native_context_acquire_turn(context, &token), "held guard entered scheduler");
    check(!dvm_native_context_set_poll(context, poll_hook, NULL), "held guard registered roots");
    check(!dvm_native_owner_attach(owner, world), "held guard joined world");
    check(!dvm_world_stop_begin(world, context), "held guard began collection");
    check(dvm_object_guard_leave(guard), "bare guard release");
    check(dvm_native_context_set_poll(context, poll_hook, NULL), "custom poll setup");
    check(dvm_object_guard_try_enter(guard) == DVM_OBJECT_REFUSED, "custom hook acquired guard");
    check(dvm_object_guard_wait_available(guard) == DVM_OBJECT_REFUSED, "custom hook parked");
    check(dvm_native_context_object_acquire(context), "negative hook guard setup");
    check(!dvm_native_context_clear_poll(context, poll_hook, NULL), "held guard removed roots");
    check(dvm_native_context_object_release(context) &&
        dvm_native_context_clear_poll(context, poll_hook, NULL), "custom poll cleanup");
    check(dvm_world_stop_begin(world, context), "collector role setup");
    check(dvm_object_guard_try_enter(guard) == DVM_OBJECT_REFUSED, "collector acquired guard");
    check(dvm_object_guard_wait_available(guard) == DVM_OBJECT_REFUSED, "collector parked");
    check(dvm_native_context_object_acquire(context), "negative collector guard setup");
    check(!dvm_world_stop_end(world), "held guard resumed mutators");
    check(dvm_native_context_object_release(context) && dvm_world_stop_end(world), "collector role cleanup");
    check(dvm_native_context_acquire_turn(context, &token), "turn setup");
    check(dvm_object_guard_try_enter(guard) == DVM_OBJECT_ACQUIRED, "turn guard");
    check(!dvm_native_context_release_turn(context, &token), "held guard abandoned turn");
    check(dvm_object_guard_leave(guard) && dvm_native_context_release_turn(context, &token), "turn cleanup");
    check(dvm_native_owner_attach(owner, world) && dvm_native_owner_enter(owner), "heap admission");
    check(dvm_object_guard_wait_available(guard) == DVM_OBJECT_REFUSED, "ACTIVE owner parked");
    check(dvm_object_guard_try_enter(guard) == DVM_OBJECT_ACQUIRED, "ACTIVE guard");
    check(dvm_object_guard_wait_available(guard) == DVM_OBJECT_REFUSED, "held guard parked");
    check(!dvm_native_context_poll(), "ACTIVE guard polled");
    check(!dvm_native_owner_leave(owner), "held guard left heap");
    check(!dvm_native_owner_suspend(owner, 5), "held guard entered native wait");
    check(dvm_native_owner_statistics(owner, &state) && state.phase == DVM_HEAP_ACTIVE && !state.releases,
        "refused transition changed ownership");
    check(dvm_object_guard_leave(guard) && dvm_native_owner_leave(owner), "world idle");
    check(dvm_object_guard_try_enter(guard) == DVM_OBJECT_REFUSED, "IDLE acquired guard");
    check(dvm_object_guard_wait_available(guard) == DVM_OBJECT_AVAILABLE, "IDLE availability");
    check(!dvm_native_context_object_count(context), "IDLE wait acquired guard");
    check(dvm_native_context_object_acquire(context), "negative recorded-guard setup");
    check(!dvm_native_owner_enter(owner), "recorded guard entered heap");
    check(dvm_native_context_object_release(context) && dvm_native_owner_enter(owner), "recorded guard cleanup");
    check(dvm_native_owner_suspend(owner, 5), "native transition");
    check(dvm_object_guard_try_enter(guard) == DVM_OBJECT_REFUSED, "NATIVE acquired guard");
    check(dvm_object_guard_wait_available(guard) == DVM_OBJECT_AVAILABLE, "NATIVE availability");
    check(dvm_native_owner_resume(owner), "native return");
    check(dvm_object_guard_try_enter(guard) == DVM_OBJECT_REFUSED, "pending reentry acquired guard");
    check(dvm_native_owner_progress(owner, 5), "native acknowledgement");
    check(dvm_native_owner_leave(owner) && dvm_native_owner_detach(owner), "world detach");
    check(dvm_world_destroy(world), "world destroy");
}


/* Exact ownership/parking and two-object contention. Borrowed native object
 * storage is permanent through each join; no managed Heap is mutated here. */
int main(void)
{
    DVM_ObjectDomain *domain = dvm_object_domain_create();
    DVM_NativeContext *context = bind_context();
    Fixture fixture;
    Worker workers[THREADS];
    FVM_PortThread *threads[THREADS];
    FVM_PortThread *waiting;
    int index;
    memset(&fixture, 0, sizeof(fixture));
    fixture.sync = fvm_port_sync_create();
    check(domain && fixture.sync, "native resources");
    for (index = 0; index < 2; index++)
    {
        check(dvm_object_guard_init(&fixture.objects[index].guard, domain), "guard init");
        fixture.objects[index].shadow = 0x5a5a;
    }
    check(!dvm_object_domain_destroy(domain), "domain retired initialized guards");
    check(!dvm_object_guard_init(&fixture.objects[0].guard, domain), "double init");
    check(dvm_object_guard_wait_available(&fixture.objects[0].guard) == DVM_OBJECT_AVAILABLE, "early wake predicate");
    check(!dvm_native_context_object_count(context), "availability took lock");
    check(dvm_object_guard_try_enter(&fixture.objects[0].guard) == DVM_OBJECT_ACQUIRED, "main guard");
    check(dvm_object_guard_try_enter(&fixture.objects[0].guard) == DVM_OBJECT_REFUSED, "recursive guard");
    check(dvm_native_context_object_count(context) == 1, "recursive accounting");
    check(dvm_object_guard_try_enter(&fixture.objects[1].guard) == DVM_OBJECT_ACQUIRED, "nested independent guard");
    check(dvm_native_context_object_count(context) == 2, "nested accounting");
    check(dvm_object_guard_leave(&fixture.objects[1].guard), "nested release");
    check(!dvm_object_guard_fini(&fixture.objects[0].guard), "held guard finalized");
    waiting = fvm_port_thread_start(waiter, &fixture);
    check(waiting != NULL, "waiter start");
    /* Private no-GC test handshake; not a native lease transition or VM callback. */
    while (!dvm_object_guard_waiters(&fixture.objects[0].guard))
    {
        fvm_port_thread_sleep_us(50);
    }
    fvm_port_sync_lock(fixture.sync);
    check(fixture.independent && !fixture.returned, "park/independent ownership");
    fvm_port_sync_unlock(fixture.sync);
    check(dvm_object_guard_leave(&fixture.objects[0].guard), "wake release");
    fvm_port_thread_join(waiting);
    check(fixture.returned && fixture.claimed, "waiter retry completion");
    check(!dvm_object_guard_leave(&fixture.objects[0].guard), "duplicate unlock");
    fences(context, &fixture.objects[0].guard);
    for (index = 0; index < THREADS; index++)
    {
        workers[index].fixture = &fixture;
        workers[index].index = index;
        threads[index] = fvm_port_thread_start(mutate, &workers[index]);
        check(threads[index] != NULL, "mutator start");
    }
    for (index = 0; index < THREADS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    check(dvm_native_context_unbind(context), "unbound-role setup");
    check(dvm_object_guard_try_enter(&fixture.objects[0].guard) == DVM_OBJECT_REFUSED, "unbound acquisition");
    check(dvm_object_guard_wait_available(&fixture.objects[0].guard) == DVM_OBJECT_REFUSED, "unbound park");
    check(dvm_native_context_bind(context), "unbound-role cleanup");
    for (index = 0; index < 2; index++)
    {
        check(fixture.objects[index].value == THREADS / 2 * ROUNDS, "lost guarded writes");
        check(!dvm_object_guard_waiters(&fixture.objects[index].guard), "waiters stranded");
        check(dvm_object_guard_fini(&fixture.objects[index].guard), "guard fini");
    }
    check(dvm_object_domain_destroy(domain), "domain final retirement");
    fvm_port_sync_destroy(fixture.sync);
    retire_context(context);
    printf("PASS native object guards: %s, 8 OS owners, two independent objects, actual park/wake, TLS/world fences\n",
        dvm_object_guard_backend());
    return 0;
}
