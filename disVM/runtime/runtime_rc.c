/* Copyright (c) 2026 bhgv
 *
 * EN: Deferred reference ownership, GC lane queues and heap retirement.
 * RU: Отложенные ссылки, очереди потоков GC и освобождение объектов кучи.
 */
#include "disvm/refcount.h"
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "heap_pool.h"
#include "disvm/native_context.h"
#include "disvm/native_owner.h"
#include "runtime_rc.h"
#include "gc/common/graph_snapshot.h"
#include "gc/common/candidate_pass.h"
#include "gc/fast/graph_confirmation.h"
#include "gc/fast/packet_budget.h"
#include "runtime_gc.h"
#include "gc_fields.h"
#include "managed_owner.h"
#include "disvm/range_access.h"
#include "retirement_queue.h"
#include "root_batch.h"
#include "root_registry.h"
#include "channel_sync.h"
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define RC_CAPACITY 512 /* Initial FIFO slots for release and packet lanes. */
#define RC_CANDIDATE_CAPACITY 256 /* Initial slots; grow during nonfinal-owner bursts. */

/* A packet owns each captured reference through the collector's drain. */
typedef struct FrameRetirement
{
    DVM_RootBatch *roots; /* Published root occurrences for keys. */
    size_t count;         /* Number of transferred owners. */
    size_t next;          /* First owner not yet consumed by Fast. */
    void *keys[];        /* Heap headers in frame-map order. */
} FrameRetirement;

/* Keep the original strong owner and registry root until the old readers end. */
typedef struct LeafRetirement
{
    Heap *heap;           /* Rooted string header until the sole consumer takes it. */
    unsigned long bytes;  /* Direct bytes charged to whichever queue owns it. */
    unsigned long epoch;  /* Last potentially interested reader generation. */
    FrameRetirement *records; /* Optional rooted closed record group. */
    RecordGroup *snapshot;    /* Edges frozen by withdrawal of the last outside owner. */
    int claimed;          /* Counts withdrawn, awaiting the final reader epoch. */
    int pinned_children;  /* Shared discovery retained every child separately. */
    int claim_refused;        /* Fall back to normal release if closure changed. */
} LeafRetirement;

static int duke_publish_pending; /* Inflight snapshot is complete and waiting for Fast. */
static RecordGroup duke_inflight; /* Partially captured graph with rooted targets. */
static RecordGroup duke_abandoned; /* Owned discovery pins awaiting rollback. */
static int duke_cleanup_fallback; /* Atomic readiness for an exceptional stopped drain. */
static FVM_PortSync *candidate_transfer; /* Cross-index ownership handoff. */
static DVM_RetirementQueue *duke_candidates; /* Rooted large graphs for Duke GC. */
static DVM_RetirementQueue *cycle_candidates; /* Nonfinal owners for the Cycles lane. */
static DVM_RetirementQueue *cycle_refused; /* Candidates individually refused by Cycles. */
static DVM_RetirementQueue *record_retries; /* Transient guard conflicts and final epochs. */
static DVM_RetirementQueue *difficult; /* Fallback work with unchanged roots. */
static struct
{
    unsigned long passes, promoted, packet_promoted, groups, nonfinal, records, arrays, lists, channels, strings, modules, other;
} fallback;
static unsigned long difficult_moved;
static DVM_RetirementQueue *leaves; /* Plain leaves eligible for shared release. */
static unsigned long shared_strings_with_peer; /* Final releases with another active owner. */
static unsigned long shared_array_graphs; /* Shared releases that transfer array children. */
static unsigned long shared_arrays; /* Final scalar-array releases on Fast. */
static unsigned long shared_arrays_with_peer; /* Scalar-array releases with an active guest. */
static unsigned long shared_channels;
static unsigned long shared_plain_records;
static unsigned long shared_lists;
static unsigned long shared_strings; /* Final releases by the Fast shared pass. */

static DVM_RetirementQueue *frames; /* Bounded queue of frame packets. */
static DVM_CellDomain *record_cells; /* Borrowed until the collectors join. */
static unsigned long shared_record_scans;
static unsigned long cycle_candidates_rotated;
static unsigned long cycle_candidates_coalesced;
static unsigned long cycle_candidates_full; /* Candidate reservations refused for capacity. */
static unsigned long cycle_candidates_rooted;
static unsigned int duke_peak_fields; /* Largest successfully published Duke snapshot. */
static unsigned long duke_publish_retries; /* Complete snapshots retained under pressure. */
static unsigned long duke_adopted_pins; /* Discovery holds transferred to Fast. */
static unsigned long duke_resumed_slices; /* Partial scans saved for another admitted turn. */
static unsigned long duke_promoted; /* Large snapshots handed to Fast GC. */
static unsigned long cycle_candidates_oversized;
static unsigned long shared_record_confirmations;
static size_t fast_reserved_confirmations; /* Claims confirmed under exact field reservations. */
static size_t fast_ignored_live_fields; /* Surviving fields skipped during successful claims. */
static unsigned long shared_record_snapshots_with_peer;
static unsigned long shared_record_groups;
static unsigned long shared_record_groups_with_peer;
static unsigned long record_groups; /* Closed record groups reclaimed from queued owners. */
static unsigned long shared_children; /* Packet references consumed under shared admission. */
static unsigned long frames_drained; /* Stopped collector's packet count. */

extern Pool *heapmem;                 /* Retained pool, accessible only under world ownership. */
static DVM_RetirementQueue *retirement; /* Native metadata; managed ownership stays under world admission. */
static DVM_NativeContext *owner; /* Original controller, valid through collector join. */
static DVM_NativeWorld *world; /* Runtime identity, stable through producer/collector joins. */
static unsigned long enqueued;       /* Ownership transfers, observed after join. */
static unsigned long released;       /* Collector-side retained destroy calls. */
static unsigned long saturated;      /* Bounded-queue inline fallbacks. */
static unsigned long byte_limited;   /* Direct block-byte budget inline fallbacks. */
static unsigned long helper_enqueued; /* Accepted releases from other admitted executors. */
static unsigned long helper_saturated; /* Helper releases falling back at the owner-count bound. */
static unsigned long helper_byte_limited; /* Helper releases falling back at the byte bound. */
static void (*notify_ready)(void); /* Optional Fast wakeup, fixed until collector join. */
static int accepting;                /* Closed before mutator leaves on shutdown. */


/* [GC:] Authorize queue access before inspecting managed payload or queue data.
 * Only a bound ACTIVE owner of this runtime's exclusive world may proceed.
 * Native/idle/detached/collector contexts refuse without reading the queue.
 * The world lifetime is retained through all producer/collector joins. */
static int admitted(int allow_shared)
{
    DVM_HeapOwner *current = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    return current && dvm_native_owner_statistics(current, &state) &&
        (state.phase == DVM_HEAP_ACTIVE || (allow_shared && state.phase == DVM_HEAP_SHARED)) &&
        !state.pending_progress &&
        dvm_native_owner_world(current) == world;
}


/* [GC:] Initialize on the bound mutator before publishing work. No
 * managed pointer changes. Native queue allocation failure keeps inline RC.
 * A preceding session must be successfully finished; restart otherwise aborts. */
void dvm_runtime_rc_start(void)
{
    owner = dvm_native_context_current();
    record_cells = dvm_managed_cell_domain();
    world = dvm_native_owner_world(dvm_native_owner_current());
    helper_enqueued = 0;
    helper_saturated = 0;
    helper_byte_limited = 0;
    if (retirement || frames || leaves || difficult || record_retries || cycle_candidates || cycle_refused || duke_candidates)
    {
        fprintf(stderr, "disvm RC queue restarted before successful finish\n");
        abort();
    }
    retirement = dvm_retirement_create_growing(RC_CAPACITY);
    if (!retirement)
    {
        fprintf(stderr, "disvm: out of memory creating mandatory GC release queue\n");
        abort();
    }
    frames = dvm_retirement_create_growing(RC_CAPACITY);
    leaves = dvm_retirement_create_growing(RC_CAPACITY);
    difficult = dvm_retirement_create_growing(RC_CAPACITY);
    record_retries = dvm_retirement_create_growing(RC_CAPACITY);
    cycle_candidates = dvm_retirement_create_growing_indexed(RC_CANDIDATE_CAPACITY, ULONG_MAX);
    candidate_transfer = fvm_port_sync_create();
    if (candidate_transfer)
    {
        cycle_refused = dvm_retirement_create_growing_indexed(RC_CANDIDATE_CAPACITY, ULONG_MAX);
        duke_candidates = dvm_retirement_create_growing_indexed(RC_CANDIDATE_CAPACITY, ULONG_MAX);
    }
    difficult_moved = 0;
    memset(&fallback, 0, sizeof(fallback));
    shared_array_graphs = 0;
    shared_arrays = 0;
    shared_arrays_with_peer = 0;
    shared_strings = 0;
    shared_channels = 0;
    shared_plain_records = 0;
    shared_lists = 0;
    shared_strings_with_peer = 0;
    frames_drained = 0;
    shared_children = 0;
    record_groups = 0;
    shared_record_groups = 0;
    shared_record_scans = 0;
    cycle_candidates_rotated = 0;
    cycle_candidates_coalesced = 0;
    cycle_candidates_full = 0;
    cycle_candidates_rooted = 0;
    duke_promoted = 0;
    duke_resumed_slices = 0;
    duke_adopted_pins = 0;
    duke_publish_retries = 0;
    duke_publish_pending = 0;
    duke_peak_fields = 0;
    cycle_candidates_oversized = 0;
    shared_record_confirmations = 0;
    fast_ignored_live_fields = 0;
    fast_reserved_confirmations = 0;
    shared_record_snapshots_with_peer = 0;
    shared_record_groups_with_peer = 0;
    enqueued = 0;
    released = 0;
    saturated = 0;
    byte_limited = 0;
    accepting = retirement != NULL;
}


/* Set the wake callback at startup before any guest produces a packet. */
void dvm_runtime_rc_set_notifier(void (*notify)(void))
{
    notify_ready = notify;
}


