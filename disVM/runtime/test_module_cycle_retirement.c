/* Copyright (c) 2026 bhgv
 * EN: Actual Modlink/MP graph claims, reader epochs and native metadata release.
 * RU: Настоящие захваты графов Modlink/MP, эпохи читателей и снятие метаданных.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "isa.h"
#include "kernel.h"
#include "runtab-legacy.h"
#include "pool.h"
#include "heap_pool.h"
#include "runtime_gc.h"
#include "runtime_rc.h"
#include "module_registry.h"
#include "managed_owner.h"
#include "root_registry.h"
#include "gc/common/runtime_state.h"
#include "gc/fast/graph_confirmation.h"
#include "gc/fast/module_retirement.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "thread_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if CONTROL == 28
#define SCENARIO 20                                       /* Restore sole-owner preflight on a real two-member cycle. */
#elif CONTROL == 29 || CONTROL == 30
#define SCENARIO 27                                       /* Omit live-member reachability or native edge filtering. */
#else
#define SCENARIO CONTROL                                  /* Ordinary positive and existing omission scenarios. */
#endif
#define MaxNodes 2301                                     /* Large graph plus separately retired original MP. */
#define OtherName "$OtherModuleCycleRetirement"           /* Second native registry identity. */
#define ModuleName "$ModuleCycleRetirement"               /* Unique registry identity in this app. */
extern Pool *heapmem;                                     /* Production allocator, alive until runtime joins. */
static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;  /* Fixture metadata only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Peer startup barrier. */
static unsigned admitted;                                 /* Actual native peer admissions, under sync. */
static unsigned turn;                                     /* Only this peer renews its epoch, under sync. */
static int released;                                      /* Parent permits peer return after physical checks. */
static int expired;                                       /* Watchdog observed missing completion, under sync. */
static int epochs_held;                                   /* Freeze old peer epochs around real graph confirmation. */
static unsigned held_passes;                              /* Actual Fast responses during final reader wait. */
static Heap *nodes[MaxNodes];                             /* Published allocation identities, under sync. */
static DVM_HeapKey keys[MaxNodes];                        /* Allocation generations through physical free. */
static unsigned char destroyed[MaxNodes];                 /* Exactly-once physical release bits. */
static unsigned node_count;                               /* Cycle nodes plus independent original MP. */
static unsigned graph_count;                              /* Nodes that belong to the claimed cycle. */
static unsigned freed;                                    /* Real target poolfree returns, under sync. */
static Module *target_module;                             /* Atomic native identity; withdrawn before free. */
static Module *second_module;                             /* Atomic second native identity for partial rollback. */
static Module *racing_lookup;            /* Module owner acquired on Fast and transferred under sync. */
static int lookup_refused;               /* Real outside lookup defeated graph claim. */
static int native_refused;               /* One native CAS refusal injected unchanged. */
static int epoch_refused;                /* One final epoch failure injected after all claims. */
static int rollback_seen;                /* Managed and native counts restored before retry. */
static _Thread_local int confirming;     /* Inside real target confirmation only. */
static Heap *leaf_target;                /* Standalone queued record for cross-lane leaf handoff. */
static int leaf_gate;                    /* Hold Cycles until Fast is demonstrably idle. */
static int leaf_started;                 /* Standalone record has its real candidate owner. */
static _Thread_local int leaf_published; /* Cycles added its actual leaf replacement root. */
static _Thread_local int leaf_notified;  /* The same publication notified Fast. */
static int metadata_freed;               /* Actual metadata free completed, under sync. */
static unsigned finalizers;              /* Ordinary unload or last graph-Module finish callbacks, under sync. */
static unsigned held_freed;              /* Completed target frees when current epoch hold begins. */
static int member_live;                  /* A rooted member may outlive another member of its Module. */
static int member_seen;                  /* Dead subset retired while native/original data stayed live. */
static int live_owner;                   /* External MP ticket is still independently rooted. */
static int live_seen;                    /* Cycles withdrew a live seed while MP owner remained. */
static int changed_fields;               /* Actual count-neutral MP field move completed. */
static int stale_refused;                /* Real confirmation rejected the changed snapshot. */
static int confirmed;                    /* Actual count claim succeeded before final reader wait. */
static DVM_NativeContext *fast_context;  /* Atomic Fast identity through collector join. */


int __real_dvm_runtime_guest_sleep(unsigned long);


void __real_Sys_werrstr(void *);


int __real_dvm_runtime_rc_defer(void *);


