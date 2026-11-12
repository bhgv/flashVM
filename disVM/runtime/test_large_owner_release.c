/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real wide object/list/array retirement beside admitted Limbo peers.
 * RU: Настоящее освобождение широких объектов/списков/массивов при активных соседях Limbo.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "heap_pool.h"
#include "runtime_gc.h"
#include "runtime_rc.h"
#include "managed_owner.h"
#include "root_batch.h"
#include "root_registry.h"
#include "retirement_queue.h"
#include "gc/common/runtime_state.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "thread_port.h"
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if CONTROL < 100
#define Case CONTROL        /* Positive scenario selected by the driver. */
#elif CONTROL == 100
#define Case 4              /* Real scenario for private semantic omission 100. */
#elif CONTROL == 101
#define Case 7              /* Real scenario for private semantic omission 101. */
#elif CONTROL == 102
#define Case 8              /* Real scenario for private semantic omission 102. */
#elif CONTROL == 103
#define Case 9              /* Real scenario for private semantic omission 103. */
#elif CONTROL == 104
#define Case 12             /* Real scenario for private semantic omission 104. */
#elif CONTROL == 105
#define Case 13             /* Real scenario for private semantic omission 105. */
#elif CONTROL == 106
#define Case 10             /* Real scenario for private semantic omission 106. */
#elif CONTROL == 107
#define Case 18             /* Real scenario for private semantic omission 107. */
#elif CONTROL == 108
#define Case 19             /* Real scenario for private semantic omission 108. */
#elif CONTROL == 109
#define Case 18             /* Real scenario for private semantic omission 109. */
#elif CONTROL == 110
#define Case 16             /* Real scenario for private semantic omission 110. */
#elif CONTROL >= 111 && CONTROL <= 120
#define Case (CONTROL - 90) /* Real childless claim scenario for each omission. */
#else
#error Unknown large owner omission control
#endif

#define IsList                                                                                                         \
    (Case == 5 || Case == 17 || Case == 20 || Case == 24 || Case == 25 || Case == 30) /* Actual list tail/data shape. */
#define IsArray                                                                                                        \
    ((Case >= 12 && Case <= 15) || Case == 19 || Case == 22 || Case == 23 || Case == 28 ||                             \
     Case == 29)                                                   /* Actual Tarray. */
#define Slots (Case == 2 || IsList ? 512 : Case == 3 ? 513 : 2300) /* Mapped data slots, including nil. */
#define EmptyRace (Case >= 21 && Case <= 26)       /* Actual temporary retain at a childless atomic claim. */
#define EligibilityRace (Case >= 27 && Case <= 30) /* Actual temporary retain at the preliminary count read. */
#define NoChildren (Case == 0 || EmptyRace || EligibilityRace)   /* Nil maps or no mapped storage/descriptor. */
#define Owners (NoChildren ? 0 : Case == 1 ? 1 : Slots + IsList) /* Strong occurrences, including aliases. */
#define Children (Case == 6 ? 1 : Owners)                        /* Distinct allocations that must physically free. */
#define Assist (Case == 16)             /* Unknown retained callback releases persistent packet malloc pressure. */
#define Race (Case >= 18 && Case <= 20) /* Real retain between child-root publication and last claim. */


/* [GC:] EN: Native fault identity; no managed ownership is simulated.
 * RU: Идентичность нативного отказа; управляемое владение не имитируется. */
enum FaultKind
{
    NoFault,      /* Unmodified real allocation/reservation. */
    VectorMemory, /* Temporary owner-key vector malloc. */
    PacketMemory, /* Actual child packet malloc. */
    RootMemory,   /* Actual child-root batch allocation. */
    PacketSlots,  /* Actual packet queue reports FULL. */
    PacketBytes   /* Actual packet queue reports BYTES. */
};

#if Case == 7
#define Fault VectorMemory /* Retry exact owner-key scratch allocation. */
#elif Case == 8 || Case == 12 || Assist
#define Fault PacketMemory /* Retry actual child-packet allocation. */
#elif Case == 9 || Case == 13 || Case == 17
#define Fault RootMemory   /* Retry child-root metadata allocation. */
#elif Case == 10 || Case == 14
#define Fault PacketSlots  /* Retry actual slot pressure. */
#elif Case == 11 || Case == 15
#define Fault PacketBytes  /* Retry actual byte pressure. */
#else
#define Fault NoFault      /* Baseline cardinality or a real competing retain. */
#endif