extern int dvm_runtime_channel_claim_quiet(Heap *heap, Array *buffer, Type **mapped, int already_claimed);


/* Publish transferred owners to an incremental pass already in progress. */
static int publish_release_roots(DVM_RootBatch *roots, void *const *keys, size_t count)
{
    for (size_t index = 0; index < count; index++)
    {
        Heap *heap = keys[index];
        if (!dvm_gc_field_mark(&heap->color, &mutator, &nprop))
        {
            return 0;
        }
    }
    return dvm_runtime_root_batch_publish(roots, keys, count);
}


/* Read leaf payload only while its queue or caller owns a strong reference.
 * Array children require a prepared retirement packet. */
static int leaf_eligible(const Heap *heap)
{
    if (heap->t == &Tstring)
    {
        return 1;
    }
    if (heap->t == &Tchannel)
    {
        return H2D(const Channel *, heap)->buf != NULL;
    }
    if (heap->t == &Tarray)
    {
        const Array *array = H2D(const Array *, heap);
        return array->t && array->t != H;
    }
    return heap->t && ((dvm_runtime_idle_cycles && heap->t == &Tlist) || heap->t->free == freeheap);
}


/* Drop an independent descriptor owner. No VM callback is invoked. */
static void release_owned_descriptor(Type *type)
{
    DVM_RefcountResult result = dvm_refcount_release_i(&type->ref);
    if (result == DVM_REFCOUNT_LAST)
    {
        free(type->initialize);
        free(type);
    }
    else if (result != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
}


/* Try the shared leaf queue while the destroying guest owns exclusion.
 * All failure paths keep the existing owner available to ordinary destroy. */
static int defer_leaf(Heap *heap, unsigned long bytes, int exclusive)
{
    if (!leaves || heapmonitor || !leaf_eligible(heap))
    {
        return 0;
    }
    DVM_RetirementReservation token = {0};
    if (dvm_retirement_reserve(leaves, bytes, &token) != DVM_RETIRE_OK)
    {
        return 0;
    }
    LeafRetirement *entry = malloc(sizeof(*entry));
    if (!entry)
    {
        dvm_retirement_cancel(leaves, &token);
        return 0;
    }
    entry->heap = heap;
    entry->bytes = bytes;
    entry->records = NULL;
    entry->snapshot = NULL;
    entry->claim_refused = 0;
    entry->pinned_children = 0;
    entry->claimed = 0;
    entry->epoch = dvm_world_retire_epoch(world);
    if (!entry->epoch || (!exclusive && !dvm_gc_field_mark(&heap->color, &mutator, &nprop)) ||
        !dvm_runtime_root_add(heap))
    {
        free(entry);
        dvm_retirement_cancel(leaves, &token);
        return 0;
    }
    if (exclusive)
    {
        Setmark(heap);
    }
    int became_ready;
    if (!dvm_retirement_commit_ready(leaves, &token, entry, &became_ready))
    {
        abort();
    }
    if (exclusive && became_ready && notify_ready)
    {
        notify_ready();
    }
    return 1;
}


/* No operation after publishing the roots can fail recoverably. */
static void unlink_record_group(const RecordGroup *group)
{
    for (unsigned int index = 1; index < group->nodes_used; index++)
    {
        if (!group->live[index] && dvm_refcount_retain_u(&D2H(group->nodes[index])->ref) != DVM_REFCOUNT_LIVE)
        {
            abort();
        }
    }
    for (unsigned int index = 0; index < group->edges_used; index++)
    {
        if (group->targets[index] == UINT_MAX ||
            (group->sources && group->live[group->sources[index]]))
        {
            continue;
        }
        void *edge = H;
        unsigned int target = group->targets[index];
        void *value = group->nodes[target];
        memcpy(group->cells[index], &edge, sizeof(edge));
        if (group->live[target])
        {
            destroy(value);
        }
        else if (!dvm_refcount_drop_nonfinal_u(&D2H(value)->ref))
        {
            abort();
        }
    }
}


/* EN: Publish group for Fast using guarded fields or existing Duke pins.
 * shared selects the parallel path. retry reports transient pressure.
 * RU: Передать group в Fast под защитой полей или с удержаниями ДюкGC.
 * shared выбирает параллельный путь. retry сообщает о временном отказе. */
static int defer_record_group(RecordGroup *group, int shared, int *retry)
{
    *retry = 0;
    unsigned int edges;
    if (!dvm_rc_snapshot_edges(group, &edges))
    {
        return 0;
    }
    DVM_RetirementReservation token = {0};
    size_t packet_bytes = sizeof(FrameRetirement) + group->nodes_used * sizeof(void *);
    size_t metadata = sizeof(LeafRetirement) + dvm_rc_record_group_bytes(group, edges) +
        packet_bytes + dvm_runtime_root_batch_bytes(group->nodes_used);
    if (metadata > ULONG_MAX)
    {
        return 0;
    }
    unsigned long bytes = metadata;
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        unsigned long size = poolmsize(heapmem, D2H(group->nodes[index]));
        if (size > ULONG_MAX - bytes)
        {
            return 0;
        }
        bytes += size;
    }
    if (dvm_world_has_borrowers(world))
    {
        *retry = 1;
        return 0;
    }
    DVM_RetirementResult reserved = dvm_retirement_reserve(leaves, bytes, &token);
    if (reserved != DVM_RETIRE_OK)
    {
        *retry = reserved == DVM_RETIRE_FULL || reserved == DVM_RETIRE_BYTES;
        return 0;
    }
    LeafRetirement *entry = malloc(sizeof(*entry));
    FrameRetirement *records = malloc(packet_bytes);
    RecordGroup *snapshot = dvm_rc_copy_record_group(group, edges, H);
    DVM_RootBatch *roots = dvm_runtime_root_batch_create(group->nodes_used);
    unsigned long epoch = dvm_world_retire_epoch(world);
    if (!entry || !records || !snapshot || !roots || !epoch)
    {
        if (roots)
        {
            dvm_runtime_root_batch_destroy(&roots);
        }
        free(snapshot);
        free(records);
        free(entry);
        dvm_retirement_cancel(leaves, &token);
        return 0;
    }
    records->roots = roots;
    records->count = group->nodes_used;
    records->next = 0;
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        records->keys[index] = D2H(group->nodes[index]);
    }
    int pin_children = shared || group->live_count || group->has_views;
    unsigned int adopted = group->pinned_nodes;
    if (adopted && (!shared || adopted != group->nodes_used - 1))
    {
        abort();
    }
    if (pin_children)
    {
        for (unsigned int index = 1; index < group->nodes_used; index++)
        {
            if (!adopted && dvm_refcount_retain_u(&D2H(group->nodes[index])->ref) != DVM_REFCOUNT_LIVE)
            {
                abort();
            }
            snapshot->incoming[index]++;
        }
    }
    if (!dvm_runtime_root_batch_publish(roots, records->keys, records->count))
    {
        abort();
    }
    /* Publish every replacement root before withdrawing the discovery roots.
     * The snapshot now owns the same strong references without another retain. */
    while (group->pinned_nodes)
    {
        if (!dvm_runtime_root_remove(records->keys[group->pinned_nodes]))
        {
            abort();
        }
        group->pinned_nodes--;
    }
    for (size_t index = 0; index < records->count; index++)
    {
        Heap *heap = records->keys[index];
        if (shared)
        {
            if (!dvm_gc_field_mark(&heap->color, &mutator, &nprop))
            {
                abort();
            }
        }
        else
        {
            Setmark(heap);
        }
    }
    *entry = (LeafRetirement){.bytes = bytes, .epoch = epoch, .records = records,
        .snapshot = snapshot, .pinned_children = pin_children};
    int became_ready;
    if (!dvm_retirement_commit_ready(leaves, &token, entry, &became_ready))
    {
        abort();
    }
    if (became_ready && notify_ready)
    {
        notify_ready();
    }
    return 1;
}


/* Ordinary records and owning arrays with managed fields can start discovery.
 * The scanner charges mapped fields as it encounters them. */
static int record_candidate(const Heap *heap)
{
    if (!dvm_runtime_idle_cycles || !heap->t || gchalt || heapmonitor)
    {
        return 0;
    }
    if (heap->t == &Tarray)
    {
        const Array *array = H2D(const Array *, heap);
        return array->root && array->t && (array->root != H ||
            (array->len > 0 && array->t->np));
    }
    return heap->t == &Tlist || heap->t == &Tchannel || (heap->t->free == freeheap && heap->t->np);
}


/* Producers are exclusive Dis owners or the single Fast lane. A ready
 * entry holds the same allocation alive through this observation. */
static int cycle_candidate_already_queued(DVM_HeapKey key)
{
    int queued = dvm_retirement_contains_key(cycle_candidates, key) ||
        dvm_retirement_contains_key(duke_candidates, key) ||
        dvm_retirement_contains_key(cycle_refused, key);
    cycle_candidates_coalesced += queued;
    return queued;
}


/* Keep the packet owner intact until its replacement queue root is published. */
static int transfer_record_candidate(Heap *heap, FrameRetirement *packet, size_t index)
{
    if (!candidate_transfer || !record_candidate(heap) || dvm_refcount_load_u(&heap->ref) <= 1)
    {
        return 0;
    }
    DVM_HeapKey key = dvm_heap_pool_key(heapmem, heap);
    if (!key.generation || cycle_candidate_already_queued(key))
    {
        return 0;
    }
    DVM_RetirementReservation token = {0};
    unsigned long bytes = poolmsize(heapmem, heap);
    DVM_RetirementResult reservation = dvm_retirement_reserve(cycle_candidates, bytes, &token);
    if (reservation != DVM_RETIRE_OK)
    {
        cycle_candidates_full += reservation == DVM_RETIRE_FULL;
        return 0;
    }
    fvm_port_sync_lock(candidate_transfer);
    if (cycle_candidate_already_queued(key) ||
        !dvm_gc_field_mark(&heap->color, &mutator, &nprop) || !dvm_runtime_root_add(heap))
    {
        fvm_port_sync_unlock(candidate_transfer);
        dvm_retirement_cancel(cycle_candidates, &token);
        return 0;
    }
    int became_ready;
    if (!dvm_runtime_root_batch_remove(packet->roots, index) ||
        !dvm_retirement_commit_key(cycle_candidates, &token, H2D(void *, heap), key, &became_ready))
    {
        abort();
    }
    fvm_port_sync_unlock(candidate_transfer);
    if (became_ready && notify_ready)
    {
        notify_ready();
    }
    __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
    helper_enqueued++;
    return 1;
}


