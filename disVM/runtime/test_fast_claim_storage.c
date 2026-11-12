/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real graph claims with sparse/boundary/dynamic native Module vectors.
 * RU: Настоящие захваты графов с редкими/граничными/динамическими векторами Module.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "kernel.h"
#include "runtab-legacy.h"
#include "pool.h"
#include "heap_pool.h"
#include "runtime_gc.h"
#include "managed_owner.h"
#include "root_registry.h"
#include "gc/common/runtime_state.h"
#include "gc/fast/graph_confirmation.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "thread_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if CONTROL < 5
#define SCENARIO CONTROL                                  /* Actual native owner count profile. */
#elif CONTROL == 5
#define SCENARIO 0                                        /* Node-count allocation omission must reject a pure graph. */
#else
#define SCENARIO 3                                        /* Dynamic allocation omission uses 514 actual Modules. */
#endif
#define NodeCount 1040                                    /* Every graph exceeds bounded Cycles discovery. */
#define MaxModules 520                                    /* Covers both sides of the 513-owner local capacity. */

extern Pool *heapmem;                                     /* Actual allocator through all collector joins. */
static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;  /* Observation metadata only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Peer admission handshakes. */
static unsigned admitted;                                 /* Actual resident peers, under sync. */
static unsigned turn;                                     /* Peer which may renew its epoch, under sync. */
static int released;                                      /* Parent completed all physical-release checks. */
static int expired;                                       /* Peer watchdog observed missing completion. */
static int epochs_held;                                   /* Freeze peer epochs across actual successful graph claim. */
static unsigned held_passes;                              /* Actual Fast responses while final readers remain. */
static Heap *nodes[NodeCount];                            /* Actual managed allocation identities. */
static DVM_HeapKey keys[NodeCount];                       /* Generations reject address reuse. */
static unsigned char destroyed[NodeCount];                /* Exactly-once managed physical frees. */
static Module *native_modules[MaxModules];                /* Actual independent native owners until free. */
static unsigned module_count;                             /* Number of ordinary Modules in this graph. */
static unsigned node_count;                               /* Zero during private construction, then NodeCount. */
static unsigned freed;                                    /* Managed poolfree completions, under sync. */
static unsigned native_freed;                             /* Actual Module free completions, under sync. */
static const RecordGroup *identity;                       /* Same rooted snapshot through every allocation refusal. */
static unsigned refusals;                                 /* Actual vector malloc refusals; sole Fast producer. */
static unsigned allocations;                              /* Actual vector allocation attempts. */
static unsigned vector_frees;                             /* Actual successful vector free completions. */
static int confirmed;                                     /* Real successful graph claim, under sync. */
static _Thread_local int confirming;                      /* Current real confirmation selects this graph. */
static _Thread_local int vector_phase;                    /* Actual reservation closed foreign field readers. */
static _Thread_local int injected;                        /* A native vector allocation failed in this call. */
static _Thread_local void *vector;                        /* Actual native scratch until original free. */


int __real_dvm_runtime_guest_sleep(unsigned long);


void __real_Sys_werrstr(void *);


void *__real_malloc(size_t);


void __real_free(void *);


void __real_poolfree(Pool *, void *);


void __real_dvm_cell_reservation_claim(DVM_CellDomain *, const void *);


DVM_CellResult
__real_dvm_gc_fast_confirm_graph(const RecordGroup *, DVM_CellDomain *, DVM_NativeWorld *, unsigned long *, int *);


unsigned int __real_dvm_runtime_rc_shared_leaves(DVM_NativeContext *, size_t);

#include "test_claim_storage_relay.inc"


/* [GC:] EN: Forward domain/token and observe the actual reservation boundary.
 * No field guards or claim results are substituted.
 * RU: Передать domain/token и наблюдать настоящую границу резервации.
 * Guards полей и ответы захвата не подменяются. */
void __wrap_dvm_cell_reservation_claim(DVM_CellDomain *domain, const void *token)
{
    __real_dvm_cell_reservation_claim(domain, token);
    vector_phase = confirming;
}


/* [GC:] EN: Refuse eight actual vector allocations, requiring exact native
 * cardinality. bytes is forwarded otherwise; no managed buffer is invented.
 * RU: Отказать восьми настоящим выделениям вектора, требуя размера по native-
 * владельцам. Иначе bytes передаётся; управляемые буферы не выдумываются. */
void *__wrap_malloc(size_t bytes)
{
    if (confirming && vector_phase)
    {
        require(module_count > RECORD_NODE_CAPACITY, "claim vector allocation requires native owners");
        require(bytes == module_count * (sizeof(Module *) + sizeof(unsigned int)),
                "native claim vector matches native owner count");
        allocations++;
        if (refusals < 8)
        {
            refusals++;
            injected = 1;
            return NULL;
        }
        vector = __real_malloc(bytes);
        require(vector != NULL, "actual vector allocation recovers");
        return vector;
    }
    return __real_malloc(bytes);
}


/* [GC:] EN: Observe real vector and Module frees without reading freed payload.
 * value remains original allocator ownership until its real free executes.
 * RU: Наблюдать настоящие free вектора/Module без чтения освобождённых данных.
 * value принадлежит исходному аллокатору до выполнения настоящего free. */
