/* A bounded last-owner queue; all heap work remains under native_world. */
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
#include "runtime_gc.h"
#include "gc_fields.h"
#include "managed_owner.h"
#include "disvm/range_access.h"
#include "retirement_queue.h"
#include "root_batch.h"
#include "root_registry.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RC_BYTES (4UL*1024UL*1024UL) /* Bound queued direct block bytes, excluding descendants. */
#define RC_CAPACITY 512 /* Fixed native slots; saturation falls back to inline RC. */

/* A packet owns each captured reference through the collector's drain. */
typedef struct FrameRetirement
{
    DVM_RootBatch *roots; /* Published root occurrences for keys. */
    size_t count;         /* Number of transferred owners. */
    size_t next;          /* First owner not yet consumed by Fast. */
    void *keys[];        /* Heap headers in frame-map order. */
} FrameRetirement;

typedef struct RecordGroup RecordGroup;

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

static DVM_RetirementQueue *cycle_candidates; /* Nonfinal owners for the Cycles lane. */
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
static unsigned long shared_record_confirmations;
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
static int admitted(void)
{
    DVM_HeapOwner *current = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    return current && dvm_native_owner_statistics(current, &state) &&
        state.phase == DVM_HEAP_ACTIVE && !state.pending_progress &&
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
    if (retirement || frames || leaves || difficult || record_retries || cycle_candidates)
    {
        fprintf(stderr, "disvm RC queue restarted before successful finish\n");
        abort();
    }
    retirement = dvm_retirement_create(RC_CAPACITY, RC_BYTES);
    frames = dvm_retirement_create(RC_CAPACITY, RC_BYTES);
    leaves = dvm_retirement_create(RC_CAPACITY, RC_BYTES);
    difficult = dvm_retirement_create(RC_CAPACITY, RC_BYTES);
    record_retries = dvm_retirement_create(RC_CAPACITY, RC_BYTES);
    cycle_candidates = dvm_retirement_create_indexed(RC_CAPACITY, RC_BYTES);
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
    shared_record_confirmations = 0;
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
extern int dvm_runtime_channel_is_quiet(Channel *channel);


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
    if (!leaves || heapmonitor || !leaf_eligible(heap) || bytes > RC_BYTES)
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


#define RECORD_GROUP_LIMIT 64
#define RECORD_EDGE_LIMIT 256
#define RECORD_INDEX_SLOTS (RECORD_GROUP_LIMIT * 2)

struct RecordGroup
{
    void *nodes[RECORD_GROUP_LIMIT];
    unsigned int incoming[RECORD_GROUP_LIMIT];
    unsigned int index[RECORD_INDEX_SLOTS]; /* Stored node index plus one. */
    void *cells[RECORD_EDGE_LIMIT];
    unsigned int targets[RECORD_EDGE_LIMIT];
    unsigned int sources[RECORD_EDGE_LIMIT];
    unsigned char live[RECORD_GROUP_LIMIT];
    unsigned int live_count;
    int has_views; /* A detached view needs descriptor-only destruction. */
    unsigned int nodes_used;
    unsigned int edges_used;
};


/* The table stays at most half full. Every indexed object is held live. */
static unsigned int record_group_index(RecordGroup *group, void *value)
{
    uintptr_t address = (uintptr_t)value / sizeof(void *);
    unsigned int slot = (address ^ (address >> 7)) % RECORD_INDEX_SLOTS;
    while (group->index[slot])
    {
        unsigned int index = group->index[slot] - 1;
        if (group->nodes[index] == value)
        {
            return index;
        }
        slot = (slot + 1) % RECORD_INDEX_SLOTS;
    }
    if (group->nodes_used == RECORD_GROUP_LIMIT)
    {
        return RECORD_GROUP_LIMIT;
    }
    unsigned int index = group->nodes_used++;
    group->nodes[index] = value;
    group->index[slot] = index + 1;
    return index;
}


/* The held source field preserves the target while the graph is scanned. */
static int record_group_edge(RecordGroup *group, unsigned int index, void *cell,
    DVM_RangeAccess *access)
{
    void *edge;
    if (access && dvm_range_access_extend(access, cell, sizeof(edge)) != DVM_CELL_ACQUIRED)
    {
        return 0;
    }
    memcpy(&edge, cell, sizeof(edge));
    if (!edge || group->edges_used == RECORD_EDGE_LIMIT)
    {
        return 0;
    }
    group->cells[group->edges_used] = cell;
    group->sources[group->edges_used] = index;
    if (edge == H)
    {
        group->targets[group->edges_used++] = RECORD_GROUP_LIMIT;
        return 1;
    }
    unsigned int target = record_group_index(group, edge);
    if (target == RECORD_GROUP_LIMIT)
    {
        return 0;
    }
    group->incoming[target]++;
    group->targets[group->edges_used++] = target;
    return 1;
}


/* Snapshot ordinary records and owning arrays. Opaque targets are
 * live boundaries. The caller owns start through its queue root. */
static int queued_record_group(void *start, unsigned long start_roots, RecordGroup *group,
    DVM_RangeAccess *access)
{
    if (!dvm_runtime_idle_cycles || gchalt || heapmonitor)
    {
        return 0;
    }
    (void)record_group_index(group, start);
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        void *record = group->nodes[index];
        Heap *heap = D2H(record);
        Type *type = heap->t;
        unsigned char *data = record;
        size_t elements = 1;
        if (type == &Tarray)
        {
            Array *array = record;
            if (!array->root || !array->t)
            {
                return 0;
            }
            if (array->root != H)
            {
                group->has_views = 1;
                type = &Tptr;
                data = (unsigned char *)&array->root;
            }
            else
            {
                if (array->len < 0 || array->len > RECORD_EDGE_LIMIT ||
                    array->data != (unsigned char *)array + sizeof(*array))
                {
                    group->live[index] = 1;
                    continue;
                }
                type = array->t;
                elements = array->len;
                data = array->data;
            }
        }
        if (heap->t == &Tlist)
        {
            List *list = record;
            if (!record_group_edge(group, index, &list->tail, access))
            {
                return 0;
            }
            type = list->t;
            data = (unsigned char *)list->data;
            elements = type ? 1 : 0;
        }
        if (heap->t == &Tchannel)
        {
            Channel *channel = record;
            if (!dvm_runtime_channel_is_quiet(channel))
            {
                group->live[index] = 1;
                continue;
            }
            type = &Tptr;
            data = (unsigned char *)&channel->buf;
        }
        if (heap->t != &Tlist && (!type || type->size < (int)sizeof(void *) ||
            (heap->t != &Tarray && heap->t != &Tchannel && type->free != freeheap)))
        {
            group->live[index] = 1;
            continue;
        }
        if (dvm_runtime_root_count(heap) != (index == 0 ? start_roots : 0))
        {
            group->live[index] = 1;
        }
        for (size_t element = 0; element < elements; element++)
        {
            for (int byte = 0; byte < type->np; byte++)
            {
                for (unsigned int bit = 0; bit < 8; bit++)
                {
                    if (!(type->map[byte] & (0x80U >> bit)))
                    {
                        continue;
                    }
                    size_t slot = (size_t)byte * 8 + bit;
                    void *edge;
                    if (slot >= (size_t)type->size / sizeof(edge))
                    {
                        return 0;
                    }
                    void *cell = data + element * type->size + slot * sizeof(edge);
                    if (!record_group_edge(group, index, cell, access))
                    {
                        return 0;
                    }
                }
            }
        }
    }
    if (!group->incoming[0])
    {
        if (!access)
        {
            return 0;
        }
        group->live[0] = 1;
    }
    unsigned int ready[RECORD_GROUP_LIMIT];
    unsigned int starts[RECORD_GROUP_LIMIT + 1];
    unsigned int edge = 0;
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        Heap *heap = D2H(group->nodes[index]);
        unsigned long expected = group->incoming[index] + (index == 0);
        unsigned long actual = dvm_refcount_load_u(&heap->ref);
        if (actual < expected)
        {
            return 0;
        }
        group->live[index] |= actual > expected;
        if (group->live[index])
        {
            ready[group->live_count++] = index;
        }
        starts[index] = edge;
        while (edge < group->edges_used && group->sources[edge] == index)
        {
            edge++;
        }
    }
    starts[group->nodes_used] = edge;

    /* Edges are captured in source order. Visit each live node's outgoing
     * range once, even when reachability runs against discovery order. */
    for (unsigned int head = 0; head < group->live_count; head++)
    {
        unsigned int source = ready[head];
        for (edge = starts[source]; edge < starts[source + 1]; edge++)
        {
            unsigned int target = group->targets[edge];
            if (target != RECORD_GROUP_LIMIT && !group->live[target])
            {
                group->live[target] = 1;
                ready[group->live_count++] = target;
            }
        }
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
        if (group->targets[index] == RECORD_GROUP_LIMIT || group->live[group->sources[index]])
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


static int defer_record_group(const RecordGroup *group, int shared)
{
    if (dvm_world_has_borrowers(world))
    {
        return 0;
    }
    DVM_RetirementReservation token = {0};
    unsigned long bytes = 0;
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        unsigned long size = poolmsize(heapmem, D2H(group->nodes[index]));
        if (size > RC_BYTES - bytes)
        {
            return 0;
        }
        bytes += size;
    }
    if (dvm_retirement_reserve(leaves, bytes, &token) != DVM_RETIRE_OK)
    {
        return 0;
    }
    LeafRetirement *entry = malloc(sizeof(*entry));
    FrameRetirement *records = malloc(sizeof(*records) + group->nodes_used * sizeof(void *));
    RecordGroup *snapshot = malloc(sizeof(*snapshot));
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
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        records->keys[index] = D2H(group->nodes[index]);
    }
    *snapshot = *group;
    int pin_children = shared || group->live_count || group->has_views;
    if (pin_children)
    {
        for (unsigned int index = 1; index < group->nodes_used; index++)
        {
            if (dvm_refcount_retain_u(&D2H(group->nodes[index])->ref) != DVM_REFCOUNT_LIVE)
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


/* Ordinary records and bounded owning arrays with managed fields. */
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
            (array->len > 0 && array->len <= RECORD_EDGE_LIMIT && array->t->np));
    }
    return heap->t == &Tlist || heap->t == &Tchannel || (heap->t->free == freeheap && heap->t->np);
}


