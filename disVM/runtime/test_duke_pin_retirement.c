/* Copyright (c) 2026 bhgv
 * [GC:] EN: Actual Duke pin retirement and shared snapshot allocation retries.
 * RU: Очистка удержаний Duke и shared-повторы выделения памяти снимка.
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
#include "root_registry.h"
#include "retirement_queue.h"
#include "gc/common/runtime_state.h"
#include "gc/common/graph_snapshot.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "thread_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if CONTROL == 6 || CONTROL == 8 || CONTROL == 9 || CONTROL == 11
#define SCENARIO 2                              /* Initial Duke allocation refusal. */
#elif CONTROL == 7 || CONTROL == 10
#define SCENARIO 3                              /* Growing Duke allocation refusal. */
#elif CONTROL == 5
#define SCENARIO 1                              /* Cursor omission must encounter actual mandatory pressure. */
#elif CONTROL < 2
#define SCENARIO CONTROL                        /* Positive normal and pressure cases. */
#else
#define SCENARIO 0                              /* Other omissions use normal detached-cycle acceptance. */
#endif
#define BranchNodes 64                          /* Detached cycle fits ordinary Cycles. */
#define Leaves 2300                             /* Pointer array exceeds bounded discovery and initial Duke storage. */
#define ArrayIndex (BranchNodes + 1)            /* Source owns this genuine array. */
#define TotalTargets (BranchNodes + Leaves + 2) /* Source, cycle, array and string leaves. */

extern Pool *heapmem;                           /* Real allocator, live through all collector joins. */
static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;  /* Private observation metadata. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Peer and retirement notifications. */
static unsigned admitted;                                 /* Actual admitted peer residents, under sync. */
static unsigned turn;                                     /* Peer allowed to renew its epoch, under sync. */
static int released;                                      /* Parent permits original peer sleep frames to resume. */
static int expired;                                       /* Peer watchdog observed missing completion. */
static Heap *nodes[TotalTargets];                         /* Real allocation identities, under sync. */
static DVM_HeapKey keys[TotalTargets];                    /* Generations prevent reuse confusion. */
static unsigned char destroyed[TotalTargets];             /* Exactly-once physical free observations. */
static unsigned node_count;                               /* Published targets; zero during private setup. */
static unsigned freed;                                    /* Actual poolfree completions. */
static unsigned detached_freed;                           /* Detached cycle frees before outside source release. */
static int outside_held;                                  /* Genuine extra source reference and registry root. */
static int mutated;                                       /* Actual source field detached and old owner retired. */
static int seed_drained;                     /* Cycles consumed the real old-edge seed while Duke's pin stayed. */
static int refused;                          /* Real stale Duke count observation was rejected. */
static int slot_refused;                     /* Mandatory FIFO pressure injected without changing reservation. */
static unsigned slot_passes;                 /* Actual positive Duke retry responses while pressure persists. */
static unsigned commits;                     /* Actual preserved pin commits to mandatory FIFO. */
static DVM_NativeContext *duke_context;      /* Collector identity, atomic publication. */
static DVM_NativeContext *fast_context;      /* Bound Fast identity, atomic publication. */
static DVM_RetirementQueue *mandatory_queue; /* Observed ticket FIFO, under sync. */
static _Thread_local int publishing;         /* Source ticket handoff identifies the real mandatory FIFO. */
static _Thread_local int mutation_pending;   /* Run mutation after the original access ends. */
static _Thread_local DVM_CellDomain *mutation_domain; /* Borrowed until pending mutation finishes. */


int __real_dvm_runtime_guest_sleep(unsigned long);


void __real_Sys_werrstr(void *);


int __real_dvm_runtime_rc_defer(void *);


int __real_dvm_runtime_root_remove(const void *);


int __real_dvm_range_access_end(DVM_RangeAccess *);


int __real_dvm_rc_duke_capture_slice(void *, RecordGroup *, DVM_RangeAccess *);


DVM_RetirementResult __real_dvm_retirement_reserve(DVM_RetirementQueue *, unsigned long, DVM_RetirementReservation *);


int __real_dvm_retirement_commit_ready(DVM_RetirementQueue *, DVM_RetirementReservation *, void *, int *);
unsigned int
__real_dvm_gc_duke_run(DVM_NativeContext *, size_t, DVM_NativeWorld *, DVM_RetirementQueue *, DVM_RetirementQueue *);


void __real_poolfree(Pool *, void *);


int __real_dvm_world_try_quiescent(DVM_NativeWorld *, DVM_NativeContext *);

#include "test_duke_pin_relay.inc"
#include "test_duke_pin_mutation.inc"
#include "test_duke_allocation_retry.inc"


/* [GC:] EN: Observe source ticket transfer on real Fast before Duke starts.
 * value remains strongly owned until production RC accepts it.
 * RU: Наблюдать передачу source ticket на настоящем Fast до начала Duke.
 * value остаётся сильной ссылкой до приёма настоящим RC. */
int __wrap_dvm_runtime_rc_defer(void *value)
{
    pthread_mutex_lock(&sync);
    int selected = node_count && D2H(value) == nodes[0] && !dvm_native_context_process(dvm_native_context_current());
    if (selected)
    {
        __atomic_store_n(&fast_context, dvm_native_context_current(), __ATOMIC_RELEASE);
    }
    pthread_mutex_unlock(&sync);
    publishing = selected;
    int result = __real_dvm_runtime_rc_defer(value);
    publishing = 0;
    return result;
}