/* [GC:] Transfer the caller's reference to GC. Optional leaf and cycle
 * lanes may decline, but a live mutator always publishes to the release FIFO.
 * RU: Передать ссылку вызывающего в GC. Специализированные очереди могут
 * отказаться, но действующий исполнитель всегда публикует её в FIFO. */
int dvm_runtime_rc_defer(void *value)
{
    if (!admitted(1) || !accepting)
    {
        return 0;
    }
    int active = admitted(0);
    int helper = dvm_native_context_current() != owner;
    Heap *heap = D2H(value);
    int candidate = active && record_candidate(heap);
    if (!candidate && dvm_refcount_drop_nonfinal_u(&heap->ref))
    {
        return 1;
    }
    int last = dvm_refcount_load_u(&heap->ref) == 1;
    unsigned long bytes = poolmsize(heapmem, heap);
    if (!active || !last || !defer_leaf(heap, bytes, 1))
    {
        DVM_RetirementReservation token = {0};
        DVM_HeapKey key = {0};
        int ready = 0;
        int queued = 0;
        if (!last && candidate)
        {
            key = dvm_heap_pool_key(heapmem, heap);
        }
        /* Transfer the existing strong reference and root it for tracing. */
        if (!dvm_runtime_root_add(heap))
        {
            fprintf(stderr, "disvm: out of memory rooting mandatory GC release\n");
            abort();
        }
        Setmark(heap);
        if (candidate_transfer && key.generation && dvm_retirement_reserve(cycle_candidates, bytes, &token) == DVM_RETIRE_OK)
        {
            fvm_port_sync_lock(candidate_transfer);
            if (!cycle_candidate_already_queued(key))
                queued = dvm_retirement_commit_key(cycle_candidates, &token, value, key, &ready);
            fvm_port_sync_unlock(candidate_transfer);
            if (!queued && !dvm_retirement_cancel(cycle_candidates, &token))
            {
                abort();
            }
        }
        if (!queued)
        {
            DVM_RetirementResult result = dvm_retirement_reserve(retirement, bytes, &token);
            if (result != DVM_RETIRE_OK ||
                !dvm_retirement_commit_ready(retirement, &token, value, &ready))
            {
                fprintf(stderr, "disvm: mandatory GC release publication failed (%d)\n", result);
                abort();
            }
        }
        if (ready && notify_ready)
        {
            notify_ready();
        }
    }
    __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
    __atomic_fetch_add(&helper_enqueued, helper != 0, __ATOMIC_RELAXED);
    return 1;
}


/* Transfer a private frame's mapped references without running destructors.
 * frame/type are borrowed from the current shared guest. On refusal the
 * frame remains intact. On success the caller must unpublish it before polling. */
int dvm_runtime_rc_transfer_frame(const void *frame, const Type *type, void *extra)
{
    DVM_HeapOwner *current = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    DVM_RetirementReservation token = {0};
    size_t capacity = extra != NULL;
    if (!frames || !dvm_native_owner_statistics(current, &state) ||
        state.phase != DVM_HEAP_SHARED || state.pending_progress ||
        dvm_native_owner_world(current) != world ||
        dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    unsigned long direct = extra ? poolmsize(heapmem, D2H(extra)) : 0;
    for (int byte = 0; type && byte < type->np; byte++)
    {
        for (unsigned bit = 0; bit < 8; bit++)
        {
            if (type->map[byte] & (0x80U >> bit))
            {
                size_t slot = (size_t)byte * 8 + bit;
                if (slot < sizeof(Frame) / sizeof(void *) ||
                    slot >= (size_t)type->size / sizeof(void *))
                {
                    return 0;
                }
                void *value;
                memcpy(&value, (const unsigned char *)frame + slot * sizeof(value), sizeof(value));
                if (!value)
                {
                    return 0;
                }
                if (value != H)
                {
                    unsigned long size = poolmsize(heapmem, D2H(value));
                    if (size > ULONG_MAX - direct)
                    {
                        return 0;
                    }
                    direct += size;
                }
                capacity++;
            }
        }
    }
    if (!capacity)
    {
        return 1;
    }
    if (capacity > (SIZE_MAX - sizeof(FrameRetirement)) / sizeof(void *))
    {
        return 0;
    }
    size_t bytes = sizeof(FrameRetirement) + capacity * sizeof(void *);
    if (bytes > ULONG_MAX - direct)
    {
        return 0;
    }
    DVM_RetirementResult reserved = dvm_retirement_reserve(frames, direct + (unsigned long)bytes, &token);
    if (reserved != DVM_RETIRE_OK)
    {
        return reserved == DVM_RETIRE_FULL || reserved == DVM_RETIRE_BYTES ? -1 : 0;
    }
    FrameRetirement *packet = malloc(bytes);
    if (!packet)
    {
        dvm_retirement_cancel(frames, &token);
        return 0;
    }
    packet->roots = dvm_runtime_root_batch_create(capacity);
    packet->count = 0;
    packet->next = 0;
    int valid = 1;
    for (int byte = 0; packet->roots && valid && type && byte < type->np; byte++)
    {
        for (unsigned bit = 0; bit < 8; bit++)
        {
            if (type->map[byte] & (1U << bit))
            {
                void *value;
                size_t offset = ((size_t)byte * 8 + 7 - bit) * sizeof(value);
                memcpy(&value, (const unsigned char *)frame + offset, sizeof(value));
                if (!value)
                {
                    valid = 0;
                    break;
                }
                if (value != H)
                {
                    packet->keys[packet->count++] = D2H(value);
                }
            }
        }
    }
    if (extra)
    {
        packet->keys[packet->count++] = D2H(extra);
    }
    if (packet->roots && valid && !packet->count)
    {
        dvm_runtime_root_batch_destroy(&packet->roots);
        free(packet);
        dvm_retirement_cancel(frames, &token);
        return 1;
    }
    if (!packet->roots || !valid ||
        !publish_release_roots(packet->roots, packet->keys, packet->count))
    {
        if (packet->roots)
        {
            dvm_runtime_root_batch_destroy(&packet->roots);
        }
        free(packet);
        dvm_retirement_cancel(frames, &token);
        return 0;
    }
    int became_ready;
    if (!dvm_retirement_commit_ready(frames, &token, packet, &became_ready))
    {
        abort();
    }
    if (became_ready && notify_ready)
    {
        notify_ready();
    }
    return 1;
}


/* EN: Stage the children of array heap before claiming its last owner.
 * retry reports temporary packet pressure. Refusal preserves the array.
 * RU: Подготовить ссылки массива heap до снятия последнего владельца.
 * retry сообщает о временном заполнении. При отказе массив сохраняется. */
static int transfer_array_children(Heap *heap, int *retry)
{
    Array *array = H2D(Array *, heap);
    Type *type = array->t;
    if (!frames || dvm_refcount_load_u(&heap->ref) != 1)
    {
        return 0;
    }
    size_t capacity = 1;
    if (array->root == H)
    {
        if (array->len < 0 || type->size <= 0 || type->np <= 0 ||
            (size_t)type->np > ((size_t)type->size / sizeof(void *) + 7) / 8)
        {
            return 0;
        }
        size_t mapped = 0;
        for (int byte = 0; byte < type->np; byte++)
        {
            mapped += __builtin_popcount((unsigned char)type->map[byte]);
        }
        size_t available = (SIZE_MAX - sizeof(FrameRetirement)) / sizeof(void *);
        if (mapped && (size_t)array->len > available / mapped)
        {
            return 0;
        }
        capacity = (size_t)array->len * mapped;
    }
    if (!capacity)
    {
        return dvm_refcount_claim_last_u(&heap->ref);
    }
    size_t storage = sizeof(FrameRetirement) + capacity * sizeof(void *);
    FrameRetirement *packet = malloc(storage);
    if (!packet)
    {
        return 0;
    }
    packet->roots = NULL;
    packet->count = 0;
    packet->next = 0;
    int valid = 1;
    if (array->root != H)
    {
        if (!array->root)
        {
            valid = 0;
        }
        else
        {
            packet->keys[packet->count++] = D2H(array->root);
        }
    }
    else
    {
        for (WORD index = 0; valid && index < array->len; index++)
        {
            const uchar *element = array->data + (size_t)index * type->size;
            for (int byte = 0; valid && byte < type->np; byte++)
            {
                for (unsigned bit = 0; bit < 8; bit++)
                {
                    if (type->map[byte] & (1U << bit))
                    {
                        size_t slot = (size_t)byte * 8 + 7 - bit;
                        void *child;
                        if (slot >= (size_t)type->size / sizeof(child))
                        {
                            valid = 0;
                            break;
                        }
                        memcpy(&child, element + slot * sizeof(child), sizeof(child));
                        if (!child)
                        {
                            valid = 0;
                            break;
                        }
                        if (child != H)
                        {
                            packet->keys[packet->count++] = D2H(child);
                        }
                    }
                }
            }
        }
    }
    if (valid && !packet->count)
    {
        free(packet);
        return dvm_refcount_claim_last_u(&heap->ref);
    }
    if (valid)
    {
        packet->roots = dvm_runtime_root_batch_create(packet->count);
        valid = packet->roots != NULL;
    }
    unsigned long charge = 0;
    valid = valid && dvm_gc_packet_charge(packet->keys, packet->count, storage, ULONG_MAX, &charge);
    DVM_RetirementReservation token = {0};
    DVM_RetirementResult reservation = valid ?
        dvm_retirement_reserve(frames, charge, &token) : DVM_RETIRE_INVALID;
    *retry = reservation == DVM_RETIRE_FULL || reservation == DVM_RETIRE_BYTES;
    int reserved = reservation == DVM_RETIRE_OK;
    int rooted = reserved && publish_release_roots(packet->roots, packet->keys, packet->count);
    if (rooted && dvm_refcount_claim_last_u(&heap->ref))
    {
        if (!dvm_retirement_commit(frames, &token, packet))
        {
            abort();
        }
        return 1;
    }
    if (rooted && !dvm_runtime_root_batch_clear(packet->roots))
    {
        abort();
    }
    if (reserved)
    {
        dvm_retirement_cancel(frames, &token);
    }
    if (packet->roots)
    {
        dvm_runtime_root_batch_destroy(&packet->roots);
    }
    free(packet);
    return 0;
}


/* Consume a bounded prefix of the oldest packet. The suffix keeps its
 * registry roots. Only the Fast lane mutates packet cursors. */
static unsigned int drain_shared_children(size_t budget)
{
    unsigned int completed = 0;
    while (budget)
    {
        FrameRetirement *packet = dvm_retirement_peek(frames);
        if (!packet)
        {
            break;
        }
        while (budget && packet->next < packet->count)
        {
            Heap *heap = packet->keys[packet->next];
            int transferred = transfer_record_candidate(heap, packet, packet->next);
            if (!transferred)
            {
                if (dvm_refcount_drop_nonfinal_u(&heap->ref))
                {
                    dvm_runtime_gc_owner_lost();
                }
                else
                {
                    if (!leaf_eligible(heap) || !defer_leaf(heap, poolmsize(heapmem, heap), 0))
                    {
                        return completed;
                    }
                    __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
                    helper_enqueued++;
                }
            }
            if (!transferred && !dvm_runtime_root_batch_remove(packet->roots, packet->next))
            {
                abort();
            }
            packet->next++;
            shared_children++;
            completed++;
            budget--;
        }
        if (packet->next == packet->count)
        {
            void *value;
            unsigned long bytes;
            if (!dvm_retirement_take(frames, &value, &bytes) || value != packet ||
                !dvm_runtime_root_batch_clear(packet->roots) ||
                !dvm_runtime_root_batch_destroy(&packet->roots))
            {
                abort();
            }
            free(packet);
            frames_drained++;
        }
    }
    return completed;
}


/* EN: Root count owners from keys. Charge payload bytes for the first
 * charged_keys entries. token receives the reservation, retry reports pressure.
 * RU: Удержать count ссылок из keys. Учесть память первых charged_keys объектов.
 * token получает резервацию, retry сообщает о временном заполнении очереди. */
static FrameRetirement *prepare_release_packet(void *const *keys, size_t count,
    size_t charged_keys, DVM_RetirementReservation *token, int *retry)
{
    unsigned long charge;
    size_t storage = sizeof(FrameRetirement) + count * sizeof(void *);
    if (!dvm_gc_packet_charge(keys, charged_keys, storage, ULONG_MAX, &charge))
    {
        return NULL;
    }
    DVM_RetirementResult reservation = dvm_retirement_reserve(frames, charge, token);
    if (reservation != DVM_RETIRE_OK)
    {
        if (retry)
        {
            *retry = reservation == DVM_RETIRE_FULL || reservation == DVM_RETIRE_BYTES;
        }
        return NULL;
    }
    FrameRetirement *packet = malloc(sizeof(*packet) + count * sizeof(void *));
    DVM_RootBatch *roots = dvm_runtime_root_batch_create(count);
    if (!packet || !roots)
    {
        if (roots)
        {
            dvm_runtime_root_batch_destroy(&roots);
        }
        free(packet);
        dvm_retirement_cancel(frames, token);
        return NULL;
    }
    packet->roots = roots;
    packet->count = count;
    packet->next = 0;
    memcpy(packet->keys, keys, count * sizeof(void *));
    if (!publish_release_roots(roots, packet->keys, count))
    {
        dvm_runtime_root_batch_destroy(&roots);
        free(packet);
        dvm_retirement_cancel(frames, token);
        return NULL;
    }
    return packet;
}


/* EN: Transfer one hold per live node and every dead-to-live edge in group.
 * Refusal preserves existing roots. Charge each live allocation only once.
 * RU: Передать удержания живых узлов и каждую ссылку на них из мёртвых узлов group.
 * При отказе корни сохраняются. Память каждого живого объекта учитывается один раз. */
static int release_record_branches(const RecordGroup *group)
{
    if (!group->live_count)
    {
        return 1;
    }
    void *local_keys[RECORD_NODE_CAPACITY + RECORD_EDGE_LIMIT];
    size_t capacity = (size_t)group->nodes_used + group->edges_used;
    void **keys = local_keys;
    if (capacity > sizeof(local_keys) / sizeof(*local_keys))
    {
        keys = malloc(capacity * sizeof(*keys));
        if (!keys)
        {
            return 0;
        }
    }
    size_t count = 0;
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        if (group->live[index])
        {
            keys[count++] = D2H(group->nodes[index]);
        }
    }
    size_t unique = count;
    for (unsigned int edge = 0; edge < group->edges_used; edge++)
    {
        unsigned int target = group->targets[edge];
        if (target != UINT_MAX && group->live[target])
        {
            keys[count++] = D2H(group->nodes[target]);
        }
    }
    DVM_RetirementReservation token = {0};
    FrameRetirement *packet = prepare_release_packet(keys, count, unique, &token, NULL);
    if (keys != local_keys)
    {
        free(keys);
    }
    if (!packet)
    {
        return 0;
    }
    if (!dvm_retirement_commit(frames, &token, packet))
    {
        abort();
    }
    return 1;
}