int __real_dvm_runtime_root_remove(const void *);


int __real_dvm_runtime_root_add(void *);


int __real_dvm_gc_workers_request(DVM_GCWorkers *, DVM_GCKind);


unsigned int __real_dvm_runtime_rc_shared_candidates(DVM_NativeContext *, size_t);


int __real_dvm_refcount_compare_i(int *, int, int);


unsigned long __real_dvm_world_retire_epoch(DVM_NativeWorld *);


int __real_dvm_module_registry_claim_counts(Module **, const unsigned int *, unsigned, int (*)(void *), void *);


void __real_poolfree(Pool *, void *);


void __real_free(void *);


void __real_dvm_gc_fast_module_finalize(Heap *);


void __real_dvm_gc_fast_module_graph_finalize(Heap *, Module *);


int __real_dvm_gc_fast_module_eligible(const Heap *);


unsigned int __real_dvm_runtime_rc_shared_leaves(DVM_NativeContext *, size_t);
DVM_CellResult
__real_dvm_gc_fast_confirm_graph(const RecordGroup *, DVM_CellDomain *, DVM_NativeWorld *, unsigned long *, int *);

#include "test_module_cycle_relay.inc"
#include "test_module_cycle_faults.inc"
#include "test_module_cycle_leaf.inc"


/* [GC:] EN: Record actual Fast identity at seed ticket handoff. value stays
 * independently owned by the ticket until production RC accepts it.
 * RU: Запомнить настоящий Fast при передаче seed ticket. До приёма RC
 * ссылкой value независимо владеет ticket. */
int __wrap_dvm_runtime_rc_defer(void *value)
{
    pthread_mutex_lock(&sync);
    if (nodes[0] && D2H(value) == nodes[0] && !dvm_native_context_process(dvm_native_context_current()))
    {
        __atomic_store_n(&fast_context, dvm_native_context_current(), __ATOMIC_RELEASE);
    }
    pthread_mutex_unlock(&sync);
    return __real_dvm_runtime_rc_defer(value);
}


/* [GC:] EN: Observe a live seed withdrawal by Cycles while an independent MP
 * ticket still roots the graph. key is metadata; acquire no managed reference.
 * RU: Наблюдать снятие живого seed сборщиком Cycles при внешнем ticket MP.
 * key — метаданные; новая управляемая ссылка не создаётся. */
int __wrap_dvm_runtime_root_remove(const void *key)
{
    int result = __real_dvm_runtime_root_remove(key);
    pthread_mutex_lock(&sync);
    if (result && live_owner && key == nodes[0] && dvm_native_context_current() != fast_context)
    {
        if (SCENARIO == 2 || SCENARIO == 26)
        {
            require(dvm_runtime_root_count(nodes[SCENARIO == 26 ? node_count - 1 : 1]) == 1,
                    "external MP owner retains its root");
        }
        else
        {
            require(dvm_refcount_load_i(&target_module->ref) >= 2, "outside Module lookup owner remains live");
        }
        live_seen = 1;
    }
    pthread_mutex_unlock(&sync);
    return result;
}


/* [GC:] EN: On the real Fast snapshot, optionally move one MP strong field
 * into its other mapped slot under the actual write guards. No count changes:
 * the snapshot pins keep this privately borrowed storage live. Then run real
 * confirmation; freeze old peer epochs before it creates the final cutoff.
 * RU: На настоящем снимке Fast перенести сильное поле MP в другой размеченный
 * слот под настоящими write guards, без изменения счётчиков. Память удерживают
 * pins снимка. Выполнить настоящее подтверждение, заранее заморозив эпохи соседей. */
