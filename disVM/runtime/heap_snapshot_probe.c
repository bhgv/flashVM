/* Real frozen Heap header while count-only native workers update ref atomically. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "heap_snapshot.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "disvm/gc_workers.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORKERS 8 /* Test count updaters, not a runtime worker quota. */
#define PAIRS 2000 /* Balanced count-only pairs per native worker. */
#define SNAPSHOTS 4000 /* Header observations with immutable non-count fields. */


/* Parent owns all storage through joins and never leaves managed admission. */
typedef struct SnapshotWork
{
    ulong *counter; /* Borrowed count storage; controller keeps original String owner. */
    FVM_PortSync *gate; /* Start barrier, not header/count synchronization. */
    int ready; /* Workers ready, protected by gate. */
    int start; /* Release workers, protected by gate. */
} SnapshotWork;


/* Explicit semantic predicate before bad lifetime access. No compiler/signal/
 * timeout/sanitizer failure counts as successful control detection. */
static void check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL snapshot-count probe: %s\n", message);
        exit(10);
    }
}


/* Native generic count API only: no managed payload/Type/header dereference,
 * context entry or root inference. Parent keeps storage/source owner until join
 * and freezes all other header bytes under ACTIVE admission. */
static void update_count(void *argument)
{
    SnapshotWork *work = argument;
    int pair;
    fvm_port_sync_lock(work->gate);
    work->ready++;
    fvm_port_sync_signal_all(work->gate);
    while (!work->start)
    {
        fvm_port_sync_wait(work->gate);
    }
    fvm_port_sync_unlock(work->gate);
    for (pair = 0; pair < PAIRS; pair++)
    {
        check(dvm_refcount_retain_u(work->counter) == DVM_REFCOUNT_LIVE, "count-only retain");
        check(dvm_refcount_release_u(work->counter) == DVM_REFCOUNT_LIVE, "source keeper prevents LAST");
    }
}


/* [GC:] workers is live GC service; caller owns ACTIVE controller. Only count
 * metadata races; this is NOT permission for arbitrary native heap access or
 * parallel managed mutators. Private snapshot owns no referenced objects. */
void dvm_runtime_snapshot_probe(DVM_GCWorkers *workers)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    String *value = c2string("snapshot-count", 14);
    Heap *source;
    Heap snapshot;
    Heap refusal;
    uchar unchanged[sizeof(Heap)];
    SnapshotWork work = {0};
    FVM_PortThread *threads[WORKERS];
    int index;
    check(value != H && value != nil, "real snapshot source");
    source = D2H(value);
    check(dvm_runtime_heap_snapshot(&snapshot, source) && snapshot.ref == 1,
        "snapshot preserves count");
    check(snapshot.t == &Tstring && snapshot.color == source->color,
        "snapshot preserves metadata");
    memset(&refusal, 0xa5, sizeof(refusal));
    memcpy(unchanged, &refusal, sizeof(unchanged));
    check(!dvm_runtime_heap_snapshot(&refusal, H) &&
        !memcmp(unchanged, &refusal, sizeof(unchanged)), "sentinel refuses unchanged");
    check(!dvm_runtime_heap_snapshot(&refusal, (uchar *)source+1) &&
        !memcmp(unchanged, &refusal, sizeof(unchanged)), "unaligned source refuses unchanged");
    check(!dvm_runtime_heap_snapshot(&refusal, (void *)((uintptr)-1-sizeof(Heap)+1)) &&
        !memcmp(unchanged, &refusal, sizeof(unchanged)), "span overflow refuses unchanged");
    check(!dvm_runtime_heap_snapshot(source, source), "overlap refuses");
    work.counter = &source->ref;
    work.gate = fvm_port_sync_create();
    check(work.gate != nil, "snapshot start gate");
    for (index = 0; index < WORKERS; index++)
    {
        threads[index] = fvm_port_thread_start(update_count, &work);
        check(threads[index] != nil, "snapshot count worker starts");
    }
    fvm_port_sync_lock(work.gate);
    while (work.ready != WORKERS)
    {
        fvm_port_sync_wait(work.gate);
    }
    work.start = 1;
    fvm_port_sync_signal_all(work.gate);
    fvm_port_sync_unlock(work.gate);
    for (index = 0; index < SNAPSHOTS; index++)
    {
        check(dvm_runtime_heap_snapshot(&snapshot, source) && snapshot.ref >= 1 &&
            snapshot.ref <= WORKERS+1 && snapshot.t == &Tstring,
            "count-only concurrent header observation");
    }
    for (index = 0; index < WORKERS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    check(heapref(value) == 1, "all native count owners joined");
    work.counter = nil;
    fvm_port_sync_destroy(work.gate);
    destroy(value);
    value = H;
    source = nil;
    check(dvm_native_owner_leave(owner), "snapshot controller leaves after all joins");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers), "snapshot source Fast retirement");
    check(dvm_native_owner_enter(owner), "snapshot controller reentry");
    puts("PASS snapshot-count probe: 4000 observations, 8 count-only workers, 16000 pairs, complete joins");
}