/* A retiring record still owns every mapped field. Publish the child
 * packet before claiming the record, with rollback on a changed count. */
static int transfer_object_fields(Heap *heap, int *retry)
{
    Type *type = heap->t;
    if (dvm_refcount_load_u(&heap->ref) != 1)
    {
        return 0;
    }
    void *keys[RECORD_EDGE_LIMIT];
    size_t count = 0;
    unsigned char *data = H2D(unsigned char *, heap);
    if (type == &Tlist)
    {
        List *list = H2D(List *, heap);
        if (!list->tail)
        {
            return 0;
        }
        if (list->tail != H)
        {
            keys[count++] = D2H(list->tail);
        }
        type = list->t;
        data = (unsigned char *)list->data;
    }
    for (int byte = 0; type && byte < type->np; byte++)
    {
        for (unsigned int bit = 0; bit < 8; bit++)
        {
            if (!(type->map[byte] & (0x80U >> bit)))
            {
                continue;
            }
            size_t slot = (size_t)byte * 8 + bit;
            void *child;
            if (slot >= (size_t)type->size / sizeof(child))
            {
                return 0;
            }
            memcpy(&child, data + slot * sizeof(child), sizeof(child));
            if (!child)
            {
                return 0;
            }
            if (child != H)
            {
                if (count == RECORD_EDGE_LIMIT)
                {
                    return 0;
                }
                keys[count++] = D2H(child);
            }
        }
    }
    if (!count)
    {
        return dvm_refcount_claim_last_u(&heap->ref);
    }
    DVM_RetirementReservation token = {0};
    FrameRetirement *packet = prepare_release_packet(keys, count, count, &token, retry);
    if (!packet)
    {
        return 0;
    }
    if (dvm_refcount_claim_last_u(&heap->ref))
    {
        if (!dvm_retirement_commit(frames, &token, packet))
        {
            abort();
        }
        return 1;
    }
    if (!dvm_runtime_root_batch_clear(packet->roots) ||
        !dvm_runtime_root_batch_destroy(&packet->roots) ||
        !dvm_retirement_cancel(frames, &token))
    {
        abort();
    }
    free(packet);
    return 0;
}


extern int dvm_runtime_channel_buffer_busy(Channel *channel);
extern int dvm_runtime_channel_post_close(Channel *channel);


/* EN: Claim the last owner of heap and return mapped descriptor on success.
 * retry reports packet pressure, an active buffer transfer or a competing retain.
 * RU: Захватить последнюю ссылку heap и вернуть дескриптор mapped при успехе.
 * retry сообщает о заполнении очереди, передаче буфера или новой ссылке. */
static int claim_quiet_channel(Heap *heap, Type **mapped, int *retry)
{
    Channel *channel = H2D(Channel *, heap);
    if (dvm_runtime_channel_buffer_busy(channel))
    {
        *retry = 1;
        return 0;
    }
    Array *buffer = channel->buf;
    if (buffer == H)
    {
        int claimed = dvm_runtime_channel_claim_quiet(heap, buffer, mapped, 0);
        *retry = !claimed && (dvm_runtime_channel_buffer_busy(channel) ||
            dvm_refcount_load_u(&heap->ref) > 1 || dvm_runtime_channel_post_close(channel));
        return claimed;
    }
    if (!buffer)
    {
        return 0;
    }
    void *key = D2H(buffer);
    DVM_RetirementReservation token = {0};
    FrameRetirement *packet = prepare_release_packet(&key, 1, 1, &token, retry);
    if (!packet)
    {
        return 0;
    }
    if (dvm_runtime_channel_claim_quiet(heap, buffer, mapped, 0))
    {
        if (!dvm_retirement_commit(frames, &token, packet))
        {
            abort();
        }
        return 1;
    }
    if (!dvm_runtime_root_batch_clear(packet->roots) ||
        !dvm_runtime_root_batch_destroy(&packet->roots) ||
        !dvm_retirement_cancel(frames, &token))
    {
        abort();
    }
    free(packet);
    *retry = dvm_runtime_channel_buffer_busy(channel) || dvm_refcount_load_u(&heap->ref) > 1 ||
        dvm_runtime_channel_post_close(channel);
    return 0;
}


/* The producer proved closure under exclusive admission and transferred the
 * sole outside owner. Wait out old borrows before touching the saved cells. */