/* Producers are exclusive Dis owners or the single Fast lane. A ready
 * entry holds the same allocation alive through this observation. */
static int cycle_candidate_already_queued(DVM_HeapKey key)
{
    int queued = dvm_retirement_contains_key(cycle_candidates, key);
    cycle_candidates_coalesced += queued;
    return queued;
}


/* Keep the packet owner intact until its replacement queue root is published. */
static int transfer_record_candidate(Heap *heap)
{
    if (!record_candidate(heap) || dvm_refcount_load_u(&heap->ref) <= 1)
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
    if (dvm_retirement_reserve(cycle_candidates, bytes, &token) != DVM_RETIRE_OK)
    {
        return 0;
    }
    if (!dvm_gc_field_mark(&heap->color, &mutator, &nprop) || !dvm_runtime_root_add(heap))
    {
        dvm_retirement_cancel(cycle_candidates, &token);
        return 0;
    }
    int became_ready;
    if (!dvm_retirement_commit_key(cycle_candidates, &token, H2D(void *, heap), key, &became_ready))
    {
        abort();
    }
    if (became_ready && notify_ready)
    {
        notify_ready();
    }
    enqueued++;
    helper_enqueued++;
    return 1;
}


/* [GC:] value has passed destroy's retained block check. Keep its retiring reference
 * as queue ownership rather than publishing an unrooted zero-count object.
 * A failed ptradd leaves ownership untouched and restores the caller error text,
 * allowing ordinary inline destruction. Reserve precedes root setup; metadata
 * mutex never spans ptradd/Setmark/errors or referent destruction. Mutable
 * statistics/accepting and all managed source access still require world exclusion. */
