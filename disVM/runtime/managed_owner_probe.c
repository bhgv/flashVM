/* Actual retained heap/root/collector ownership across independent OS contexts. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "pool.h"
#include "managed_owner.h"
#include "managed_owner_probe.h"
#include "type_counts.h"
#include "heap_counts.h"
#include "module_counts.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include "disvm/owner_slot.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROBE_THREADS 8 /* Native ticket consumers, not managed executor quota. */
#define PROBE_CYCLES 32 /* Actual retained cycle passes while only ticket roots hold source. */

int dvm_managed_owner_probe_requested; /* Startup-only option, set before runtime launch. */


/* Native synchronization/transfer storage outlives all receiver threads. */
typedef struct Probe
{
    FVM_PortSync *sync; /* Protect ready/go, never held across VM/managed access. */
    DVM_OwnerSlot *slot; /* One shared native owner; receiver copies happen under its mutex. */
    int ready; /* All accepted owners, protected by sync. */
    int go; /* Release receivers to close scopes, protected by sync. */
} Probe;


/* One child borrows stable parent-owned probe until join. */
typedef struct Receiver
{
    Probe *probe; /* Native synchronization/transfer owner. */
} Receiver;


/* Fatal semantic diagnostic. valid/message borrowed; never treat a compiler,
 * signal or timeout as proof. status10 is reserved for explicit predicates. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL managed owner probe: %s\n", message);
        exit(10);
    }
}


/* Receive only native ticket ownership. Bare context cannot borrow its managed
 * referent. Explicit cleanup queues retirement; this thread never calls destroy
 * or ptrdel, even if it is the final native owner. argument lives until join. */
static void receive(void *argument)
{
    Receiver *receiver = argument;
    Probe *probe = receiver->probe;
    DVM_NativeContext *context = dvm_native_process_create(NULL);
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry entry = {0};
    check(context && dvm_native_context_bind(context), "receiver context");
    check(dvm_owner_scope_open(&scope) &&
        dvm_owner_slot_copy(probe->slot, &scope, &entry), "receiver accepts owner");
    check(!dvm_managed_owner_value(&entry), "bare native receiver refuses managed payload");
    fvm_port_sync_lock(probe->sync);
    probe->ready++;
    fvm_port_sync_signal_all(probe->sync);
    while (!probe->go)
    {
        fvm_port_sync_wait(probe->sync);
    }
    fvm_port_sync_unlock(probe->sync);
    check(dvm_owner_scope_close(&scope), "receiver closes owner");
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "receiver retires");
}


/* Borrowed source is protected by an older ticket scope until this newer native
 * record unlinks. No payload access is allowed during cleanup, even with ACTIVE
 * world admission. All storage stays live until the pending longjmp. */
typedef struct UnwindProbe
{
    DVM_NativeCleanup record; /* Newer than source scope, so dispatched first. */
    DVM_OwnerEntry *entry; /* Borrowed older native owner, still live in callback. */
    void *value; /* Borrowed managed pointer, never dereferenced in callback. */
} UnwindProbe;


/* Refused capture/value must not enter VM or touch managed payload while cleanup
 * runs. argument borrows still-live C frame; unlink before normal callback return. */
static int refuse_during_unwind(void *argument)
{
    UnwindProbe *probe = argument;
    DVM_OwnerTransfer transfer = {0};
    check(!dvm_managed_owner_capture(probe->value, &transfer), "cleanup capture refuses before VM entry");
    check(!dvm_managed_owner_value(probe->entry), "cleanup managed borrow refuses");
    return dvm_native_cleanup_unlink(&probe->record);
}


/* [GC:] Real native error path owns a rooted managed String exclusively through
 * its scope ticket before raising. The hosted checkpoint closes the scope BEFORE
 * longjmp; ticket drop queues Fast and performs no managed destructor itself. */
static int error_scope(void *argument)
{
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry entry = {0};
    DVM_OwnerTransfer transfer = {0};
    String *value = c2string("managed-error-root", 18);
    UnwindProbe probe = {0};
    DVM_OwnerSlot *slot = dvm_owner_slot_create();
    (void)argument;
    check(dvm_owner_scope_open(&scope), "error scope open");
    check(dvm_managed_owner_capture(value, &transfer) &&
        dvm_owner_scope_accept(&scope, &entry, &transfer), "error scope rooted owner");
    check(slot && dvm_owner_slot_put(slot, &entry) &&
        dvm_owner_slot_take(slot, &scope, &entry) && dvm_owner_slot_destroy(&slot),
        "error owner moved into tracked scope before throw");
    probe.entry = &entry;
    probe.value = value;
    check(dvm_native_cleanup_link(&probe.record, refuse_during_unwind, &probe), "register cleanup role probe");
    destroy(value);
    value = H;
    error("managed owner cleanup probe");
    return 1;
}


/* [GC:] Only ticket roots hold the created managed String during actual cycle
 * collection. Controller drops its ordinary creation owner, leaves ACTIVE world
 * admission, waits collectors, then borrows through its still-owned ticket only
 * after reentry. Every native child is joined before probe storage retires. */