static void reclaim_detached_records(LeafRetirement *entry, int shared)
{
    FrameRetirement *records = entry->records;
    if (!shared && !entry->claim_refused && !entry->pinned_children)
    {
        unlink_record_group(entry->snapshot);
    }
    RecordGroup *snapshot = entry->snapshot;
    if (!dvm_runtime_root_batch_clear(records->roots) ||
        !dvm_runtime_root_batch_destroy(&records->roots))
    {
        abort();
    }
    size_t owners = entry->claim_refused && !entry->pinned_children ? 1 : records->count;
    for (size_t index = 0; index < owners; index++)
    {
        Heap *heap = records->keys[index];
        if ((shared || entry->claimed) && snapshot->live[index])
        {
            if (!shared)
            {
                destroy(H2D(void *, heap));
            }
        }
        else if (shared || entry->claimed)
        {
            if (heap->t == &Tarray)
            {
                release_owned_descriptor(H2D(Array *, heap)->t);
            }
            if (heap->t == &Tchannel)
            {
                Type *mapped = NULL;
                if (!dvm_runtime_channel_claim_quiet(heap, H2D(Channel *, heap)->buf, &mapped, 1))
                {
                    abort();
                }
                if (mapped)
                {
                    release_owned_descriptor(mapped);
                }
                shared_channels += shared;
            }
            if (heap->t == &Tlist && H2D(List *, heap)->t)
            {
                release_owned_descriptor(H2D(List *, heap)->t);
            }
            release_owned_descriptor(heap->t);
            poolfree(heapmem, heap);
        }
        else
        {
            destroy(H2D(void *, heap));
        }
    }
    if (!shared && entry->claimed)
    {
        for (unsigned int edge = 0; edge < snapshot->edges_used; edge++)
        {
            unsigned int target = snapshot->targets[edge];
            if (target != UINT_MAX && snapshot->live[target])
            {
                destroy(snapshot->nodes[target]);
            }
        }
    }
    free(snapshot);
    free(records);
    record_groups += !entry->claim_refused && (shared || entry->claimed || !entry->pinned_children);
}


/* The sole consumer moves metadata only. Reserve before removing the head
 * so a full fallback queue leaves ownership and root publication unchanged. */
static int move_leaf(DVM_RetirementQueue *source, DVM_RetirementQueue *destination,
    LeafRetirement *entry)
{
    DVM_RetirementReservation token = {0};
    if (dvm_retirement_reserve(destination, entry->bytes, &token) != DVM_RETIRE_OK)
    {
        return 0;
    }
    void *value;
    unsigned long bytes;
    if (!dvm_retirement_take(source, &value, &bytes) || value != entry ||
        bytes != entry->bytes || !dvm_retirement_commit(destination, &token, entry))
    {
        abort();
    }
    difficult_moved += destination == difficult;
    return 1;
}


/* EN: Return strong references from a refused entry to Fast as a release
 * packet. source and entry remain intact if the packet cannot be queued.
 * RU: Передать сильные ссылки отклонённого entry в Fast. Если пакет нельзя
 * поставить в очередь, source и entry остаются целыми. */
static DVM_RetirementResult release_refused_snapshot(DVM_RetirementQueue *source, LeafRetirement *entry)
{
    FrameRetirement *records = entry->records;
    unsigned long charge = entry->bytes - sizeof(*entry) - dvm_rc_record_group_bytes(entry->snapshot, entry->snapshot->edges_used);
    DVM_RetirementReservation token = {0};
    DVM_RetirementResult result = dvm_retirement_reserve(frames, charge, &token);
    if (result != DVM_RETIRE_OK)
    {
        return result;
    }
    if (!entry->pinned_children)
    {
        /* Exclusive discovery published descendant roots without retaining
         * their counts. Only the initial node owns a releasable reference. */
        for (size_t index = 1; index < records->count; index++)
        {
            if (!dvm_runtime_root_batch_remove(records->roots, index))
            {
                abort();
            }
        }
        records->count = 1;
    }
    void *value;
    unsigned long bytes;
    if (!dvm_retirement_take(source, &value, &bytes) || value != entry ||
        bytes != entry->bytes || !dvm_retirement_commit(frames, &token, records))
    {
        abort();
    }
    free(entry->snapshot);
    free(entry);
    return DVM_RETIRE_OK;
}


/* Guarded incoming fields keep discovered children alive. Live candidates
 * can give up the queue owner here. Potential garbage retains its root for
 * the existing final confirmation path. */
/* Withdrawal clears the FIFO key and the corresponding queue root. */
static void take_cycle_candidate(DVM_RetirementQueue *source, Heap *heap)
{
    void *head;
    unsigned long bytes;
    if (!dvm_retirement_take(source, &head, &bytes) || head != H2D(void *, heap) ||
        !dvm_runtime_root_remove(heap))
    {
        abort();
    }
}


/* EN: Move candidate and its root from source to destination. Return zero if full.
 * RU: Передать candidate и его корень из source в destination. При отказе вернуть ноль. */
int dvm_rc_move_candidate(DVM_RetirementQueue *source,
    DVM_RetirementQueue *destination, void *candidate)
{
    Heap *heap = D2H(candidate);
    DVM_RetirementReservation token = {0};
    unsigned long bytes = poolmsize(heapmem, heap);
    DVM_HeapKey key = {0};
    int indexed = destination && (destination == duke_candidates || destination == cycle_refused);
    if (indexed)
    {
        key = dvm_heap_pool_key(heapmem, heap);
        if (!key.generation)
        {
            return 0;
        }
    }
    if (dvm_retirement_reserve(destination, bytes, &token) != DVM_RETIRE_OK)
    {
        return 0;
    }
    if (indexed)
    {
        fvm_port_sync_lock(candidate_transfer);
    }
    void *value;
    unsigned long prior_bytes;
    if (!dvm_retirement_take(source, &value, &prior_bytes) ||
        value != H2D(void *, heap) || prior_bytes != bytes ||
        !dvm_retirement_commit_key(destination, &token, value, key, NULL))
    {
        abort();
    }
    if (indexed)
    {
        fvm_port_sync_unlock(candidate_transfer);
    }
    if (notify_ready)
    {
        notify_ready();
    }
    return 1;
}


/* EN: Drop a nonfinal pin through the borrowed refcount argument.
 * RU: Снять непоследнее удержание через заимствованный счётчик argument. */
static int drop_duke_pin_nonfinal(void *argument)
{
    return dvm_refcount_drop_nonfinal_u(argument);
}


/* EN: Release up to budget abandoned Duke pins. stopped permits retained
 * destruction. Return nonzero when the journal is empty.
 * RU: Снять до budget удержаний отменённого обхода. stopped разрешает
 * резервное освобождение. Ненулевой результат означает пустой журнал. */
static int release_duke_pins(size_t budget, int stopped)
{
    while (duke_abandoned.pinned_nodes && budget)
    {
        void *value = duke_abandoned.nodes[duke_abandoned.pinned_nodes];
        Heap *heap = D2H(value);
        int transferred = 0;
        int fast_queued = 0;
        if (!stopped && dvm_runtime_root_remove_if(heap, drop_duke_pin_nonfinal, &heap->ref))
        {
            transferred = 1;
        }
        else if (!stopped)
        {
            unsigned long bytes = poolmsize(heapmem, heap);
            if (leaves && !heapmonitor && leaf_eligible(heap))
            {
                if (!defer_leaf(heap, bytes, 0))
                {
                    /* Fast can accept this object after queue pressure clears.
                     * Preserve Duke's reference and root until then. */
                    if (notify_ready)
                    {
                        notify_ready();
                    }
                    return 0;
                }
                __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
                fast_queued = 1;
            }
            else
            {
                DVM_RetirementReservation token = {0};
                if (dvm_retirement_reserve(retirement, bytes, &token) != DVM_RETIRE_OK)
                {
                    __atomic_store_n(&duke_cleanup_fallback, 1, __ATOMIC_RELEASE);
                    if (notify_ready)
                    {
                        notify_ready();
                    }
                    return 0;
                }
                if (!dvm_retirement_commit(retirement, &token, value))
                {
                    abort();
                }
                transferred = 1;
                __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
                if (notify_ready)
                {
                    notify_ready();
                }
            }
        }
        if (!transferred && !dvm_runtime_root_remove(heap))
        {
            abort();
        }
        if (fast_queued && notify_ready)
        {
            notify_ready();
        }
        if (stopped)
        {
            destroy(value);
        }
        duke_abandoned.pinned_nodes--;
        budget--;
    }
    if (duke_abandoned.pinned_nodes)
    {
        return 0;
    }
    free(duke_abandoned.owned_scratch);
    memset(&duke_abandoned, 0, sizeof(duke_abandoned));
    __atomic_store_n(&duke_cleanup_fallback, 0, __ATOMIC_RELEASE);
    return 1;
}


/* EN: Send the current Cycles candidate to the next collector after a hard refusal.
 * RU: Передать текущего кандидата Cycles следующему сборщику после отказа. */
static int route_refused_cycle(void)
{
    void *value = dvm_retirement_peek(cycle_candidates);
    if (duke_candidates && dvm_runtime_idle_cycles && !gchalt && !heapmonitor)
    {
        DVM_HeapKey key = dvm_heap_pool_key(heapmem, D2H(value));
        if (dvm_retirement_contains_key(duke_candidates, key))
        {
            return 0;
        }
        return dvm_rc_move_candidate(cycle_candidates, duke_candidates, value);
    }
    return cycle_refused && dvm_rc_move_candidate(cycle_candidates, cycle_refused, value);
}


/* EN: Try the source head under shared admission. cost counts work, busy signals retry.
 * RU: Проверить первый элемент source под параллельным допуском. cost учитывает работу, busy означает повтор. */
