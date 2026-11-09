/* Copyright (c) 2026 bhgv
 *
 * EN: Test native GC role boundaries using real owners and a real world.
 * RU: Проверка границ ролей GC на реальных владельцах и домене кучи.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "heap_pool.h"
#include "gc_fields.h"
#include "managed_owner.h"
#include "runtime_gc.h"
#include "runtime_rc.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type Tarray;  /* Type identity only. This fixture owns no real arrays. */
Type Tstring; /* Type identity only. This fixture owns no real strings. */
Type Tchannel, Tlist, Tmodlink, Tptr;
int dvm_runtime_idle_cycles;
int gchalt;
void (*heapmonitor)(int, void *, ulong); /* Monitoring stays disabled. */
Pool *heapmem; /* Never used: rejected roles must not ask the allocator anything. */
int mutator;  /* Link retained marking declarations; marking is forbidden here. */
int nprop;    /* Link retained marking declarations; no mark may be published. */
static void *probe; /* Native-backed sentinel, never admitted as a managed object. */


/* EN: No managed-owner tickets exist in this role fixture.
 * RU: В этой проверке ролей нет заданий управляемого владения. */
unsigned long dvm_managed_owner_pending(void)
{
    return 0;
}


/* Fail a lifecycle check. valid/message are borrowed; no VM exception is forged. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL RC role: %s\n", message);
        exit(1);
    }
}


/* Deliberate semantic boundary for active removal controls. Rejected contexts
 * must never reach any payload allocator operation. No allocator is emulated. */
ulong poolmsize(Pool *pool, void *block)
{
    (void)pool;
    (void)block;
    fputs("RC role reached forbidden payload operation\n", stderr);
    exit(10);
}


/* The role fixture may never free a managed pool block. */
void poolfree(Pool *pool, void *value)
{
    (void)poolmsize(pool, value);
}


/* Unexpected reference mutation is a semantic refusal failure, not a mock RC. */
void ptradd(Heap *heap)
{
    (void)heap;
    fputs("RC role reached forbidden payload operation\n", stderr);
    exit(10);
}


/* No role-only test owns a queue reference to remove. heap is never borrowed. */
void ptrdel(Heap *heap)
{
    ptradd(heap);
}


/* No managed destructor may run in this fixture. value is an opaque sentinel. */
void destroy(void *value)
{
    (void)value;
    ptradd(NULL);
}


/* The fixture initializes service metadata but never opens a shared cell
 * domain or publishes managed objects. Fail loudly if either is attempted. */
DVM_CellDomain *dvm_managed_cell_domain(void)
{
    return NULL;
}

DVM_HeapKey dvm_heap_pool_key(Pool *pool, void *block)
{
    (void)poolmsize(pool, block);
    return (DVM_HeapKey){0};
}

int dvm_gc_field_mark(int *color, const int *selector, int *pending)
{
    (void)color;
    (void)selector;
    (void)pending;
    ptradd(NULL);
    return 0;
}

void freeheap(Heap *heap, int swept)
{
    (void)swept;
    ptradd(heap);
}

/* EN: The role fixture has no native buffer transfer for channel.
 * RU: В тесте ролей нет нативной передачи буфера для channel. */
int dvm_runtime_channel_buffer_busy(Channel *channel)
{
    (void)channel;
    return 0;
}


/* EN: The role fixture has no guest waiting on channel.
 * RU: В тесте ролей нет потока, ожидающего channel. */
int dvm_runtime_channel_post_close(Channel *channel)
{
    (void)channel;
    return 0;
}


int dvm_runtime_channel_is_quiet(Channel *channel)
{
    (void)channel;
    ptradd(NULL);
    return 0;
}

int dvm_runtime_channel_claim_quiet(Heap *heap, Array *buffer, Type **mapped, int already_claimed)
{
    (void)buffer;
    (void)mapped;
    (void)already_claimed;
    ptradd(heap);
    return 0;
}

