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
#include "gc/fast/file_retirement.h"
#include "gc/fast/table_retirement.h"
#include "gc/fast/module_retirement.h"
#include "bigtable_owner.h"
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

#define RC_CAPACITY 512           /* Initial FIFO slots for release and packet lanes. */
#define RC_CANDIDATE_CAPACITY 256 /* Initial slots; grow during nonfinal-owner bursts. */


/* A packet owns each captured reference through the collector's drain. */
typedef struct FrameRetirement
{
    DVM_RootBatch *roots; /* Published root occurrences for keys. */
    size_t count;         /* Number of transferred owners. */
    size_t next;          /* First owner not yet consumed by Fast. */
    void *keys[];         /* Heap headers in frame-map order. */
} FrameRetirement;


/* Keep the original strong owner and registry root until the old readers end. */
typedef struct LeafRetirement
{
    Heap *heap;               /* Rooted string header until the sole consumer takes it. */
    unsigned long bytes;      /* Direct bytes charged to whichever queue owns it. */
    unsigned long epoch;      /* Last potentially interested reader generation. */
    FrameRetirement *records; /* Optional rooted closed record group. */
    RecordGroup *snapshot;    /* Edges frozen by withdrawal of the last outside owner. */
    int claimed;              /* Counts withdrawn, awaiting the final reader epoch. */
    int pinned_children;      /* Shared discovery retained every child separately. */
    int claim_refused;        /* Fall back to normal release if closure changed. */
} LeafRetirement;

static int duke_publish_pending;              /* Inflight snapshot is complete and waiting for Fast. */
static RecordGroup duke_inflight;             /* Partially captured graph with rooted targets. */
static RecordGroup duke_abandoned;            /* Owned discovery pins awaiting rollback. */
static int duke_cleanup_fallback;             /* Atomic readiness for an exceptional stopped drain. */
static FVM_PortSync *candidate_transfer;      /* Cross-index ownership handoff. */
static DVM_RetirementQueue *duke_candidates;  /* Rooted large graphs for Duke GC. */
static DVM_RetirementQueue *cycle_candidates; /* Nonfinal owners for the Cycles lane. */
static DVM_RetirementQueue *cycle_refused;    /* Candidates finally refused by Cycles or Duke. */
static DVM_RetirementQueue *record_retries;   /* Transient guard conflicts and final epochs. */
static DVM_RetirementQueue *difficult;        /* Fallback work with unchanged roots. */

static struct
{
    unsigned long passes;          /* Stopped fallback invocations. */
    unsigned long promoted;        /* Stopped candidates transferred to shared Fast. */
    unsigned long packet_promoted; /* Stopped packet children handed to shared queues. */
    unsigned long groups;          /* Stopped closed-group releases. */
    unsigned long nonfinal;        /* Nonfinal fallback owners dropped. */
    unsigned long records;         /* Final fallback ordinary records. */
    unsigned long arrays;          /* Final fallback arrays. */
    unsigned long lists;           /* Final fallback lists. */
    unsigned long channels;        /* Final fallback channels. */
    unsigned long strings;         /* Final fallback strings. */
    unsigned long modules;         /* Final fallback module links. */
    unsigned long other;           /* Other final fallback heap types. */
} fallback;

static unsigned long difficult_moved;               /* Entries transferred to the retained fallback queue. */
static DVM_RetirementQueue *leaves;                 /* Plain leaves eligible for shared release. */
static unsigned long shared_strings_with_peer;      /* Final releases with another active owner. */
static unsigned long shared_array_graphs;           /* Shared releases that transfer array children. */
static unsigned long shared_arrays;                 /* Final scalar-array releases on Fast. */
static unsigned long shared_arrays_with_peer;       /* Scalar-array releases with an active guest. */
static unsigned long shared_channels;               /* Final known channel releases on shared Fast. */
static unsigned long shared_plain_records;          /* Final ordinary record releases on shared Fast. */
static unsigned long shared_modules;                /* Final ordinary Modlink releases on shared Fast. */
static unsigned long shared_modules_with_peer;      /* Modlink finalizations overlapping an active guest. */
static unsigned long shared_lists;                  /* Final list releases on shared Fast. */
static unsigned long shared_tables;                 /* Known table heaps finalized on shared Fast. */
static unsigned long shared_keys;                   /* Known key-snapshot heaps finalized on shared Fast. */
static unsigned long shared_table_owners_with_peer; /* Native table/key claims with an active guest. */
static unsigned long shared_files;                  /* Last Sys.FD releases on shared Fast. */
static unsigned long shared_files_with_peer;        /* FD finalization with a live peer resident. */
static unsigned long shared_strings;                /* Final releases by the Fast shared pass. */

