/* A bounded last-owner queue; all heap work remains under native_world. */
#include "disvm/refcount.h"
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "disvm/native_context.h"
#include "disvm/native_owner.h"
#include "runtime_rc.h"
#include "retirement_queue.h"
#include "root_batch.h"
#include "root_registry.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define RC_BYTES (4UL*1024UL*1024UL) /* Bound queued direct block bytes, excluding descendants. */
#define RC_CAPACITY 512 /* Fixed native slots; saturation falls back to inline RC. */

/* A packet owns each captured reference through the collector's drain. */
typedef struct FrameRetirement
{
    DVM_RootBatch *roots; /* Published root occurrences for keys. */
    size_t count;         /* Number of transferred owners. */
    void *keys[];        /* Heap headers in frame-map order. */
} FrameRetirement;

/* Keep the original strong owner and registry root until the old readers end. */
typedef struct StringRetirement
{
    Heap *heap;           /* Rooted string header until the sole consumer takes it. */
    unsigned long epoch;  /* Last potentially interested reader generation. */
} StringRetirement;

static DVM_RetirementQueue *strings; /* Plain strings eligible for shared release. */
static unsigned long shared_strings_with_peer; /* Final releases with another active owner. */
static unsigned long shared_strings; /* Final releases by the Fast shared pass. */

static DVM_RetirementQueue *frames; /* Bounded queue of frame packets. */
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
    world = dvm_native_owner_world(dvm_native_owner_current());
    helper_enqueued = 0;
    helper_saturated = 0;
    helper_byte_limited = 0;
    if (retirement || frames || strings)
    {
        fprintf(stderr, "disvm RC queue restarted before successful finish\n");
        abort();
    }
    retirement = dvm_retirement_create(RC_CAPACITY, RC_BYTES);
    frames = dvm_retirement_create(RC_CAPACITY, RC_BYTES);
    strings = dvm_retirement_create(RC_CAPACITY, RC_BYTES);
    shared_strings = 0;
    shared_strings_with_peer = 0;
    frames_drained = 0;
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


/* Try the shared-string queue while the destroying guest owns exclusion.
 * All failure paths keep the existing owner available to ordinary destroy. */
static int defer_string(Heap *heap, unsigned long bytes)
{
    if (!strings || heapmonitor || heap->t != &Tstring ||
        H2D(String *, heap)->tmp || bytes > RC_BYTES)
    {
        return 0;
    }
    DVM_RetirementReservation token = {0};
    if (dvm_retirement_reserve(strings, bytes, &token) != DVM_RETIRE_OK)
    {
        return 0;
    }
    StringRetirement *entry = malloc(sizeof(*entry));
    if (!entry)
    {
        dvm_retirement_cancel(strings, &token);
        return 0;
    }
    entry->heap = heap;
    entry->epoch = dvm_world_retire_epoch(world);
    if (!entry->epoch || !dvm_runtime_root_add(heap))
    {
        free(entry);
        dvm_retirement_cancel(strings, &token);
        return 0;
    }
    Setmark(heap);
    int became_ready;
    if (!dvm_retirement_commit_ready(strings, &token, entry, &became_ready))
    {
        abort();
    }
    if (became_ready && notify_ready)
    {
        notify_ready();
    }
    return 1;
}


/* [GC:] value has passed destroy's retained block check. Keep its last reference
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
    if (dvm_refcount_load_u(&heap->ref) != 1)
    {
        return 0;
    }
    bytes = poolmsize(heapmem,heap);
    if (defer_string(heap, bytes))
    {
        enqueued++;
        helper_enqueued += helper != 0;
        return 1;
    }
    reserved = dvm_retirement_reserve(retirement, bytes, &token);
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
        (void)dvm_retirement_cancel(retirement, &token);
        memcpy(up->env->errstr,previous,sizeof(previous));
        return 0;
    }
    ptradd(heap);
    poperror();
    Setmark(heap);
    int became_ready;
    if (!dvm_retirement_commit_ready(retirement, &token, value, &became_ready))
    {
        (void)dvm_retirement_cancel(retirement, &token);
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
        !dvm_runtime_root_batch_publish(packet->roots, packet->keys, packet->count))
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


/* Try a bounded shared pass on the Fast thread. It never waits for admission
 * and touches only the leaf queue. Cycles cannot run during this callback. */
unsigned int dvm_runtime_rc_shared_strings(DVM_NativeContext *context, size_t budget)
{
    if (!dvm_retirement_peek(strings))
    {
        return 0;
    }
    DVM_WorldParticipant *reader = dvm_world_join(world, context);
    if (!reader)
    {
        return 0;
    }
    unsigned int completed = 0;
    if (dvm_world_try_enter_shared(reader))
    {
        while (budget && !heapmonitor)
        {
            StringRetirement *entry = dvm_retirement_peek(strings);
            if (!entry || !dvm_world_epoch_elapsed(world, entry->epoch) ||
                H2D(String *, entry->heap)->tmp)
            {
                break;
            }
            void *value;
            unsigned long bytes;
            if (!dvm_retirement_take(strings, &value, &bytes) || value != entry ||
                !dvm_runtime_root_remove(entry->heap))
            {
                abort();
            }
            DVM_RefcountResult result = dvm_refcount_release_u(&entry->heap->ref);
            if (result == DVM_REFCOUNT_LAST)
            {
                if (dvm_refcount_release_i(&Tstring.ref) != DVM_REFCOUNT_LIVE)
                {
                    abort();
                }
                DVM_WorldState activity;
                if (dvm_world_state(world, &activity) && activity.active > 1)
                {
                    shared_strings_with_peer++;
                }
                poolfree(heapmem, entry->heap);
                shared_strings++;
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
        if (!dvm_world_leave(reader))
        {
            abort();
        }
    }

    if (!dvm_world_detach(reader))
    {
        abort();
    }
    return completed;
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
    while (budget && dvm_retirement_take(retirement, &value, &bytes))
    {
        (void)bytes;
        budget--;
        ptrdel(D2H(value));
        destroy(value);
        released++;
    }
    while (budget && dvm_retirement_take(strings, &value, &bytes))
    {
        StringRetirement *entry = value;
        budget--;
        ptrdel(entry->heap);
        destroy(H2D(void *, entry->heap));
        free(entry);
        released++;
    }
    while (budget && dvm_retirement_take(frames, &value, &bytes))
    {
        FrameRetirement *packet = value;
        budget--;
        for (size_t index = 0; index < packet->count; index++)
        {
            if (!dvm_runtime_root_batch_remove(packet->roots, index))
            {
                abort();
            }
            destroy(H2D(void *, packet->keys[index]));
        }
        if (!dvm_runtime_root_batch_clear(packet->roots) ||
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
        (dvm_retirement_peek(strings) != NULL);
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
        (dvm_retirement_peek(strings) != NULL);
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
    return refs.ready != 0 || packets.ready != 0 || dvm_retirement_peek(strings) != NULL;
}


/* [GC:] Caller is the active mutator; the collector cannot access admission
 * fields until this owner leaves. Existing queued references stay rooted. */
void dvm_runtime_rc_close(void)
{
    accepting = 0;
    dvm_retirement_close(retirement);
    dvm_retirement_close(frames);
    dvm_retirement_close(strings);
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
        printf("NATIVEFRAMERC drained=%lu pending=%u\n",
            frames_drained, frame_stats.ready + frame_stats.reserved);
    }
    if (diagnostics)
    {
        printf("NATIVESTRINGRC shared=%lu with_peer=%lu\n", shared_strings, shared_strings_with_peer);
    }
    result = (strings == NULL || dvm_retirement_destroy(&strings)) &&
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