int dvm_rc_scan_candidate(DVM_RetirementQueue *source, size_t *cost, int *busy)
{
    *cost = 1;
    *busy = 0;
    if (source == duke_candidates && duke_abandoned.pinned_nodes && !release_duke_pins(64, 0))
    {
        *busy = 1;
        return 0;
    }
    void *value = dvm_retirement_peek(source);
    if (!dvm_runtime_idle_cycles || !value)
    {
        return 0;
    }
    Heap *heap = D2H(value);
    int resume = source == duke_candidates && duke_inflight.owned_scratch;
    if (!resume && dvm_refcount_load_u(&heap->ref) == 1)
    {
        if (!defer_leaf(heap, poolmsize(heapmem, heap), 0))
        {
            *busy = !gchalt && !heapmonitor;
            return 0;
        }
        take_cycle_candidate(source, heap);
        return 1;
    }
    if (!resume && dvm_runtime_root_count(heap) > 1 && dvm_refcount_drop_nonfinal_u(&heap->ref))
    {
        take_cycle_candidate(source, heap);
        dvm_runtime_gc_owner_lost();
        __atomic_fetch_add(&cycle_candidates_rooted, 1, __ATOMIC_RELAXED);
        __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
        return 1;
    }
    RecordGroupScratch scratch = {0};
    RecordGroup group = dvm_rc_empty_record_group(&scratch);
    group.growing = source == duke_candidates;
    if (resume)
    {
        if (duke_inflight.nodes[0] != value)
        {
            fputs("disvm Duke candidate changed between slices\n", stderr);
            abort();
        }
        group = duke_inflight;
    }
    else if (group.growing && !dvm_rc_grow_record_group(&group, DUKE_INITIAL_FIELDS))
    {
        return 0;
    }
    DVM_RangeAccess access = {0};
    int publishing = resume && duke_publish_pending;
    int scanned;
    if (publishing)
    {
        scanned = !gchalt && !heapmonitor;
    }
    else
    {
        if (dvm_range_access_begin(record_cells, NULL, 0, NULL, 0, &access) != DVM_CELL_ACQUIRED)
        {
            if (resume)
            {
                return DVM_CANDIDATE_RESUME;
            }
            free(group.owned_scratch);
            *busy = 1;
            return 0;
        }
        unsigned int previous_fields = group.edges_used;
        scanned = group.growing ?
            dvm_rc_duke_capture_slice(value, &group, &access) :
            dvm_rc_queued_record_group(value, 1, &group, &access);
        *cost += group.edges_used - previous_fields;
        if (scanned == DVM_DUKE_CAPTURE_RESUME)
        {
            if (!dvm_range_access_end(&access))
            {
                abort();
            }
            duke_inflight = group;
            duke_resumed_slices++;
            return DVM_CANDIDATE_RESUME;
        }
    }
    if (resume)
    {
        memset(&duke_inflight, 0, sizeof(duke_inflight));
        duke_publish_pending = 0;
    }
    unsigned int discovery_pins = group.pinned_nodes;
    int retry_promotion = 0;
    int promoted = scanned && !group.live[0] && defer_record_group(&group, 1, &retry_promotion);
    if (source == duke_candidates)
    {
        duke_promoted += promoted;
        if (promoted)
        {
            duke_adopted_pins += discovery_pins;
        }
        if (promoted && group.edges_used > duke_peak_fields)
        {
            duke_peak_fields = group.edges_used;
        }
    }
    __atomic_fetch_add(&shared_record_snapshots_with_peer,
        !publishing && scanned && dvm_world_has_active_guest(world), __ATOMIC_RELAXED);
    if (access.domain && !dvm_range_access_end(&access))
    {
        abort();
    }
    __atomic_fetch_add(&shared_record_scans, !publishing && scanned, __ATOMIC_RELAXED);
    if (group.growing && retry_promotion)
    {
        /* Fast will revalidate the fields. Keep the rooted snapshot intact
         * while its destination is temporarily unavailable. */
        duke_inflight = group;
        duke_publish_pending = 1;
        duke_publish_retries++;
        return DVM_CANDIDATE_RESUME;
    }
    *busy = group.busy || retry_promotion;
    int live = scanned && group.live[0];
    if (group.pinned_nodes)
    {
        duke_abandoned = group;
        if (!release_duke_pins(64, 0))
        {
            *busy = 1;
            return 0;
        }
    }
    else
    {
        free(group.owned_scratch);
    }
    if (group.oversized)
    {
        DVM_RetirementQueue *destination = source == cycle_candidates && duke_candidates &&
            !dvm_retirement_contains_key(duke_candidates, dvm_heap_pool_key(heapmem, heap)) ?
            duke_candidates : retirement;
        int moved = dvm_rc_move_candidate(source, destination, value);
        if (!moved && destination == duke_candidates)
        {
            *busy = 1;
        }
        if (source == cycle_candidates)
        {
            cycle_candidates_oversized += moved;
        }
        return moved;
    }
    if (!promoted && (!live || !dvm_refcount_drop_nonfinal_u(&D2H(value)->ref)))
    {
        return 0;
    }
    take_cycle_candidate(source, heap);
    if (!promoted)
    {
        dvm_runtime_gc_owner_lost();
        __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
    }
    return 1;
}


int dvm_runtime_rc_candidates_ready(void)
{
    return dvm_retirement_peek(cycle_candidates) != NULL;
}


unsigned int dvm_runtime_rc_shared_candidates(DVM_NativeContext *context, size_t budget)
{
    DVM_HeapOwner *reader = dvm_native_owner_current();
    if (!budget || !dvm_runtime_rc_candidates_ready() || context != dvm_native_context_current() ||
        !dvm_native_owner_attach_service(reader, world))
    {
        return 0;
    }
    unsigned int completed = 0;
    int retry = 0;
    if (dvm_native_owner_try_enter_idle_shared(reader))
    {
        DVM_RetirementStats queued = {0};
        (void)dvm_retirement_statistics(cycle_candidates, &queued);
        for (size_t index = 0; queued.accepting && index < queued.ready && budget; index++)
        {
            size_t cost;
            int busy;
            if (dvm_rc_scan_candidate(cycle_candidates, &cost, &busy))
            {
                completed++;
            }
            else if (!busy && route_refused_cycle())
            {
                completed++;
            }
            else
            {
                dvm_retirement_rotate(cycle_candidates);
                cycle_candidates_rotated++;
            }
            budget = cost < budget ? budget - cost : 0;
        }
        retry = queued.accepting && dvm_runtime_rc_candidates_ready();
        if (!dvm_native_owner_leave_idle_shared(reader))
        {
            abort();
        }
    }
    else
    {
        /* The lane retries after releasing its scheduler lease. */
        completed = 1;
    }
    if (!dvm_native_owner_detach(reader))
    {
        abort();
    }
    return completed + retry;
}


/* Report rooted work queued for the optional Duke collector. */
int dvm_runtime_rc_duke_ready(void)
{
    return dvm_retirement_peek(duke_candidates) != NULL;
}


/* EN: Run Duke on context with budget using the runtime-owned queues.
 * RU: Запустить ДюкGC на context с бюджетом budget и очередями рантайма. */
unsigned int dvm_runtime_rc_shared_duke(DVM_NativeContext *context, size_t budget)
{
    return dvm_gc_duke_run(context, budget, world, duke_candidates, retirement);
}


static int retirement_ready(int include_shared_retries); /* Queue metadata only. */


/* Withdraw the sole Fast consumer's head and its corresponding root.
 * The caller retains the entry metadata until its release is complete. */
static void take_shared_leaf(DVM_RetirementQueue *source, LeafRetirement *entry)
{
    void *value;
    unsigned long bytes;
    if (!dvm_retirement_take(source, &value, &bytes) || value != entry ||
        !dvm_runtime_root_remove(entry->heap))
    {
        abort();
    }
}


/* Try a bounded shared pass on the Fast thread. It never waits for admission
 * while serving leaves, retries and packet children. Cycles cannot run here. */
