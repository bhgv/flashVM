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
#include "heap_snapshot.h"
#include "service_counts.h"
#include "pointer_builder.h"
#include "root_registry.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include "disvm/owner_slot.h"
#include "thread_port.h"
#include "channel_sync.h"
#include "vfs/managed_value.h"
#include "isa.h"
#include "vfs/scalar_direct.h"
#include "../native/terminal/call_record.h"
#include "../native/terminal/scalar_record.h"
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


/* [GC:] EN: Decode borrowed wire bytes, retain local values, and collect with only ticket roots.
 * RU: Декодировать записи, удержать локальные значения и запустить GC с корнями билетов. */
static void styx_value_probe(DVM_HeapOwner *owner, DVM_GCWorkers *workers)
{
    size_t words;
    unsigned char pointers;
    check(dvm_styx_frame_layout("(ys(a[b]y)r[ib])", &words, &pointers) &&
        words == 5 && pointers == 0x68, "derive exact argument reference bits for mixed Styx frame");
    const unsigned char utf8[] = {0xd0, 0x96, 0, 0x61};
    unsigned char encoded[64];
    size_t first, second;
    void *text = H;
    void *bytes = H;
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry text_root = {0}, bytes_root = {0};
    FVM_SRecord string = {.tag = STYX_MANAGED_STRING_TAG, .payload = utf8, .size = sizeof(utf8)};
    check(dvm_styx_managed_new("s", &string, &text) && text != H, "construct counted Styx string");
    check(styxscalarwrite(STYX_CALL_BYTE_TAG, STYX_SCALAR_VALUE, 255, encoded, sizeof(encoded), &first) &&
        styxscalarwrite(STYX_CALL_BYTE_TAG, STYX_SCALAR_VALUE, 0, encoded + first, sizeof(encoded) - first, &second),
        "encode typed bytes");
    FVM_SRecord array = {.tag = STYX_MANAGED_ARRAY_TAG, .payload = encoded, .size = first + second};
    check(dvm_styx_managed_new("a[y]", &array, &bytes) && bytes != H, "construct Styx byte array");
    unsigned char fields[120], frame_wire[128];
    size_t n, m, k, wire_size;
    DVM_StyxValueFrame frame = {0};
    check(styxscalarwrite(STYX_CALL_BYTE_TAG, STYX_SCALAR_VALUE, 93, fields, sizeof(fields), &n) &&
        fvm_srecord_write(STYX_MANAGED_STRING_TAG, utf8, sizeof(utf8), fields + n, sizeof(fields) - n, &m) == FVM_S_OK &&
        fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, encoded, first + second, fields + n + m,
            sizeof(fields) - n - m, &k) == FVM_S_OK &&
        styxmanaged_list_write(89, "(ysa[y])", fields, n + m + k, frame_wire, sizeof(frame_wire), &wire_size) &&
        dvm_styx_value_frame_read(89, "(ysa[y])", frame_wire, wire_size, &frame),
        "decode scalar and managed arguments into exact NC64 slots");
    String *frame_text;
    Array *frame_array;
    memcpy(&frame_text, &frame.words[1], sizeof(frame_text));
    memcpy(&frame_array, &frame.words[2], sizeof(frame_array));
    check(frame.layout.size == 24 && frame.layout.pointers == 6 && *(unsigned char *)frame.words == 93 &&
        !memcmp(string2c(frame_text), utf8, sizeof(utf8)) &&
        frame_array->len == 2 && frame_array->data[0] == 255 && frame_array->data[1] == 0,
        "decoded frame matches the original mixed arguments");
    dvm_styx_value_frame_clear(&frame);
    check(!frame.layout.count && !frame.words[1] && !frame.words[2], "retire decoded frame owners");
    check(fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, NULL, 0, fields + n + m,
            sizeof(fields) - n - m, &k) == FVM_S_OK &&
        styxmanaged_list_write(89, "(ysa[b])", fields, n + m + k, frame_wire, sizeof(frame_wire), &wire_size) &&
        !dvm_styx_value_frame_read(89, "(ysa[b])", frame_wire, wire_size, &frame) && !frame.layout.count,
        "unsupported leaf rolls back an already decoded owner");
    FVM_SRecord truncated = array;
    truncated.size--;
    void *unchanged = text;
    check(!dvm_styx_managed_new("a[y]", &truncated, &unchanged) && unchanged == text, "malformed array preserves output");
    FVM_SRecord absent = {.tag = STYX_MANAGED_NIL_TAG};
    check(dvm_styx_managed_new("s", &absent, &unchanged) && unchanged == H, "nil string remains nil");
    check(dvm_owner_scope_open(&scope) &&
        dvm_managed_owner_capture_into(text, &scope, &text_root) &&
        dvm_managed_owner_capture_into(bytes, &scope, &bytes_root), "root decoded Styx values");
    destroy(text);
    destroy(bytes);
    memset(encoded, 0, sizeof(encoded));
    check(dvm_native_owner_leave(owner), "leave with only Styx value roots");
    check(dvm_gc_workers_request(workers, DVM_GC_CYCLES) &&
        dvm_gc_workers_wait_idle(workers), "collect while decoded values are rooted");
    check(dvm_native_owner_enter(owner), "reenter with Styx ticket roots");
    text = dvm_managed_owner_value(&text_root);
    Array *restored = dvm_managed_owner_value(&bytes_root);
    check(text && text != H && !memcmp(string2c(text), utf8, sizeof(utf8)) &&
        restored && restored != H && restored->len == 2 && restored->data[0] == 255 && restored->data[1] == 0,
        "decoded values are independent of transport storage and survive collection");
    FVM_SRecord view;
    size_t used, consumed;
    check(dvm_styx_managed_write("s", text, encoded, sizeof(encoded), &used) &&
        fvm_srecord_read(encoded, used, sizeof(encoded), &view, &consumed) == FVM_S_OK &&
        consumed == used && view.tag == STYX_MANAGED_STRING_TAG && view.size == sizeof(utf8) &&
        !memcmp(view.payload, utf8, sizeof(utf8)), "encode rooted counted Styx string");
    check(dvm_styx_managed_write("a[y]", restored, encoded, sizeof(encoded), &used) &&
        fvm_srecord_read(encoded, used, sizeof(encoded), &view, &consumed) == FVM_S_OK &&
        consumed == used && dvm_styx_managed_new("a[y]", &view, &bytes) &&
        ((Array *)bytes)->len == 2 && !memcmp(((Array *)bytes)->data, restored->data, 2),
        "round-trip rooted Styx byte array");
    destroy(bytes);
    check(dvm_styx_managed_write("s", H, encoded, sizeof(encoded), &used) &&
        fvm_srecord_read(encoded, used, sizeof(encoded), &view, &consumed) == FVM_S_OK &&
        view.tag == STYX_MANAGED_NIL_TAG && !view.size, "encode nil without a heap object");
    const size_t previous = used;
    check(!dvm_styx_managed_write("s", text, encoded, 1, &used) && used == previous,
        "oversized wire value preserves result size");
    check(dvm_owner_scope_close(&scope), "retire decoded values");
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_wait_idle(workers) &&
        dvm_native_owner_enter(owner), "finish native root retirement");
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
    DVM_HeapOwner *owner = dvm_native_owner_current();
    String *value;
    int index;
    char previous[ERRMAX];
    dvm_runtime_type_probe(workers);
    dvm_runtime_heap_probe(workers);
    dvm_runtime_opcode_probe(workers);
    dvm_runtime_module_probe(workers);
    dvm_runtime_snapshot_probe(workers);
    dvm_runtime_service_probe(workers);
    dvm_runtime_pointer_builder_probe(workers);
    dvm_runtime_root_registry_probe(workers);
    styx_value_probe(owner, workers);
    value = c2string("managed-ticket-root", 19);
    check(world && workers && value != H && value != nil, "real managed String");
    probe.slot = dvm_owner_slot_create();
    check(probe.slot != NULL, "managed publication slot");
    probe.sync = fvm_port_sync_create();
    check(probe.sync && dvm_owner_scope_open(&scope), "controller scope");
    check(!dvm_managed_owner_capture_into((void *)1, NULL, &main_entry),
        "direct capture rejects destination before managed reads");
    check(dvm_managed_owner_capture_into(value, &scope, &main_entry) &&
        dvm_managed_owner_value(&main_entry) == value, "direct capture links rooted ticket");
    check(!dvm_managed_owner_capture_into((void *)1, &scope, &main_entry),
        "occupied direct capture refuses before managed reads");
    destroy(value);
    value = H;
    check(!dvm_managed_owner_can_close() && !dvm_managed_owner_close(), "live owners refuse shutdown");
    check(dvm_owner_scope_retain(&scope, &outgoing, main_entry.object, main_entry.ops), "distinct shared slot owner");
    check(dvm_owner_slot_put(probe.slot, &outgoing) && !outgoing.object,
        "publish rooted ticket directly from scope");
    check(dvm_owner_slot_replace(probe.slot, &main_entry) && main_entry.scope == &scope,
        "rooted publication replacement keeps local cleanup owner");
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
    dvm_runtime_guard_registry_probe(workers);
    puts("PASS managed owner probe: 8 native receivers, 32 real cycle passes, rooted until Fast, shared-slot snapshots, native error cleanup, complete joins");
}