DVM_CellResult __wrap_dvm_gc_fast_confirm_graph(
    const RecordGroup *group, DVM_CellDomain *cells, DVM_NativeWorld *world, unsigned long *epoch, int *reserved)
{
    pthread_mutex_lock(&sync);
    int selected = 0;
    unsigned int anchor_index = (SCENARIO == 27 || SCENARIO == 33) && destroyed[0] ? (three_members() ? 4 : 2) : 0;
    for (unsigned index = 0; nodes[anchor_index] && !destroyed[anchor_index] && index < group->nodes_used; index++)
    {
        selected |= group->nodes[index] == H2D(void *, nodes[anchor_index]);
    }
    int mutate = selected && SCENARIO == 5 && !changed_fields;
    if (selected)
    {
        require(group->has_modules, "snapshot records native module finalization");
        epochs_held = 1;
        held_passes = 0;
        held_freed = freed;
    }
    pthread_mutex_unlock(&sync);
    if (mutate)
    {
        void **mp = H2D(void **, nodes[1]);
        DVM_RangeAccess access = {0};
        require(dvm_range_access_begin(cells, NULL, 0, mp, 2 * sizeof(void *), &access) == DVM_CELL_ACQUIRED,
                "snapshot MP write guards available");
        require(mp[0] == H2D(void *, nodes[0]) && mp[1] == H, "original MP back edge is intact");
        mp[1] = mp[0];
        mp[0] = H;
        require(dvm_range_access_end(&access), "MP write guards released");
        pthread_mutex_lock(&sync);
        changed_fields = 1;
        pthread_mutex_unlock(&sync);
    }
    if (selected && (SCENARIO == 13 || SCENARIO == 24) && !lookup_refused)
    {
        Module *lookup = lookmod((char *)ModuleName);
        require(lookup != NULL, "actual lookup wins before graph claim");
        pthread_mutex_lock(&sync);
        racing_lookup = lookup;
        live_owner = 1;
        pthread_mutex_unlock(&sync);
    }
    confirming = selected;
    DVM_CellResult result = __real_dvm_gc_fast_confirm_graph(group, cells, world, epoch, reserved);
    confirming = 0;
    observe_claim_rollback(group, selected, result);
    if (selected)
    {
        pthread_mutex_lock(&sync);
        if (mutate)
        {
            require(result == DVM_CELL_REFUSED, "changed MP fields must refuse actual stale snapshot");
            stale_refused = 1;
        }
        confirmed |= result == DVM_CELL_ACQUIRED;
        if (result != DVM_CELL_ACQUIRED)
        {
            epochs_held = 0;
        }
        pthread_mutex_unlock(&sync);
    }
    return result;
}


/* [GC:] EN: Count real Fast retry responses while peer epochs stay old.
 * context/budget and actual work result pass unchanged, without fake progress.
 * RU: Считать настоящие ответы Fast при старых эпохах соседей. context/budget
 * и результат передаются без изменения, без выдуманного прогресса. */
unsigned int __wrap_dvm_runtime_rc_shared_leaves(DVM_NativeContext *context, size_t budget)
{
    unsigned int work = __real_dvm_runtime_rc_shared_leaves(context, budget);
    pthread_mutex_lock(&sync);
    if (work && epochs_held && confirmed)
    {
        held_passes++;
    }
    pthread_mutex_unlock(&sync);
    return work;
}


/* [GC:] EN: Observe native finalization after MP edge consumption. heap is
 * claimed and still allocated. Reject double destruction or missing detachment.
 * RU: Наблюдать native-финализацию после снятия ребра MP. heap захвачен и ещё
 * выделен. Отвергнуть двойное разрушение и отсутствие отсоединения MP. */
static void observe_module_finalizer(Heap *heap)
{
    pthread_mutex_lock(&sync);
    int selected =
        heap == nodes[0] ||
        ((SCENARIO == 3 || SCENARIO == 16 || (SCENARIO >= 20 && SCENARIO <= 27) || three_members() || SCENARIO == 34) &&
         heap == nodes[2]) ||
        (three_members() && heap == nodes[4]);
    if (selected)
    {
        require(H2D(Modlink *, heap)->MP == H, "claimed module MP edge must be detached once");
        require(!epochs_held && !live_owner && dvm_refcount_load_u(&heap->ref) == 0,
                "module finalizer waits for claim and reader epoch");
        finalizers++;
    }
    pthread_mutex_unlock(&sync);
}


/* [GC:] EN: Observe ordinary unload after consumed MP. heap remains claimed.
 * RU: Наблюдать обычный unload после снятия MP. heap остаётся захваченным. */
void __wrap_dvm_gc_fast_module_finalize(Heap *heap)
{
    observe_module_finalizer(heap);
    __real_dvm_gc_fast_module_finalize(heap);
}


/* [GC:] EN: Observe finalization of heap's atomically claimed native module.
 * heap/module remain allocated until the production finalizer consumes them.
 * RU: Наблюдать финализацию атомарно захваченного native-модуля heap.
 * heap/module ещё выделены до передачи настоящему финализатору. */
void __wrap_dvm_gc_fast_module_graph_finalize(Heap *heap, Module *module)
{
    observe_module_finalizer(heap);
    require(dvm_refcount_load_i(&module->ref) == 0 && module->link == NULL,
            "graph owns an unlinked zero-count native Module");
    __real_dvm_gc_fast_module_graph_finalize(heap, module);
}