int dvm_runtime_rc_defer(void *value)
{
    Heap *heap;
    unsigned long bytes;
    char previous[ERRMAX];
    int helper;
    DVM_RetirementReservation token = {0};
    DVM_RetirementResult reserved;
    if (!admitted() || !accepting)
    {
        return 0;
    }
    helper = dvm_native_context_current() != owner;
    heap = D2H(value);
    int last = dvm_refcount_load_u(&heap->ref) == 1;
    DVM_HeapKey key = {0};
    if (!last)
    {
        if (!record_candidate(heap))
        {
            return 0;
        }
        key = dvm_heap_pool_key(heapmem, heap);
        if (!key.generation || cycle_candidate_already_queued(key))
        {
            return 0;
        }
    }
    bytes = poolmsize(heapmem,heap);
    if (last && defer_leaf(heap, bytes, 1))
    {
        enqueued++;
        helper_enqueued += helper != 0;
        return 1;
    }
    DVM_RetirementQueue *queue = last ? retirement : cycle_candidates;
    reserved = dvm_retirement_reserve(queue, bytes, &token);
    if (reserved != DVM_RETIRE_OK)
    {
        if (reserved == DVM_RETIRE_FULL)
        {
            saturated++;
            helper_saturated += helper != 0;
        }
        else if (reserved == DVM_RETIRE_BYTES)
        {
            byte_limited++;
            helper_byte_limited += helper != 0;
        }
        return 0;
    }
    memcpy(previous,up->env->errstr,sizeof(previous));
    if (waserror())
    {
        (void)dvm_retirement_cancel(queue, &token);
        memcpy(up->env->errstr,previous,sizeof(previous));
        return 0;
    }
    ptradd(heap);
    poperror();
    Setmark(heap);
    int became_ready;
    if (!dvm_retirement_commit_key(queue, &token, value, key, &became_ready))
    {
        (void)dvm_retirement_cancel(queue, &token);
        ptrdel(heap);
        return 0;
    }
    enqueued++;
    helper_enqueued += helper != 0;
    if (became_ready && notify_ready)
    {
        notify_ready();
    }
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
    if (direct > RC_BYTES)
    {
        return 0;
    }
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
                    if (size > RC_BYTES - direct)
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
    if (capacity > (RC_BYTES - sizeof(FrameRetirement)) / sizeof(void *))
    {
        return 0;
    }
    size_t bytes = sizeof(FrameRetirement) + capacity * sizeof(void *);
    if (bytes > RC_BYTES - direct)
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


/* Prepare the complete child packet before claiming the parent. A refusal
 * leaves the original array, queue entry and reference count unchanged. */
static int transfer_array_children(Heap *heap)
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
            (size_t)type->np > ((size_t)type->size / sizeof(void *) + 7) / 8 ||
            (size_t)array->len > RC_BYTES / (size_t)type->np / 8 / sizeof(void *))
        {
            return 0;
        }
        capacity = (size_t)array->len * type->np * 8;
    }
    if (!capacity)
    {
        return dvm_refcount_claim_last_u(&heap->ref);
    }
    size_t storage = sizeof(FrameRetirement) + capacity * sizeof(void *);
    if (storage > RC_BYTES)
    {
        return 0;
    }
    FrameRetirement *packet = malloc(storage);
    if (!packet)
    {
        return 0;
    }
    packet->roots = dvm_runtime_root_batch_create(capacity);
    packet->count = 0;
    packet->next = 0;
    int valid = packet->roots != NULL;
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
    unsigned long charge = (unsigned long)storage;
    for (size_t index = 0; valid && index < packet->count; index++)
    {
        unsigned long size = poolmsize(heapmem, packet->keys[index]);
        if (size > RC_BYTES - charge)
        {
            valid = 0;
        }
        else
        {
            charge += size;
        }
    }
    DVM_RetirementReservation token = {0};
    int reserved = valid && dvm_retirement_reserve(frames, charge, &token) == DVM_RETIRE_OK;
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
            if (!transfer_record_candidate(heap))
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
                    enqueued++;
                    helper_enqueued++;
                }
            }
            if (!dvm_runtime_root_batch_remove(packet->roots, packet->next))
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


