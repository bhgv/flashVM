/* Actual TLS/world roles; sentinel boundaries never emulate a managed heap. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "managed_owner.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>

int mutator; /* Retained Setmark link boundary; no marking is admitted here. */
int nprop; /* Retained Setmark accounting, never used by positive role checks. */
static void *probe; /* Native-backed header sentinel, not a managed allocation. */


/* Fail a role/lifecycle predicate. valid/text borrowed; explicit status10 only
 * for reached forbidden managed callbacks, not allocator/signal/timeouts. */
static void check(int valid, const char *text)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL managed owner role: %s\n", text);
        exit(1);
    }
}


/* Any root access from a rejected role is the semantic control marker. No root
 * table or collector is emulated. heap is an unused native sentinel pointer. */
void ptradd(Heap *heap)
{
    (void)heap;
    fputs("managed owner role reached forbidden root/payload operation\n", stderr);
    exit(10);
}


/* Same refusal boundary for unexpected root removal. heap is never accessed. */
void ptrdel(Heap *heap)
{
    ptradd(heap);
}


/* No positive role case owns a managed referent. value is not dereferenced. */
void destroy(void *value)
{
    (void)value;
    ptradd(NULL);
}


/* No callback is reached in valid refusal runs. callback/argument are borrowed;
 * this sentinel fixture does NOT claim actual native exception recovery. */
int kprotect(int (*callback)(void *), void *argument)
{
    return callback(argument);
}


/* Collector factory owns native context only; user unused. No managed roots. */
static DVM_NativeContext *create_context(void *user)
{
    (void)user;
    return dvm_native_process_create(NULL);
}


/* Native empty-root visit for actual world coordination. Arguments borrowed;
 * no heap scan is attempted. Full runtime probe tests actual root retention. */
static void roots(DVM_NativeContext *context, void *registers, size_t size, void *process, void *user)
{
    (void)context;
    (void)registers;
    (void)size;
    (void)process;
    (void)user;
}


/* No GC work is requested by this fixture. Arguments borrowed; no managed
 * allocator/collector is simulated. Return DONE for the native worker contract. */
static DVM_GCWorkResult empty_pass(DVM_NativeContext *context, size_t budget, void *user)
{
    (void)context;
    (void)budget;
    (void)user;
    return DVM_GC_DONE;
}


/* Exercise true collector role with actual stop ownership. argument borrows
 * world through join; rejected capture may not touch root/payload callbacks. */
static void collector(void *argument)
{
    DVM_NativeWorld *world = argument;
    DVM_NativeContext *context = dvm_native_context_create(sizeof(REG), NULL);
    DVM_OwnerTransfer transfer = {0};
    check(context && dvm_native_context_bind(context), "collector bind");
    check(dvm_world_stop_begin(world, context) && dvm_world_collector_owns(world), "collector stop ownership");
    check(!dvm_managed_owner_capture(probe, &transfer), "collector capture refused");
    check(dvm_world_stop_end(world) && !dvm_world_collector_owns(world), "collector stop ended");
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "collector retirement");
}


/* Register-only contexts have no real hosted error stack/environment. Even if
 * admitted ACTIVE to this world, they must refuse capture before kprotect/heap
 * access. argument world stays alive through parent join. */
static void untyped_owner(void *argument)
{
    DVM_NativeWorld *world = argument;
    DVM_NativeContext *context = dvm_native_context_create(sizeof(REG), NULL);
    DVM_HeapOwner *owner;
    DVM_OwnerTransfer transfer = {0};
    check(context && dvm_native_context_bind(context), "untyped bind");
    owner = dvm_native_owner_current();
    check(dvm_native_owner_attach(owner, world) && dvm_native_owner_enter(owner), "untyped active admission");
    check(!dvm_managed_owner_capture(probe, &transfer), "untyped ACTIVE capture refused");
    check(dvm_native_owner_leave(owner) && dvm_native_owner_detach(owner), "untyped detach");
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "untyped retirement");
}


/* Use actual owner/world transitions and collector threads. No admitted capture
 * or real heap is attempted. Full runtime managed_owner probe covers those. */
int main(void)
{
    Heap *backing = calloc(1, sizeof(Heap) + 32);
    DVM_NativeContext *context = dvm_native_process_create(NULL);
    DVM_NativeWorld *world = dvm_world_create_serial(0);
    DVM_NativeWorld *foreign = dvm_world_create_serial(0);
    DVM_GCWorkersConfig config = {0};
    DVM_GCWorkers *workers;
    DVM_OwnerTransfer transfer = {0};
    DVM_HeapOwner *owner;
    FVM_PortThread *thread;
    check(backing && context && world && foreign, "native setup");
    backing->ref = 1;
    probe = H2D(void *, backing);
    check(dvm_native_context_bind(context), "controller bind");
    up->env = &up->defenv;
    up->env->errstr = up->env->errbuf1;
    owner = dvm_native_owner_current();
    check(!dvm_managed_owner_capture(probe, &transfer), "detached refused");
    check(dvm_native_owner_attach(owner, world), "attach controller");
    config.world = world;
    config.create_context = create_context;
    config.roots = roots;
    config.Fast.run = empty_pass;
    config.Fast.budget = 1;
    config.Cycles.run = empty_pass;
    config.Cycles.budget = 1;
    workers = dvm_gc_workers_create(&config);
    check(workers && dvm_native_owner_enter(owner), "collectors/controller active");
    check(dvm_managed_owner_start(world, workers), "start managed owner service");
    check(!dvm_world_collector_owns(world), "active mutator is not collector");
    check(dvm_native_owner_leave(owner), "idle transition");
    check(!dvm_managed_owner_capture(probe, &transfer), "idle refused");
    thread = fvm_port_thread_start(untyped_owner, world);
    check(thread != NULL, "untyped owner creation");
    fvm_port_thread_join(thread);
    check(dvm_native_owner_enter(owner) && dvm_native_owner_suspend(owner, 7), "native transition");
    check(!dvm_managed_owner_capture(probe, &transfer), "native refused");
    check(dvm_native_owner_resume(owner), "native resume");
    check(!dvm_managed_owner_capture(probe, &transfer), "unacknowledged reentry refused");
    check(dvm_native_owner_progress(owner, 7) && dvm_native_owner_leave(owner), "acknowledged leave");
    thread = fvm_port_thread_start(collector, world);
    check(thread != NULL, "collector creation");
    fvm_port_thread_join(thread);
    check(dvm_native_owner_detach(owner), "detach own world");
    check(dvm_native_owner_attach(owner, foreign) && dvm_native_owner_enter(owner), "foreign active");
    check(!dvm_managed_owner_capture(probe, &transfer), "foreign refused");
    check(dvm_native_owner_leave(owner) && dvm_native_owner_detach(owner), "foreign detach");
    check(dvm_native_owner_attach(owner, world) && dvm_native_owner_enter(owner), "restore own world");
    check(dvm_managed_owner_close(), "close empty service");
    check(!dvm_managed_owner_capture(probe, &transfer), "closed service refused");
    check(dvm_native_owner_leave(owner), "leave before collector joins");
    check(dvm_gc_workers_wait_idle(workers) && dvm_managed_owner_finish(0), "finish empty service");
    check(dvm_gc_workers_destroy(workers), "collectors join");
    check(dvm_native_owner_detach(owner), "controller detach");
    check(dvm_world_destroy(world) && dvm_world_destroy(foreign), "world retirement");
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "context retirement");
    free(backing);
    puts("PASS managed owner roles: idle/native/pending/collector/foreign/closed refuse before root access");
    return 0;
}