void __wrap_free(void *value)
{
    int scratch = confirming && value && value == vector;
    unsigned limit = __atomic_load_n(&module_count, __ATOMIC_ACQUIRE);
    unsigned index = 0;
    while (index < limit && value != __atomic_load_n(&native_modules[index], __ATOMIC_ACQUIRE))
    {
        index++;
    }
    if (value && index < limit)
    {
        pthread_mutex_lock(&sync);
        require(!epochs_held && confirmed, "Module metadata waits for actual final reader epoch");
        DVM_WorldState state;
        require(dvm_world_state(dvm_gc_runtime.world, &state) && state.active >= 2 && !state.stopping && !state.stopped,
                "native Module metadata frees with admitted peers");
        char name[96];
        snprintf(name, sizeof(name), "$FlashVM-native-claim-storage-%u", index);
        require(lookmod(name) == NULL, "each native Module unlinked before metadata free");
        __atomic_store_n(&native_modules[index], NULL, __ATOMIC_RELEASE);
        __real_free(value);
        native_freed++;
        pthread_mutex_unlock(&sync);
    }
    else
    {
        __real_free(value);
    }
    if (scratch)
    {
        vector = NULL;
        vector_frees++;
    }
}


/* [GC:] EN: Run actual selected confirmation; failed allocation must leave
 * counts/roots and the same snapshot intact and report BUSY, never replay.
 * group/cells/world/epoch/reserved remain runtime-owned and pass unchanged.
 * RU: Выполнить настоящее подтверждение. Отказ памяти сохраняет счётчики/корни
 * и тот же снимок, возвращая BUSY вместо повтора пакета. Параметры принадлежат
 * runtime и передаются без изменения. */
DVM_CellResult __wrap_dvm_gc_fast_confirm_graph(
    const RecordGroup *group, DVM_CellDomain *cells, DVM_NativeWorld *world, unsigned long *epoch, int *reserved)
{
    pthread_mutex_lock(&sync);
    int selected = node_count && !destroyed[0] && group->nodes[0] == H2D(void *, nodes[0]);
    if (selected)
    {
        require(group->nodes_used == NodeCount && group->dead_count == NodeCount, "actual full dead graph");
        require(!identity || identity == group, "allocation retry retains the original Fast snapshot");
        identity = group;
        epochs_held = 1;
    }
    pthread_mutex_unlock(&sync);
    confirming = selected;
    vector_phase = 0;
    injected = 0;
    DVM_CellResult result = __real_dvm_gc_fast_confirm_graph(group, cells, world, epoch, reserved);
    require(!vector, "native claim vector must be freed before confirmation returns");
    if (injected)
    {
        require(result == DVM_CELL_BUSY && !*epoch, "native vector allocation must retry original snapshot");
        for (unsigned index = 0; index < group->nodes_used; index++)
        {
            Heap *heap = D2H(group->nodes[index]);
            require(dvm_runtime_root_count(heap) == 1 &&
                        dvm_refcount_load_u(&heap->ref) == group->incoming[index] + (index == 0),
                    "native vector refusal preserves managed references and roots");
        }
        for (unsigned index = 0; index < module_count; index++)
        {
            require(dvm_refcount_load_i(&native_modules[index]->ref) == 1,
                    "native vector refusal preserves all Modules");
        }
    }
    pthread_mutex_lock(&sync);
    if (selected && result == DVM_CELL_ACQUIRED)
    {
        for (unsigned index = 0; index < module_count; index++)
        {
            require(dvm_refcount_load_i(&native_modules[index]->ref) == 0, "all actual native Module owners claimed");
        }
        confirmed = 1;
    }
    pthread_mutex_unlock(&sync);
    confirming = 0;
    vector_phase = 0;
    return result;
}


/* [GC:] EN: Observe actual Fast responses while peer reader epochs stay old.
 * context/budget and the actual work result pass unchanged.
 * RU: Наблюдать настоящие ответы Fast при старых эпохах соседей.
 * context/budget и настоящий результат передаются без изменения. */
unsigned int __wrap_dvm_runtime_rc_shared_leaves(DVM_NativeContext *context, size_t budget)
{
    unsigned int work = __real_dvm_runtime_rc_shared_leaves(context, budget);
    pthread_mutex_lock(&sync);
    if (work && confirmed && epochs_held)
    {
        held_passes++;
    }
    pthread_mutex_unlock(&sync);
    return work;
}


/* [GC:] EN: Observe real generation frees with admitted peers and no roots.
 * pool/value forward to the allocator; no test directly frees managed storage.
 * RU: Наблюдать free настоящих поколений при допущенных соседях без корней.
 * pool/value передаются аллокатору; тест не освобождает managed-память сам. */
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
        require(!destroyed[index] && !epochs_held && confirmed && !expired && !dvm_runtime_root_count(value) &&
                    dvm_world_state(dvm_gc_runtime.world, &state) && state.active >= 2 && !state.stopping &&
                    !state.stopped,
                "actual graph frees after final epoch with admitted peers");
        destroyed[index] = 1;
        freed++;
    }
    __real_poolfree(pool, value);
    pthread_mutex_unlock(&sync);
}

#include "test_fast_claim_case.inc"