/* Reserve every count before freeing any storage. Roll back in reverse order
 * on refusal, while the rooted queue entry still owns the group. */
static int claim_record_group(const RecordGroup *group)
{
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        if (group->live[index])
        {
            continue;
        }
        Heap *heap = D2H(group->nodes[index]);
        if (!dvm_refcount_replace_u(&heap->ref, group->incoming[index] + (index == 0), 0))
        {
            while (index)
            {
                index--;
                if (group->live[index])
                {
                    continue;
                }
                heap = D2H(group->nodes[index]);
                if (!dvm_refcount_replace_u(&heap->ref, 0, group->incoming[index] + (index == 0)))
                {
                    abort();
                }
            }
            return 0;
        }
    }
    return 1;
}


/* Re-read the entire edge set under the guest's canonical cell guards.
 * The rooted entry pins storage. Keep every acquired stripe through the count
 * claim so a guarded reader cannot observe a partially claimed graph. */
static DVM_CellResult validate_and_claim_record_group(const RecordGroup *group, unsigned long *final_epoch)
{
    DVM_RangeAccess access = {0};
    DVM_CellResult result = dvm_range_access_begin(record_cells, NULL, 0, NULL, 0, &access);
    if (result != DVM_CELL_ACQUIRED)
    {
        return result;
    }
    for (unsigned int index = 0; index < group->edges_used; index++)
    {
        result = dvm_range_access_extend(&access, group->cells[index], sizeof(void *));
        if (result != DVM_CELL_ACQUIRED)
        {
            break;
        }
        void *edge;
        memcpy(&edge, group->cells[index], sizeof(edge));
        unsigned int target = group->targets[index];
        if (edge != (target == RECORD_GROUP_LIMIT ? H : group->nodes[target]))
        {
            result = DVM_CELL_REFUSED;
            break;
        }
    }
    for (unsigned int index = 0; result == DVM_CELL_ACQUIRED && index < group->nodes_used; index++)
    {
        Heap *heap = D2H(group->nodes[index]);
        if (!group->live[index] && (dvm_runtime_root_count(heap) != 1 ||
            (heap->t == &Tchannel && !dvm_runtime_channel_is_quiet(H2D(Channel *, heap)))))
        {
            result = DVM_CELL_REFUSED;
        }
    }
    /* Issue the final epoch under the same guards as the count withdrawal.
     * Guarded readers cannot start between the epoch and the claim. */
    if (result == DVM_CELL_ACQUIRED && final_epoch &&
        !(*final_epoch = dvm_world_retire_epoch(world)))
    {
        result = DVM_CELL_REFUSED;
    }
    if (result == DVM_CELL_ACQUIRED && !claim_record_group(group))
    {
        result = DVM_CELL_REFUSED;
    }
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return result;
}