/* [GC:] EN: Capture FIFO identity at source publication, then refuse real Duke
 * pin reservations for eight actual retries. queue/token/bytes stay unchanged.
 * The original pin/root and source owner must survive every refused attempt.
 * RU: Зафиксировать FIFO при публикации source, затем отказать резервам Duke
 * на восемь настоящих повторов. queue/token/bytes не меняются. Исходные pin/корень
 * и внешний source-владелец должны пережить каждый отказ. */
DVM_RetirementResult
__wrap_dvm_retirement_reserve(DVM_RetirementQueue *queue, unsigned long bytes, DVM_RetirementReservation *token)
{
    pthread_mutex_lock(&sync);
    if (publishing && !mandatory_queue)
    {
        mandatory_queue = queue;
    }
    int pressure = SCENARIO == 1 && mutated && seed_drained && queue == mandatory_queue &&
                   dvm_native_context_current() == duke_context && slot_passes < 8;
    if (pressure)
    {
        if (CONTROL != 5)
        {
            require(dvm_runtime_root_count(nodes[BranchNodes]) == 1 &&
                        dvm_refcount_load_u(&nodes[BranchNodes]->ref) == 2,
                    "refused Duke pin retains original root and strong owner");
        }
        slot_refused = 1;
    }
    pthread_mutex_unlock(&sync);
    return pressure ? DVM_RETIRE_FULL : __real_dvm_retirement_reserve(queue, bytes, token);
}


/* [GC:] EN: Validate existing pin/root before the real mandatory commit.
 * Do not inspect the managed payload after the commit can transfer ownership.
 * RU: Проверить существующие pin/корень до настоящего commit. После передачи
 * владения commit управляемые данные не читаются. */
int __wrap_dvm_retirement_commit_ready(DVM_RetirementQueue *queue,
                                       DVM_RetirementReservation *token,
                                       void *value,
                                       int *ready)
{
    pthread_mutex_lock(&sync);
    int selected = mutated && seed_drained && queue == mandatory_queue && dvm_native_context_current() == duke_context;
    if (selected)
    {
        require(dvm_runtime_root_count(D2H(value)) >= 1, "Duke pin root disappeared before mandatory commit");
        require(CONTROL != 5 || commits || value == H2D(void *, nodes[BranchNodes]),
                "refused Duke pin advanced journal cursor");
    }
    pthread_mutex_unlock(&sync);
    int result = __real_dvm_retirement_commit_ready(queue, token, value, ready);
    if (selected && result)
    {
        pthread_mutex_lock(&sync);
        commits++;
        pthread_mutex_unlock(&sync);
    }
    return result;
}


/* [GC:] EN: Count actual nonzero Duke responses while injected capacity remains.
 * Forward every context/budget/world/queue/fallback to the production pass.
 * RU: Считать настоящие ненулевые ответы Duke при внедрённом заполнении.
 * context/budget/world/queue/fallback передаются настоящему проходу. */
unsigned int __wrap_dvm_gc_duke_run(DVM_NativeContext *context,
                                    size_t budget,
                                    DVM_NativeWorld *world,
                                    DVM_RetirementQueue *queue,
                                    DVM_RetirementQueue *fallback)
{
    unsigned int work = __real_dvm_gc_duke_run(context, budget, world, queue, fallback);
    pthread_mutex_lock(&sync);
    if (work && allocation_refusals > allocation_passes && allocation_passes < 8)
    {
        allocation_passes++;
    }
    if (work && slot_refused && slot_passes < 8)
    {
        slot_passes++;
    }
    pthread_mutex_unlock(&sync);
    return work;
}


/* [GC:] EN: Observe exact generation frees after the actual allocator returns.
 * Detached records must free while source/array/strings stay independently live.
 * pool/value remain allocated until forwarded poolfree; no test performs a free.
 * RU: Наблюдать освобождения поколений после возврата настоящего аллокатора.
 * Отделённые записи освобождаются при живых source/array/строках. pool/value ещё
 * выделены до настоящего poolfree; сам тест не освобождает объекты. */
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
        require(!destroyed[index] && !expired && dvm_runtime_root_count(value) == 0 &&
                    dvm_world_state(dvm_gc_runtime.world, &state) && !state.stopping && !state.stopped &&
                    state.active >= 2,
                "Duke pin physical release overlaps admitted peers without roots");
        require(!outside_held || (index > 0 && index <= BranchNodes),
                "outside source owner preserves source array and string fields");
        require(index == 0 || index > BranchNodes || outside_held,
                "detached cycle must retire before outside source release");
    }
    __real_poolfree(pool, value);
    if (index < node_count)
    {
        destroyed[index] = 1;
        freed++;
        detached_freed += index > 0 && index <= BranchNodes;
    }
    pthread_mutex_unlock(&sync);
}


/* [GC:] EN: Supported pin cleanup must not request a stopped Fast fallback.
 * Other collectors retain mixed-runtime policy; world/context pass unchanged.
 * RU: Поддержанная очистка pin не запрашивает остановленный Fast. Другие
 * сборщики сохраняют смешанную политику; world/context проходят без изменения. */
int __wrap_dvm_world_try_quiescent(DVM_NativeWorld *world, DVM_NativeContext *context)
{
    pthread_mutex_lock(&sync);
    require(!(mutated && !released && context == fast_context && freed < node_count),
            "Duke cleanup requested stopped Fast admission");
    pthread_mutex_unlock(&sync);
    return __real_dvm_world_try_quiescent(world, context);
}

#include "test_duke_pin_case.inc"