/* [GC:] EN: Control 6 disables module discovery only on Cycles/Duke, retaining
 * ordinary Fast eligibility. Other heap observations forward unchanged.
 * RU: Контроль 6 отключает обход модулей только на Cycles/Duke, сохраняя
 * допустимость обычного Fast. Остальные проверки heap проходят без изменения. */
int __wrap_dvm_gc_fast_module_eligible(const Heap *heap)
{
    if (SCENARIO == 6 && heap && heap->t == &Tmodlink &&
        dvm_native_context_current() != __atomic_load_n(&fast_context, __ATOMIC_ACQUIRE) &&
        !dvm_native_context_process(dvm_native_context_current()))
    {
        return 0;
    }
    return __real_dvm_gc_fast_module_eligible(heap);
}


/* [GC:] EN: Check exact generation at physical pool free; count completion
 * after allocator return. All graph frees must overlap admitted peers and have
 * no roots, external owner or old epoch. pool/value remain allocated until free.
 * RU: Проверить поколение при poolfree и считать завершение после возврата.
 * Освобождения перекрываются с соседями, без корней, внешнего владельца и старой
 * эпохи. pool/value выделены до настоящего free. */
void __wrap_poolfree(Pool *pool, void *value)
{
    pthread_mutex_lock(&sync);
    unsigned index = 0;
    while (index < node_count && (value != nodes[index] || !dvm_heap_pool_key_matches(pool, value, keys[index])))
    {
        index++;
    }
    if (index < node_count)
    {
        DVM_WorldState state;
        require(!(member_live && index >= (three_members() ? 4U : 2U)),
                "live member and original MP cannot be reclaimed with dead subset");
        require(!destroyed[index] && !live_owner && !epochs_held && !expired &&
                    dvm_world_state(dvm_gc_runtime.world, &state) && !state.stopping && !state.stopped &&
                    state.active >= 2,
                "module cycle physical release waits for readers with admitted peers");
        require(dvm_runtime_root_count(value) == 0, "module cycle has no root at physical free");
        if (((Heap *)value)->t == &Tmodlink)
        {
            require(H2D(Modlink *, value)->m == NULL, "module cycle must run native Module finalization");
        }
    }
    if (index < node_count && value == leaf_target)
    {
        leaf_target = NULL;
    }
    __real_poolfree(pool, value);
    if (index < node_count)
    {
        destroyed[index] = 1;
        freed++;
    }
    pthread_mutex_unlock(&sync);
}


/* [GC:] EN: Observe actual native Module free without locking unrelated native
 * frees. Its identity is withdrawn before allocator return to permit reuse.
 * RU: Наблюдать настоящий free Module без mutex на посторонних native free.
 * Идентичность снимается до возврата аллокатора для корректного переиспользования. */
void __wrap_free(void *value)
{
    Module *identity = __atomic_load_n(&target_module, __ATOMIC_ACQUIRE);
    Module *other = __atomic_load_n(&second_module, __ATOMIC_ACQUIRE);
    unsigned bit = value == identity && identity ? 1 : value == other && other ? 2 : 0;
    if (!bit)
    {
        __real_free(value);
        return;
    }
    pthread_mutex_lock(&sync);
    require(!(metadata_freed & bit) && !epochs_held && !live_owner && !member_live &&
                (SCENARIO == 16 ? finalizers >= 1
                                : finalizers == (SCENARIO == 33   ? 3U
                                                 : SCENARIO == 27 ? 2U
                                                                  : 1U)),
            "native metadata waits for all claimed links");
    DVM_WorldState state;
    require(dvm_world_state(dvm_gc_runtime.world, &state) && !state.stopping && !state.stopped && state.active >= 2,
            "native Module metadata release overlaps admitted peers");
    require(lookmod((char *)(bit == 1 ? ModuleName : OtherName)) == NULL,
            "Module registry unlinks before metadata free");
    if (bit == 1)
    {
        __atomic_store_n(&target_module, NULL, __ATOMIC_RELEASE);
    }
    else
    {
        __atomic_store_n(&second_module, NULL, __ATOMIC_RELEASE);
    }
    pthread_mutex_unlock(&sync);
    __real_free(value);
    pthread_mutex_lock(&sync);
    metadata_freed |= bit;
    pthread_mutex_unlock(&sync);
}

#include "test_module_cycle_case.inc"