unsigned int dvm_runtime_rc_shared_leaves(DVM_NativeContext *context, size_t budget)
{
    if (!dvm_retirement_peek(leaves) && !dvm_retirement_peek(frames) &&
        !dvm_retirement_peek(record_retries))
    {
        return 0;
    }
    DVM_HeapOwner *reader = dvm_native_owner_current();
    if (context != dvm_native_context_current() || !dvm_native_owner_attach_service(reader, world))
    {
        return 0;
    }
    unsigned int completed = 0;
    int retry = 0;
    if (dvm_native_owner_try_enter_idle_shared(reader))
    {
        DVM_RetirementStats retry_stats = {0};
        (void)dvm_retirement_statistics(record_retries, &retry_stats);
        size_t retries = budget / 2 + budget % 2;
        if (retries > retry_stats.ready)
        {
            retries = retry_stats.ready;
        }
        while (budget && !heapmonitor)
        {
            DVM_RetirementQueue *source = leaves;
            if (retries)
            {
                retries--;
                source = record_retries;
            }
            LeafRetirement *entry = dvm_retirement_peek(source);
            if (!entry || !dvm_world_epoch_elapsed(world, entry->epoch))
            {
                if (source == record_retries)
                {
                    dvm_retirement_rotate(source);
                    continue;
                }
                if (entry && entry->records && move_leaf(source, record_retries, entry))
                {
                    budget--;
                    completed++;
                    continue;
                }
                break;
            }
            if (entry->records)
            {
                size_t cost = entry->records->count + entry->snapshot->edges_used;
                unsigned long final_epoch = 0;
                DVM_CellResult claim;
                if (entry->claimed)
                {
                    claim = DVM_CELL_ACQUIRED;
                }
                else if (entry->claim_refused)
                {
                    claim = DVM_CELL_REFUSED;
                }
                else if (entry->pinned_children && dvm_world_has_borrowers(world))
                {
                    claim = DVM_CELL_BUSY;
                }
                else
                {
                    int reserved;
                    claim = dvm_gc_fast_confirm_graph(entry->snapshot, record_cells, world,
                        entry->pinned_children ? &final_epoch : NULL, &reserved);
                    fast_reserved_confirmations += reserved;
                    if (claim == DVM_CELL_ACQUIRED)
                    {
                        fast_ignored_live_fields += entry->snapshot->omitted_live_fields;
                    }
                    budget = cost < budget ? budget - cost : 0;
                }
                if (claim != DVM_CELL_ACQUIRED)
                {
                    entry->claim_refused = claim != DVM_CELL_BUSY;
                    int retry = claim == DVM_CELL_BUSY;
                    if (entry->claim_refused)
                    {
                        DVM_RetirementResult release = release_refused_snapshot(source, entry);
                        if (release == DVM_RETIRE_OK)
                        {
                            budget -= budget != 0;
                            completed++;
                            continue;
                        }
                        retry = release == DVM_RETIRE_FULL || release == DVM_RETIRE_BYTES;
                    }
                    if (retry && source == record_retries)
                    {
                        dvm_retirement_rotate(source);
                        continue;
                    }
                    DVM_RetirementQueue *destination = retry ? record_retries : difficult;
                    if (!move_leaf(source, destination, entry))
                    {
                        if (source == record_retries)
                        {
                            continue;
                        }
                        break;
                    }
                    budget -= budget != 0;
                    completed++;
                    continue;
                }
                if (final_epoch)
                {
                    entry->claimed = 1;
                    entry->epoch = final_epoch;
                    budget -= budget != 0;
                    completed++;
                    if (source == record_retries)
                    {
                        dvm_retirement_rotate(source);
                        continue;
                    }
                    if (move_leaf(source, record_retries, entry))
                    {
                        continue;
                    }
                    break;
                }
                if (entry->claimed && !release_record_branches(entry->snapshot))
                {
                    if (source == record_retries)
                    {
                        dvm_retirement_rotate(source);
                        continue;
                    }
                    break;
                }
                void *value;
                unsigned long bytes;
                if (!dvm_retirement_take(source, &value, &bytes) || value != entry)
                {
                    abort();
                }
                shared_record_groups_with_peer += dvm_world_has_active_guest(world);
                shared_record_confirmations += entry->pinned_children;
                /* A claimed group must leave the traced heap as a unit. Charge
                 * the full group so another large reclamation waits its turn. */
                reclaim_detached_records(entry, 1);
                shared_record_groups++;
                free(entry);
                __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
                budget = cost < budget ? budget - cost : 0;
                completed++;
                continue;
            }
            if (!leaf_eligible(entry->heap))
            {
                break;
            }
            if (dvm_refcount_drop_nonfinal_u(&entry->heap->ref))
            {
                take_shared_leaf(source, entry);
                dvm_runtime_gc_owner_lost();
                free(entry);
                __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
                budget--;
                completed++;
                continue;
            }
            Type *mapped_channel = NULL;
            int channel = entry->heap->t == &Tchannel;
            Array *array = entry->heap->t == &Tarray ? H2D(Array *, entry->heap) : NULL;
            int has_children = array && (array->root != H || array->t->np);
            int record = entry->heap->t->free == freeheap;
            int list = entry->heap->t == &Tlist;
            int retry_leaf = 0;
            if ((channel && !claim_quiet_channel(entry->heap, &mapped_channel, &retry_leaf)) ||
                (has_children && !transfer_array_children(entry->heap, &retry_leaf)) ||
                ((record || list) && !transfer_object_fields(entry->heap, &retry_leaf)))
            {
                retry_leaf |= dvm_refcount_load_u(&entry->heap->ref) > 1;
                if (retry_leaf && source == record_retries)
                {
                    dvm_retirement_rotate(source);
                    continue;
                }
                if (!move_leaf(source, retry_leaf ? record_retries : difficult, entry))
                {
                    if (source == record_retries)
                    {
                        dvm_retirement_rotate(source);
                        continue;
                    }
                    break;
                }
                budget--;
                completed++;
                continue;
            }
            take_shared_leaf(source, entry);
            DVM_RefcountResult result = has_children || channel || record || list ? DVM_REFCOUNT_LAST :
                dvm_refcount_release_u(&entry->heap->ref);
            if (result == DVM_REFCOUNT_LAST)
            {
                Type *type = entry->heap->t;
                int peer = dvm_world_has_active_guest(world);
                if (type == &Tarray)
                {
                    release_owned_descriptor(H2D(Array *, entry->heap)->t);
                    shared_arrays++;
                    shared_array_graphs += has_children;
                    shared_arrays_with_peer += peer;
                }
                else if (channel)
                {
                    if (mapped_channel)
                    {
                        release_owned_descriptor(mapped_channel);
                    }
                    shared_channels++;
                }
                else if (list)
                {
                    Type *element = H2D(List *, entry->heap)->t;
                    if (element)
                    {
                        release_owned_descriptor(element);
                    }
                    shared_lists++;
                }
                else if (!record)
                {
                    free(H2D(String *, entry->heap)->tmp);
                    shared_strings++;
                    shared_strings_with_peer += peer;
                }
                if (record)
                {
                    release_owned_descriptor(type);
                    shared_plain_records++;
                }
                else if (dvm_refcount_release_i(&type->ref) != DVM_REFCOUNT_LIVE)
                {
                    abort();
                }
                poolfree(heapmem, entry->heap);
            }
            else if (result != DVM_REFCOUNT_LIVE)
            {
                abort();
            }
            free(entry);
            __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
            budget--;
            completed++;
        }
        if (!heapmonitor)
        {
            completed += drain_shared_children(budget);
            /* An accepting retry queue is outside the exclusive drain.
             * Ask for another paced shared pass if nothing else can run. */
            DVM_RetirementStats pending = {0};
            (void)dvm_retirement_statistics(record_retries, &pending);
            retry = !completed && pending.accepting && pending.ready &&
                !retirement_ready(0) && !dvm_managed_owner_pending();
        }
        if (!dvm_native_owner_leave_idle_shared(reader))
        {
            abort();
        }
    }

    if (!dvm_native_owner_detach(reader))
    {
        abort();
    }
    return completed + retry;
}


/* Revalidate under collector admission before mutating any edge. Keep one
 * owner per node through the whole unlink, then use ordinary destruction. */
static int process_dvm_rc_queued_record_group(void *value, int promote)
{
    RecordGroupScratch scratch = {0};
    RecordGroup group = dvm_rc_empty_record_group(&scratch);
    if (!dvm_rc_queued_record_group(value, 1, &group, NULL) || group.live[0])
    {
        return 0;
    }
    int retry;
    if (promote && defer_record_group(&group, 0, &retry))
    {
        return 1;
    }
    /* Clearing a view's root would make its legacy destructor treat borrowed
     * data as owned. Leave unpromoted views to the retained tracer. */
    if (group.has_views)
    {
        return 0;
    }
    unlink_record_group(&group);
    for (unsigned int index = 1; index < group.nodes_used; index++)
    {
        if (!group.live[index])
        {
            destroy(group.nodes[index]);
        }
    }
    record_groups++;
    return 0;
}


/* Count the retiring owner before a destructor can invalidate its header. */
static void fallback_destroy(void *value)
{
    Heap *heap = D2H(value);
    Type *type = heap->t;
    if (dvm_refcount_load_u(&heap->ref) > 1)
        fallback.nonfinal++;
    else if (type == &Tarray)
        fallback.arrays++;
    else if (type == &Tlist)
        fallback.lists++;
    else if (type == &Tchannel)
        fallback.channels++;
    else if (type == &Tstring)
        fallback.strings++;
    else if (type == &Tmodlink)
        fallback.modules++;
    else if (type && type->free == freeheap)
        fallback.records++;
    else
        fallback.other++;
    destroy(value);
}


/* [GC:] Release rooted queue owners only on a stopped-world collector. Each
 * destructor and its nested releases use the retained algorithm; they bypass
 * admission because the collector has no ACTIVE mutator ownership. Budget
 * bounds top-level owners, not the size of a recursively destroyed graph. */
unsigned int dvm_runtime_rc_drain(unsigned int budget)
{
    DVM_RetirementStats stats = {0};
    void *value;
    unsigned long bytes;
    if (!dvm_world_collector_owns(world))
    {
        fputs("disvm RC retirement outside stopped collector\n", stderr);
        abort();
    }
    fallback.passes++;
    if (!accepting && duke_inflight.owned_scratch)
    {
        if (duke_abandoned.owned_scratch)
        {
            abort();
        }
        duke_abandoned = duke_inflight;
        memset(&duke_inflight, 0, sizeof(duke_inflight));
        duke_publish_pending = 0;
    }
    if (budget && (!accepting || __atomic_load_n(&duke_cleanup_fallback, __ATOMIC_ACQUIRE)) &&
        duke_abandoned.owned_scratch)
    {
        size_t pins = duke_abandoned.pinned_nodes;
        (void)release_duke_pins(budget, 1);
        size_t consumed = pins - duke_abandoned.pinned_nodes;
        budget -= consumed;
    }
    while (budget && ((!accepting && dvm_retirement_take(record_retries, &value, &bytes)) ||
                      dvm_retirement_take(difficult, &value, &bytes) ||
                      ((!dvm_runtime_idle_cycles || !accepting || heapmonitor) &&
                       dvm_retirement_take(leaves, &value, &bytes))))
    {
        LeafRetirement *entry = value;
        budget--;
        if (entry->records)
        {
            fallback.groups++;
            reclaim_detached_records(entry, 0);
        }
        else
        {
            ptrdel(entry->heap);
            fallback_destroy(H2D(void *, entry->heap));
        }
        free(entry);
        __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
    }
    /* During execution only individually refused candidates reach this pass.
     * The monitor and shutdown may drain the remaining shared queue. */
    int direct_release;
    while (budget && ((direct_release = dvm_retirement_take(retirement, &value, &bytes) ||
                       (!accepting && dvm_retirement_take(duke_candidates, &value, &bytes))) ||
                      dvm_retirement_take(cycle_refused, &value, &bytes) ||
                      ((!accepting || heapmonitor) &&
                       dvm_retirement_take(cycle_candidates, &value, &bytes))))
    {
        (void)bytes;
        budget--;
        /* Direct entries held the last owner or exhausted the snapshot
         * budget in Cycles. The retained tracer handles oversized closure. */
        int promoted = !direct_release && process_dvm_rc_queued_record_group(value, 1);
        ptrdel(D2H(value));
        fallback.promoted += promoted;
        if (!promoted)
        {
            fallback_destroy(value);
            __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
        }
    }
    /* Charge individual children so one frame cannot hold the idle heap
     * across all of its nested releases. Shared Fast can resume the suffix. */
    while (budget)
    {
        FrameRetirement *packet = dvm_retirement_peek(frames);
        if (!packet)
        {
            break;
        }
        while (budget && packet->next < packet->count)
        {
            size_t index = packet->next++;
            budget--;
            Heap *heap = packet->keys[index];
            void *child = H2D(void *, heap);
            int promoted = 0;
            int transferred = 0;
            if (dvm_runtime_idle_cycles && accepting && !heapmonitor)
            {
                if (dvm_refcount_load_u(&heap->ref) == 1 &&
                    defer_leaf(heap, poolmsize(heapmem, heap), 1))
                {
                    __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);
                    promoted = 1;
                }
                else
                {
                    transferred = transfer_record_candidate(heap, packet, index);
                    promoted = transferred;
                }
            }
            if (!promoted)
            {
                process_dvm_rc_queued_record_group(child, 0);
            }
            if (!transferred && !dvm_runtime_root_batch_remove(packet->roots, index))
            {
                abort();
            }
            if (promoted)
            {
                fallback.packet_promoted++;
            }
            else
            {
                fallback_destroy(child);
            }
        }
        if (packet->next != packet->count)
        {
            break;
        }
        if (!dvm_retirement_take(frames, &value, &bytes) || value != packet ||
            !dvm_runtime_root_batch_clear(packet->roots) ||
            !dvm_runtime_root_batch_destroy(&packet->roots))
        {
            abort();
        }
        free(packet);
        frames_drained++;
    }
    DVM_RetirementStats frame_stats = {0};
    (void)dvm_retirement_statistics(frames, &frame_stats);
    (void)dvm_retirement_statistics(retirement, &stats);
    return stats.ready + stats.reserved + frame_stats.ready + frame_stats.reserved +
        (dvm_retirement_peek(leaves) != NULL) + (dvm_retirement_peek(difficult) != NULL) + (dvm_retirement_peek(record_retries) != NULL) + (dvm_retirement_peek(cycle_refused) != NULL) + dvm_runtime_rc_candidates_ready() +
        (!accepting && (dvm_runtime_rc_duke_ready() || duke_abandoned.pinned_nodes)) +
        __atomic_load_n(&duke_cleanup_fallback, __ATOMIC_ACQUIRE);
}