/* Reserve and root existing owners before the caller commits a transfer. */
static FrameRetirement *prepare_release_packet(void *const *keys, size_t count,
    DVM_RetirementReservation *token)
{
    unsigned long charge = sizeof(FrameRetirement) + count * sizeof(void *);
    for (size_t index = 0; index < count; index++)
    {
        unsigned long size = poolmsize(heapmem, keys[index]);
        if (size > RC_BYTES - charge)
        {
            return NULL;
        }
        charge += size;
    }
    if (dvm_retirement_reserve(frames, charge, token) != DVM_RETIRE_OK)
    {
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


/* Transfer one discovery hold per live node plus each dead-to-live edge.
 * The original rooted entry stays intact if packet setup cannot complete. */
static int release_record_branches(const RecordGroup *group)
{
    if (!group->live_count)
    {
        return 1;
    }
    void *keys[RECORD_GROUP_LIMIT + RECORD_EDGE_LIMIT];
    size_t count = 0;
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        if (group->live[index])
        {
            keys[count++] = D2H(group->nodes[index]);
        }
    }
    for (unsigned int edge = 0; edge < group->edges_used; edge++)
    {
        unsigned int target = group->targets[edge];
        if (target != RECORD_GROUP_LIMIT && group->live[target] && !group->live[group->sources[edge]])
        {
            keys[count++] = D2H(group->nodes[target]);
        }
    }
    DVM_RetirementReservation token = {0};
    FrameRetirement *packet = prepare_release_packet(keys, count, &token);
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
static int transfer_object_fields(Heap *heap)
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
    FrameRetirement *packet = prepare_release_packet(keys, count, &token);
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


static int claim_quiet_channel(Heap *heap, Type **mapped)
{
    Array *buffer = H2D(Channel *, heap)->buf;
    if (buffer == H)
    {
        return dvm_runtime_channel_claim_quiet(heap, buffer, mapped, 0);
    }
    if (!buffer)
    {
        return 0;
    }
    void *key = D2H(buffer);
    DVM_RetirementReservation token = {0};
    FrameRetirement *packet = prepare_release_packet(&key, 1, &token);
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
            if (target != RECORD_GROUP_LIMIT && snapshot->live[target] &&
                !snapshot->live[snapshot->sources[edge]])
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


/* Guarded incoming fields keep discovered children alive. Live candidates
 * can give up the queue owner here. Potential garbage retains its root for
 * the existing final confirmation path. */
static int scan_retiring_record(void)
{
    void *value = dvm_retirement_peek(cycle_candidates);
    if (!dvm_runtime_idle_cycles || !value)
    {
        return 0;
    }
    Heap *heap = D2H(value);
    if (dvm_refcount_load_u(&heap->ref) == 1)
    {
        if (!defer_leaf(heap, poolmsize(heapmem, heap), 0))
        {
            return 0;
        }
        void *head;
        unsigned long bytes;
        if (!dvm_retirement_take(cycle_candidates, &head, &bytes) || head != value ||
            !dvm_runtime_root_remove(heap))
        {
            abort();
        }
        return 1;
    }
    RecordGroup group = {0};
    DVM_RangeAccess access = {0};
    if (dvm_range_access_begin(record_cells, NULL, 0, NULL, 0, &access) != DVM_CELL_ACQUIRED)
    {
        return 0;
    }
    int scanned = queued_record_group(value, 1, &group, &access);
    int promoted = scanned && !group.live[0] && defer_record_group(&group, 1);
    DVM_WorldState activity;
    shared_record_snapshots_with_peer += scanned &&
        dvm_world_state(world, &activity) && activity.active > 1;
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    shared_record_scans += scanned;
    if (!promoted && (!scanned || !group.live[0] || !dvm_refcount_drop_nonfinal_u(&D2H(value)->ref)))
    {
        return 0;
    }
    void *head;
    unsigned long bytes;
    if (!dvm_retirement_take(cycle_candidates, &head, &bytes) || head != value ||
        !dvm_runtime_root_remove(D2H(value)))
    {
        abort();
    }
    if (!promoted)
    {
        dvm_runtime_gc_owner_lost();
        released++;
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
    if (!dvm_runtime_rc_candidates_ready() || context != dvm_native_context_current() ||
        !dvm_native_owner_attach(reader, world))
    {
        return 0;
    }
    unsigned int completed = 0;
    if (dvm_native_owner_try_enter_idle_shared(reader))
    {
        DVM_RetirementStats queued = {0};
        (void)dvm_retirement_statistics(cycle_candidates, &queued);
        size_t attempts = queued.ready < budget ? queued.ready : budget;
        for (size_t index = 0; index < attempts; index++)
        {
            if (scan_retiring_record())
            {
                completed++;
            }
            else
            {
                dvm_retirement_rotate(cycle_candidates);
                cycle_candidates_rotated++;
            }
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
    return completed;
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
    if (context != dvm_native_context_current() || !dvm_native_owner_attach(reader, world))
    {
        return 0;
    }
    unsigned int completed = 0;
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
                unsigned long final_epoch = 0;
                DVM_CellResult claim = entry->claimed ? DVM_CELL_ACQUIRED :
                    entry->claim_refused ? DVM_CELL_REFUSED :
                    entry->pinned_children && dvm_world_has_borrowers(world) ? DVM_CELL_BUSY :
                    validate_and_claim_record_group(entry->snapshot,
                        entry->pinned_children ? &final_epoch : NULL);
                if (claim != DVM_CELL_ACQUIRED)
                {
                    entry->claim_refused = claim != DVM_CELL_BUSY;
                    if (claim == DVM_CELL_BUSY && source == record_retries)
                    {
                        dvm_retirement_rotate(source);
                        continue;
                    }
                    DVM_RetirementQueue *destination = claim == DVM_CELL_BUSY ?
                        record_retries : difficult;
                    if (!move_leaf(source, destination, entry))
                    {
                        if (source == record_retries)
                        {
                            continue;
                        }
                        break;
                    }
                    budget--;
                    completed++;
                    continue;
                }
                if (final_epoch)
                {
                    entry->claimed = 1;
                    entry->epoch = final_epoch;
                    budget--;
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
                DVM_WorldState activity;
                shared_record_groups_with_peer += dvm_world_state(world, &activity) && activity.active > 1;
                shared_record_confirmations += entry->pinned_children;
                reclaim_detached_records(entry, 1);
                shared_record_groups++;
                free(entry);
                released++;
                budget--;
                completed++;
                continue;
            }
            if (!leaf_eligible(entry->heap))
            {
                break;
            }
            Type *mapped_channel = NULL;
            int channel = entry->heap->t == &Tchannel;
            if (channel && !claim_quiet_channel(entry->heap, &mapped_channel))
            {
                if (!move_leaf(leaves, difficult, entry))
                {
                    break;
                }
                budget--;
                completed++;
                continue;
            }
            Array *array = entry->heap->t == &Tarray ? H2D(Array *, entry->heap) : NULL;
            int has_children = array && (array->root != H || array->t->np);
            int record = entry->heap->t->free == freeheap;
            int list = entry->heap->t == &Tlist;
            if ((has_children && !transfer_array_children(entry->heap)) ||
                ((record || list) && !transfer_object_fields(entry->heap)))
            {
                if (!move_leaf(leaves, difficult, entry))
                {
                    break;
                }
                budget--;
                completed++;
                continue;
            }
            void *value;
            unsigned long bytes;
            if (!dvm_retirement_take(leaves, &value, &bytes) || value != entry ||
                !dvm_runtime_root_remove(entry->heap))
            {
                abort();
            }
            DVM_RefcountResult result = has_children || channel || record || list ? DVM_REFCOUNT_LAST :
                dvm_refcount_release_u(&entry->heap->ref);
            if (result == DVM_REFCOUNT_LAST)
            {
                Type *type = entry->heap->t;
                DVM_WorldState activity;
                int peer = dvm_world_state(world, &activity) && activity.active > 1;
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
            released++;
            budget--;
            completed++;
        }
        if (!heapmonitor)
        {
            completed += drain_shared_children(budget);
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
    return completed;
}


/* Revalidate under collector admission before mutating any edge. Keep one
 * owner per node through the whole unlink, then use ordinary destruction. */
static int process_queued_record_group(void *value, int promote)
{
    RecordGroup group = {0};
    if (!queued_record_group(value, 1, &group, NULL) || group.live[0])
    {
        return 0;
    }
    if (promote && defer_record_group(&group, 0))
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
        released++;
    }
    while (budget && (dvm_retirement_take(retirement, &value, &bytes) ||
                      dvm_retirement_take(cycle_candidates, &value, &bytes)))
    {
        (void)bytes;
        budget--;
        int promoted = process_queued_record_group(value, 1);
        ptrdel(D2H(value));
        fallback.promoted += promoted;
        if (!promoted)
        {
            fallback_destroy(value);
            released++;
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
            if (dvm_runtime_idle_cycles && accepting && !heapmonitor)
            {
                if (dvm_refcount_load_u(&heap->ref) == 1 &&
                    defer_leaf(heap, poolmsize(heapmem, heap), 1))
                {
                    enqueued++;
                    promoted = 1;
                }
                else
                {
                    promoted = transfer_record_candidate(heap);
                }
            }
            if (!promoted)
            {
                process_queued_record_group(child, 0);
            }
            if (!dvm_runtime_root_batch_remove(packet->roots, index))
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
        (dvm_retirement_peek(leaves) != NULL) + (dvm_retirement_peek(difficult) != NULL) + (dvm_retirement_peek(record_retries) != NULL) + dvm_runtime_rc_candidates_ready();
}


/* [GC:] Read queue occupancy on its active Dis owner. Collectors cannot mutate
 * it until that owner parks/leaves. Every ACTIVE executor of this world can
 * request Fast work; native/idle/foreign/collector callers receive zero. */
unsigned int dvm_runtime_rc_pending(void)
{
    DVM_RetirementStats stats = {0};
    if (admitted())
    {
        DVM_RetirementStats frame_stats = {0};
        (void)dvm_retirement_statistics(frames, &frame_stats);
        (void)dvm_retirement_statistics(retirement, &stats);
        return stats.ready + stats.reserved + frame_stats.ready + frame_stats.reserved +
        (dvm_retirement_peek(leaves) != NULL) + (dvm_retirement_peek(difficult) != NULL) + (dvm_retirement_peek(record_retries) != NULL) + dvm_runtime_rc_candidates_ready();
    }
    return stats.ready+stats.reserved;
}


/* Check only committed native entries. Queue storage stays live through
 * worker join, including during shutdown. No managed object is inspected. */
int dvm_runtime_rc_ready(void)
{
    DVM_RetirementStats refs = {0};
    DVM_RetirementStats packets = {0};
    (void)dvm_retirement_statistics(retirement, &refs);
    (void)dvm_retirement_statistics(frames, &packets);
    return refs.ready != 0 || packets.ready != 0 ||
        dvm_retirement_peek(leaves) != NULL || dvm_retirement_peek(difficult) != NULL ||
        dvm_retirement_peek(record_retries) != NULL || dvm_runtime_rc_candidates_ready();
}


/* [GC:] Caller is the active mutator; the collector cannot access admission
 * fields until this owner leaves. Existing queued references stay rooted. */
void dvm_runtime_rc_close(void)
{
    accepting = 0;
    dvm_retirement_close(retirement);
    dvm_retirement_close(cycle_candidates);
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
        printf("NATIVEFRAMERC drained=%lu pending=%u shared_children=%lu record_groups=%lu shared_record_groups=%lu shared_record_groups_with_peer=%lu shared_record_scans=%lu shared_record_snapshots_with_peer=%lu shared_record_confirmations=%lu cycle_candidates_rotated=%lu cycle_candidates_coalesced=%lu\n",
            frames_drained, frame_stats.ready + frame_stats.reserved, shared_children, record_groups, shared_record_groups, shared_record_groups_with_peer, shared_record_scans, shared_record_snapshots_with_peer, shared_record_confirmations, cycle_candidates_rotated, cycle_candidates_coalesced);
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
    result = (cycle_candidates == NULL || dvm_retirement_destroy(&cycle_candidates)) &&
        (record_retries == NULL || dvm_retirement_destroy(&record_retries)) &&
        (difficult == NULL || dvm_retirement_destroy(&difficult)) &&
        (leaves == NULL || dvm_retirement_destroy(&leaves)) &&
        (retirement == NULL || dvm_retirement_destroy(&retirement)) &&
        (frames == NULL || dvm_retirement_destroy(&frames));
    if (result)
    {
        owner = NULL;
        world = NULL;
        notify_ready = NULL;
    }
    return result;
}