extern Pool *heapmem;                                     /* Actual managed arena through worker joins. */
static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;  /* Fixture metadata only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Real peer readiness. */
static unsigned admitted;                                 /* Number of permanently bound peers, under sync. */
static unsigned turn;                                     /* Peer allowed to advance its epoch, under sync. */
static int released;                                      /* Permit peers to leave shared admission, under sync. */
static int expired;                                       /* Peer watchdog failed, under sync. */
static int epochs_held;                       /* Preserve old readers until eight actual attempts, under sync. */
static unsigned held_passes;                  /* Real Fast callbacks with the old parent epoch, under sync. */
static Heap *target;                          /* Rooted container identity, immutable after publication. */
static DVM_HeapKey target_key;                /* Detect address reuse at actual poolfree. */
static unsigned long target_epoch;            /* Real leaf cutoff, under sync. */
static Heap *children[Children + 1];          /* Actual unique child identities. */
static DVM_HeapKey child_keys[Children + 1];  /* Generation of each child. */
static unsigned occurrences[Children + 1];    /* Strong references owned by mapped fields. */
static unsigned char destroyed[Children + 1]; /* Exactly-once child frees, under sync. */
static unsigned freed;                        /* Unique child frees, under sync. */
static int target_freed;                      /* Parent free completed, under sync. */
static Heap *unknown;                         /* Real unsupported final owner beside shared work. */
static DVM_HeapKey unknown_key;               /* Generation of the unsupported owner. */
static unsigned unknown_calls;                /* Actual retained callback count, under sync. */
static unsigned unknown_freed;                /* Actual unsupported object frees, under sync. */
static unsigned refusals;                     /* Actual injected allocation/reservation refusals, under sync. */
static unsigned failure_passes;               /* Real Fast responses during injected pressure, under sync. */
static unsigned races;                        /* Actual last-claim losses after a competing retain, under sync. */
static void *vector;                          /* Exact temporary key vector, under sync. */
static unsigned vectors;                      /* Successful key scratch allocations, under sync. */
static unsigned vector_frees;                 /* Real matching free calls, under sync. */
static DVM_RootBatch *race_batch;             /* Published child roots awaiting rollback on Fast. */
static int extra_owner;                       /* Actual independent parent owner during the claim race. */
static _Thread_local int fast_call;           /* Current callback is shared Fast. */
static _Thread_local int publishing_target;   /* This producer owns target leaf metadata before commit. */
static _Thread_local int selected;            /* Current actual leaf epoch identifies target. */
static _Thread_local int epoch_seen;          /* Parent cutoff was tested in this callback. */
static _Thread_local int failure_seen;        /* This callback encountered an actual injected failure. */
static _Thread_local void *scratch;           /* This thread's selected temporary key vector. */


int __real_dvm_runtime_guest_sleep(unsigned long);


void __real_Sys_werrstr(void *);


void *__real_malloc(size_t);


void __real_free(void *);


void __real_poolfree(Pool *, void *);


int __real_dvm_retirement_commit_ready(DVM_RetirementQueue *, DVM_RetirementReservation *, void *, int *);


int __real_dvm_world_epoch_elapsed(DVM_NativeWorld *, unsigned long);


DVM_RetirementResult __real_dvm_retirement_reserve(DVM_RetirementQueue *, unsigned long, DVM_RetirementReservation *);


DVM_RootBatch *__real_dvm_runtime_root_batch_create(size_t);


int __real_dvm_runtime_root_batch_publish(DVM_RootBatch *, void *const *, size_t);


int __real_dvm_runtime_root_batch_clear(DVM_RootBatch *);


int __real_dvm_runtime_root_batch_destroy(DVM_RootBatch **);


unsigned int __real_dvm_runtime_rc_shared_leaves(DVM_NativeContext *, size_t);


int __real_dvm_runtime_rc_defer(void *);

#include "test_large_owner_relay.inc"
#include "test_large_owner_faults.inc"
#include "test_empty_leaf_claim.inc"
#include "test_large_owner_observers.inc"
#include "test_large_owner_case.inc"