/* [GC:] Read queue occupancy on its active Dis owner. Collectors cannot mutate
 * it until that owner parks/leaves. Every ACTIVE executor of this world can
 * request Fast work; native/idle/foreign/collector callers receive zero. */
unsigned int dvm_runtime_rc_pending(void)
{
    DVM_RetirementStats stats = {0};
    if (admitted(0))
    {
        DVM_RetirementStats frame_stats = {0};
        (void)dvm_retirement_statistics(frames, &frame_stats);
        (void)dvm_retirement_statistics(retirement, &stats);
        return stats.ready + stats.reserved + frame_stats.ready + frame_stats.reserved +
        (dvm_retirement_peek(leaves) != NULL) + (dvm_retirement_peek(difficult) != NULL) + (dvm_retirement_peek(record_retries) != NULL) + (dvm_retirement_peek(cycle_refused) != NULL) + dvm_runtime_rc_candidates_ready() + dvm_runtime_rc_duke_ready();
    }
    return stats.ready+stats.reserved;
}


/* Check only committed native entries. Queue storage stays live through
 * worker join, including during shutdown. No managed object is inspected. */
static int retirement_ready(int include_shared_retries)
{
    DVM_RetirementStats refs = {0};
    DVM_RetirementStats packets = {0};
    DVM_RetirementStats candidates = {0};
    DVM_RetirementStats duke = {0};
    (void)dvm_retirement_statistics(duke_candidates, &duke);
    DVM_RetirementStats retries = {0};
    (void)dvm_retirement_statistics(record_retries, &retries);
    (void)dvm_retirement_statistics(cycle_candidates, &candidates);
    (void)dvm_retirement_statistics(retirement, &refs);
    (void)dvm_retirement_statistics(frames, &packets);
    return __atomic_load_n(&duke_cleanup_fallback, __ATOMIC_ACQUIRE) ||
        refs.ready != 0 || packets.ready != 0 || (duke.ready && !duke.accepting) ||
        dvm_retirement_peek(leaves) != NULL || dvm_retirement_peek(difficult) != NULL ||
        (retries.ready && (include_shared_retries || !retries.accepting)) ||
        dvm_retirement_peek(cycle_refused) != NULL ||
        (candidates.ready && !candidates.accepting);
}


/* Report pending Fast work, including snapshots queued for shared retry. */
int dvm_runtime_rc_ready(void)
{
    return retirement_ready(1);
}


/* [GC:] Close with the mutator active, or after sealing with every collector idle.
 * Existing queued references stay rooted until Fast resumes. */
void dvm_runtime_rc_close(void)
{
    accepting = 0;
    dvm_retirement_close(retirement);
    dvm_retirement_close(cycle_candidates);
    dvm_retirement_close(cycle_refused);
    dvm_retirement_close(duke_candidates);
    dvm_retirement_close(frames);
    dvm_retirement_close(leaves);
    dvm_retirement_close(difficult);
    dvm_retirement_close(record_retries);
}


/* [GC:] Called after join, before releasing mutator TLS. No collector accesses
 * counters or queue storage now. Nonempty queues are reported as failure and
 * retain their roots instead of silently losing managed owners. */
int dvm_runtime_rc_finish(int diagnostics)
{
    DVM_RetirementStats stats = {0};
    int result;
    (void)dvm_retirement_statistics(retirement, &stats);
    if (diagnostics)
    {
        printf("NATIVERC enqueued=%lu released=%lu pending=%u saturated=%lu bytes=%lu byte_limited=%lu helper_enqueued=%lu helper_saturated=%lu helper_byte_limited=%lu\n",
            enqueued,released,stats.ready+stats.reserved,saturated,stats.bytes,byte_limited,
            helper_enqueued,helper_saturated,helper_byte_limited);
    }
    DVM_RetirementStats frame_stats = {0};
    (void)dvm_retirement_statistics(frames, &frame_stats);
    if (diagnostics)
    {
        DVM_RetirementStats candidates = {0};
        (void)dvm_retirement_statistics(cycle_candidates, &candidates);
        printf("NATIVECYCLEQUEUE ready=%u reserved=%u capacity=%u peak_capacity=%u\n",
            candidates.ready, candidates.reserved, candidates.capacity, candidates.peak_capacity);
        printf("NATIVEFASTGC ignored_live_fields=%zu reserved_confirmations=%zu\n",
            fast_ignored_live_fields, fast_reserved_confirmations);
        printf("NATIVEDUKEGC promoted=%lu peak_fields=%u resumed=%lu adopted_pins=%lu publish_retries=%lu\n",
            duke_promoted, duke_peak_fields, duke_resumed_slices, duke_adopted_pins, duke_publish_retries);
        printf("NATIVEFRAMERC drained=%lu pending=%u shared_children=%lu record_groups=%lu shared_record_groups=%lu shared_record_groups_with_peer=%lu shared_record_scans=%lu shared_record_snapshots_with_peer=%lu shared_record_confirmations=%lu cycle_candidates_rotated=%lu cycle_candidates_coalesced=%lu cycle_candidates_rooted=%lu cycle_candidates_oversized=%lu cycle_candidates_full=%lu\n",
            frames_drained, frame_stats.ready + frame_stats.reserved, shared_children, record_groups, shared_record_groups, shared_record_groups_with_peer, shared_record_scans, shared_record_snapshots_with_peer, shared_record_confirmations, cycle_candidates_rotated, cycle_candidates_coalesced, cycle_candidates_rooted, cycle_candidates_oversized, cycle_candidates_full);
    }
    if (diagnostics)
    {
        printf("NATIVELISTRC shared=%lu\n", shared_lists);
        printf("NATIVERECORDRC shared=%lu\n", shared_plain_records);
        printf("NATIVECHANNELRC shared=%lu\n", shared_channels);
        printf("NATIVESTRINGRC shared=%lu with_peer=%lu\n", shared_strings, shared_strings_with_peer);
        printf("NATIVEARRAYRC shared=%lu with_peer=%lu graphs=%lu\n",
            shared_arrays, shared_arrays_with_peer, shared_array_graphs);
    }
    if (diagnostics)
    {
        printf("NATIVERCPASSES idle=%lu promoted=%lu packet_promoted=%lu groups=%lu nonfinal=%lu records=%lu arrays=%lu lists=%lu channels=%lu strings=%lu modules=%lu other=%lu\n",
            fallback.passes, fallback.promoted, fallback.packet_promoted, fallback.groups, fallback.nonfinal,
            fallback.records, fallback.arrays, fallback.lists, fallback.channels,
            fallback.strings, fallback.modules, fallback.other);
        DVM_RetirementStats difficult_stats = {0};
        (void)dvm_retirement_statistics(difficult, &difficult_stats);
        printf("NATIVERCFALLBACK moved=%lu pending=%u\n", difficult_moved, difficult_stats.ready + difficult_stats.reserved);
    }
    result = !duke_inflight.owned_scratch && !duke_abandoned.pinned_nodes && (duke_candidates == NULL || dvm_retirement_destroy(&duke_candidates)) &&
        (cycle_candidates == NULL || dvm_retirement_destroy(&cycle_candidates)) &&
        (cycle_refused == NULL || dvm_retirement_destroy(&cycle_refused)) &&
        (record_retries == NULL || dvm_retirement_destroy(&record_retries)) &&
        (difficult == NULL || dvm_retirement_destroy(&difficult)) &&
        (leaves == NULL || dvm_retirement_destroy(&leaves)) &&
        (retirement == NULL || dvm_retirement_destroy(&retirement)) &&
        (frames == NULL || dvm_retirement_destroy(&frames));
    if (result)
    {
        if (candidate_transfer)
        {
            fvm_port_sync_destroy(candidate_transfer);
            candidate_transfer = NULL;
        }
        owner = NULL;
        world = NULL;
        notify_ready = NULL;
    }
    return result;
}