static DVM_RetirementQueue *frames;                 /* Bounded queue of frame packets. */
static DVM_CellDomain *record_cells;                /* Borrowed until the collectors join. */
static unsigned long shared_record_scans;           /* Shared candidate scans completed. */
static unsigned long cycle_candidates_rotated;      /* Candidates postponed behind later queue heads. */
static unsigned long cycle_candidates_coalesced;    /* Duplicate generation keys found during publication. */
static unsigned long cycle_candidates_full;         /* Candidate reservations refused for capacity. */
static unsigned long cycle_candidates_rooted;       /* Candidate references dropped while an external root remains. */
static unsigned int duke_peak_fields;               /* Largest successfully published Duke snapshot. */
static unsigned long duke_publish_retries;          /* Complete snapshots retained under pressure. */
static unsigned long duke_adopted_pins;             /* Discovery holds transferred to Fast. */
static unsigned long duke_resumed_slices;           /* Partial scans saved for another admitted turn. */
static unsigned long duke_promoted;                 /* Large snapshots handed to Fast GC. */
static unsigned long cycle_candidates_oversized;    /* Candidate graphs exceeding bounded discovery. */
static unsigned long shared_record_confirmations;   /* Pinned snapshots confirmed for reclamation. */
static size_t fast_reserved_confirmations;          /* Claims confirmed under exact field reservations. */
static size_t fast_ignored_live_fields;             /* Surviving fields skipped during successful claims. */
static unsigned long shared_record_snapshots_with_peer; /* Published snapshots overlapping an active guest. */
static unsigned long shared_record_groups;              /* Groups reclaimed by shared Fast. */
static unsigned long shared_record_groups_with_peer;    /* Group reclamations overlapping an active guest. */
static unsigned long record_groups;                     /* Closed record groups reclaimed from queued owners. */
static unsigned long mandatory_cycle_transfers;         /* Mandatory owners transferred intact to Cycles by Fast. */
static unsigned long mandatory_cycle_retries;           /* Duplicate index observations preserving the pending owner. */
static unsigned long shared_mandatory_owners;           /* Mandatory FIFO owners rescued without a world stop. */
static unsigned long shared_children;                   /* Packet references consumed under shared admission. */
static unsigned long frames_drained;                    /* Stopped collector's packet count. */

extern Pool *heapmem;                                   /* Retained pool, accessible only under world ownership. */
static DVM_RetirementQueue *retirement; /* Native metadata; managed ownership stays under world admission. */
static DVM_NativeContext *owner;        /* Original controller, valid through collector join. */
static DVM_NativeWorld *world;          /* Runtime identity, stable through producer/collector joins. */
static unsigned long enqueued;          /* Ownership transfers, observed after join. */
static unsigned long released;          /* Collector-side retained destroy calls. */
static unsigned long saturated;         /* Bounded-queue inline fallbacks. */
static unsigned long byte_limited;      /* Direct block-byte budget inline fallbacks. */
static unsigned long helper_enqueued;   /* Accepted releases from other admitted executors. */
static unsigned long helper_saturated;  /* Helper releases falling back at the owner-count bound. */


static unsigned long helper_byte_limited; /* Helper releases falling back at the byte bound. */
static void (*notify_ready)(void);        /* Optional Fast wakeup, fixed until collector join. */
static int accepting;                     /* Closed before mutator leaves on shutdown. */


/* [GC:] EN: Authorize payload access for this runtime; allow_shared permits the Fast service as well as ACTIVE
 * guests. Return false without payload access for every other phase, pending progress or foreign world.
 * World remains live through worker joins.
 * RU: Проверить доступ к данным этого мира; allow_shared допускает Fast наряду с ACTIVE-гостями. Иные фазы,
 * незавершённый progress и чужой мир отвергаются до доступа к данным. Мир живёт до join служб. */
static int admitted(int allow_shared)
{
    DVM_HeapOwner *current = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    return current && dvm_native_owner_statistics(current, &state) &&
           (state.phase == DVM_HEAP_ACTIVE || (allow_shared && state.phase == DVM_HEAP_SHARED)) &&
           !state.pending_progress && dvm_native_owner_world(current) == world;
}


/* [GC:] EN: Create RC queues on the bound controller before publishing producers. A prior session must be
 * finished; mandatory queue allocation failure or restart aborts. No managed pointer changes.
 * RU: Создать очереди RC на привязанном контроллере до запуска производителей. Предыдущий сеанс должен
 * завершиться; отказ обязательной очереди или повторный запуск вызывает abort. Управляемые указатели не
 * меняются. */
void dvm_runtime_rc_start(void)
{
    owner = dvm_native_context_current();
    record_cells = dvm_managed_cell_domain();
    world = dvm_native_owner_world(dvm_native_owner_current());
    helper_enqueued = 0;
    helper_saturated = 0;
    helper_byte_limited = 0;
    if (retirement || frames || leaves || difficult || record_retries || cycle_candidates || cycle_refused ||
        duke_candidates)
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
    shared_tables = 0;
    shared_keys = 0;
    shared_table_owners_with_peer = 0;
    shared_files = 0;
    shared_files_with_peer = 0;
    shared_strings = 0;
    shared_channels = 0;
    shared_plain_records = 0;
    shared_lists = 0;
    shared_modules = 0;
    shared_modules_with_peer = 0;
    shared_strings_with_peer = 0;
    frames_drained = 0;
    shared_children = 0;
    shared_mandatory_owners = 0;
    mandatory_cycle_transfers = 0;
    mandatory_cycle_retries = 0;
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


/* [GC:] EN: Borrow notify until worker join; set it at startup before producers publish. NULL disables
 * notification. No callbacks run here.
 * RU: Заимствовать notify до join служб; установить до публикаций производителей. NULL отключает уведомление.
 * Здесь callbacks не вызываются. */
void dvm_runtime_rc_set_notifier(void (*notify)(void))
{
    notify_ready = notify;
}


extern int dvm_runtime_channel_claim_quiet(Heap *heap, Array *buffer, Type **mapped, int already_claimed);