void dvm_managed_owner_probe_run(DVM_NativeWorld *world, DVM_GCWorkers *workers)
{
    Probe probe = {0};
    Receiver receivers[PROBE_THREADS];
    FVM_PortThread *threads[PROBE_THREADS];
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry main_entry = {0};
    DVM_OwnerEntry outgoing = {0};
    DVM_OwnerTransfer transfer = {0};
    DVM_HeapOwner *owner = dvm_native_owner_current();
    String *value;
    int index;
    char previous[ERRMAX];
    dvm_runtime_type_probe(workers);
    dvm_runtime_heap_probe(workers);
    dvm_runtime_opcode_probe(workers);
    dvm_runtime_module_probe(workers);
    value = c2string("managed-ticket-root", 19);
    check(world && workers && value != H && value != nil, "real managed String");
    probe.slot = dvm_owner_slot_create();
    check(probe.slot != NULL, "managed publication slot");
    probe.sync = fvm_port_sync_create();
    check(probe.sync && dvm_owner_scope_open(&scope), "controller scope");
    check(dvm_managed_owner_capture(value, &transfer), "capture managed owner/root");
    check(dvm_owner_scope_accept(&scope, &main_entry, &transfer), "controller accepts ticket");
    destroy(value);
    value = H;
    check(!dvm_managed_owner_can_close() && !dvm_managed_owner_close(), "live owners refuse shutdown");
    check(dvm_owner_scope_retain(&scope, &outgoing, main_entry.object, main_entry.ops), "distinct shared slot owner");
    check(dvm_owner_slot_put(probe.slot, &outgoing) && !outgoing.object,
        "publish rooted ticket directly from scope");
    for (index = 0; index < PROBE_THREADS; index++)
    {
        receivers[index].probe = &probe;
        threads[index] = fvm_port_thread_start(receive, &receivers[index]);
        check(threads[index] != NULL, "receiver creation");
    }
    fvm_port_sync_lock(probe.sync);
    while (probe.ready != PROBE_THREADS)
    {
        fvm_port_sync_wait(probe.sync);
    }
    fvm_port_sync_unlock(probe.sync);
    check(dvm_owner_slot_take(probe.slot, &scope, &outgoing) &&
        dvm_owner_scope_drop(&outgoing), "unpublish slot directly into scope after receiver copies");
    check(dvm_native_owner_leave(owner), "controller leaves before collection");
    check(!dvm_managed_owner_value(&main_entry), "IDLE refuses managed payload");
    for (index = 0; index < PROBE_CYCLES; index++)
    {
        check(dvm_gc_workers_request(workers, DVM_GC_CYCLES) &&
            dvm_gc_workers_wait_idle(workers), "real cycle completion");
    }
    check(dvm_native_owner_enter(owner), "controller reentry");
    value = dvm_managed_owner_value(&main_entry);
    check(value && value != H, "ACTIVE ticket borrow");
    {
        /* Runtime arena remains allocated throughout: inspect its native block
         * header before managed payload, so a missing-root control fails by an
         * explicit predicate rather than reading a reclaimed String. */
        Bhdr *block = (Bhdr *)((uchar *)D2H(value) - offsetof(Bhdr, u.data));
        check(block->magic == MAGIC_A, "managed root block reclaimed during cycles");
    }
    check(value && value != H && !strcmp(string2c(value), "managed-ticket-root"), "root survives actual cycle collection");
    value = H;
    check(dvm_owner_scope_close(&scope), "controller releases ticket owner");
    check(dvm_native_owner_leave(owner), "controller leaves for native final drops");
    fvm_port_sync_lock(probe.sync);
    probe.go = 1;
    fvm_port_sync_signal_all(probe.sync);
    fvm_port_sync_unlock(probe.sync);
    for (index = 0; index < PROBE_THREADS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    check(dvm_gc_workers_wait_idle(workers), "Fast root/reference retirement completes");
    check(dvm_native_owner_enter(owner), "controller final reentry");
    check(dvm_managed_owner_can_close() && !dvm_managed_owner_pending(), "all owners/producers/queue retired");
    memcpy(previous, up->env->errstr, sizeof(previous));
    check(!kprotect(error_scope, NULL), "real managed scope error caught");
    check(!strcmp(up->env->errstr, "managed owner cleanup probe"), "error text retained");
    memcpy(up->env->errstr, previous, sizeof(previous));
    check(dvm_managed_owner_pending() == 1, "error cleanup queued root before managed drop");
    check(dvm_native_owner_leave(owner), "leave after native error cleanup");
    check(dvm_gc_workers_wait_idle(workers), "Fast retires managed error owner");
    check(dvm_native_owner_enter(owner), "reenter after error retirement");
    check(dvm_managed_owner_can_close() && !dvm_managed_owner_pending(), "error owner fully retired");
    check(dvm_owner_slot_destroy(&probe.slot), "joined empty managed slot");
    fvm_port_sync_destroy(probe.sync);
    puts("PASS managed owner probe: 8 native receivers, 32 real cycle passes, rooted until Fast, shared-slot snapshots, native error cleanup, complete joins");
}