void dvm_runtime_gc_owner_lost(void)
{
    ptradd(NULL);
}

/* A genuine stopped-world collector has no ACTIVE mutator admission. world is
 * borrowed through parent join; the collector context unregisters before exit. */
static void collector(void *argument)
{
    DVM_NativeWorld *world = argument;
    DVM_NativeContext *context = dvm_native_context_create(sizeof(REG), NULL);
    check(context && dvm_native_context_bind(context), "collector context");
    check(dvm_world_stop_begin(world, context), "collector stop");
    check(!dvm_runtime_rc_defer(probe) && !dvm_runtime_rc_pending(), "collector admitted");
    check(!dvm_runtime_rc_drain(1), "stopped collector can drain an empty queue");
    check(dvm_world_stop_end(world), "collector resume");
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "collector retire");
}


/* Use real serialized-world transitions for IDLE/NATIVE/pending/foreign and
 * collector callers. No admitted queue transfer is attempted; whole runtime
 * helper_rc tests cover allocation, references, GC and exact queue draining. */
int main(int argc, char **argv)
{
    Heap *backing = calloc(1, sizeof(Heap) + 32);
    DVM_NativeContext *context = dvm_native_context_create(sizeof(REG), NULL);
    DVM_NativeWorld *world = dvm_world_create_serial(0);
    DVM_NativeWorld *foreign = dvm_world_create_serial(0);
    DVM_HeapOwner *owner;
    FVM_PortThread *thread;
    check(backing && context && world && foreign, "native setup");
    backing->ref = 1;
    probe = H2D(void *, backing);
    check(context && dvm_native_context_bind(context), "bind");
    owner = dvm_native_owner_current();
    check(!dvm_runtime_rc_defer(probe), "detached admitted");
    check(dvm_native_owner_attach(owner, world) && dvm_native_owner_enter(owner), "initial admission");
    dvm_runtime_rc_start();
    if (argc == 2 && !strcmp(argv[1], "--invalid-drain"))
    {
        (void)dvm_runtime_rc_drain(1);
        check(0, "active mutator drained the retirement queue");
    }
    check(dvm_native_owner_leave(owner), "idle transition");
    check(!dvm_runtime_rc_defer(probe) && !dvm_runtime_rc_pending(), "idle admitted");
    check(dvm_native_owner_enter(owner) && dvm_native_owner_suspend(owner, 7), "native transition");
    check(!dvm_runtime_rc_defer(probe), "native admitted");
    check(dvm_native_owner_resume(owner), "native resume");
    check(!dvm_runtime_rc_defer(probe), "unacknowledged reentry admitted");
    check(dvm_native_owner_progress(owner, 7) && dvm_native_owner_leave(owner), "progress/leave");
    thread = fvm_port_thread_start(collector, world);
    check(thread != NULL, "collector start");
    fvm_port_thread_join(thread);
    check(dvm_native_owner_detach(owner), "world detach");
    check(dvm_native_owner_attach(owner, foreign) && dvm_native_owner_enter(owner), "foreign admission");
    check(!dvm_runtime_rc_defer(probe), "foreign world admitted");
    check(dvm_native_owner_leave(owner) && dvm_native_owner_detach(owner), "foreign leave");
    check(dvm_world_destroy(foreign), "foreign destroy");
    check(dvm_native_owner_attach(owner, world) && dvm_native_owner_enter(owner), "restore controller");
    dvm_runtime_rc_close();
    check(!dvm_runtime_rc_defer(probe), "closed queue admitted");
    check(dvm_runtime_rc_finish(0), "empty queue finish");
    check(dvm_native_owner_leave(owner) && dvm_native_owner_detach(owner), "final detach");
    check(dvm_world_destroy(world), "world destroy");
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "context destroy");
    free(backing);
    puts("PASS real RC roles: detached/idle/native/pending/collector/foreign/closed refuse payload access");
    return 0;
}
