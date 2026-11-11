/* Copyright (c) 2026 bhgv
 * EN: Real packet cycle owners survive slot/root failures and refused snapshots.
 * RU: Настоящие владельцы циклов в пакетах переживают отказы и отклонённые снимки.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "heap_pool.h"
#include "runtime_gc.h"
#include "managed_owner.h"
#include "root_batch.h"
#include "root_registry.h"
#include "retirement_queue.h"
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

extern Pool *heapmem;                                     /* Production arena, live through runtime join. */
static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;  /* Fixture metadata, never guest fields. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Peer admission notifications. */
static unsigned admitted;                                 /* Peers at the native barrier, under sync. */
static unsigned turn;                                     /* Peer allowed to refresh its admission epoch, under sync. */
static int released;                                      /* Parent finished physical checks, under sync. */
static int expired;                                       /* A peer watchdog observed no parent progress, under sync. */


enum
{
    MaxNodes = 2300
}; /* Exceeds bounded Cycles and initial Duke capture. */

static Heap *nodes[MaxNodes];                /* Target allocation identities, under sync. */
static DVM_HeapKey keys[MaxNodes];           /* Native generations prevent address-reuse confusion. */
static unsigned char destroyed[MaxNodes];    /* Exact target allocation release bits. */
static unsigned node_count;                  /* Published graph size, under sync. */
static unsigned freed_nodes;                 /* Actual target frees completed, under sync. */
static Heap *envelope;                       /* Last-owner container that produces the actual field packet. */
static DVM_HeapKey envelope_key;             /* Container allocation identity through physical free. */
static int envelope_freed;                   /* Real container poolfree returned, under sync. */
static DVM_NativeContext *fast_context;      /* Atomic publication of the actual Fast service identity. */
static DVM_RetirementQueue *candidate_queue; /* Observed real candidate queue, under sync. */
static DVM_RetirementQueue *mandatory_queue; /* Observed real mandatory queue, under sync. */
static DVM_RootBatch *source_batch;          /* Actual packet batch selected at publication, under sync. */
static Heap *source_keys[MaxNodes];          /* Exact occurrence keys of the selected source batch. */
static unsigned source_remaining;            /* Source occurrences not yet withdrawn, under sync. */
static unsigned spills;                      /* Actual raw owners committed to mandatory FIFO, under sync. */
static unsigned direct_refusals;             /* Real candidate reservations refused, under sync. */
static int root_refused;                     /* One replacement-root OOM observed, under sync. */
static int slot_refused;                     /* Mandatory slot refusal observed, under sync. */
static unsigned slot_passes;                 /* Actual Fast responses while slot pressure remains. */
static int duplicate_wait;                   /* Actual pending duplicate index observed, under sync. */
static unsigned duplicate_passes;            /* Real retry responses before opening Cycles. */
static int snapshot_refused;                 /* A genuine published snapshot was refused, under sync. */
static int outside_live; /* Independently rooted native owner keeps the cycle reachable, under sync. */
static int live_seen;    /* Actual Cycles release observed with that outside owner, under sync. */
static int cycles_open;  /* Allow actual Cycles work after intended overlap. */
static _Thread_local int ticket_candidate; /* Outside ticket source-root interval, on Fast only. */
static _Thread_local Heap *key_target;     /* Current Fast candidate-key observation. */
static _Thread_local int declined;         /* Next different queue reserve is the preserving handoff. */
static _Thread_local int staging;          /* Replacement-root attempt following that reservation. */


int __real_dvm_runtime_guest_sleep(unsigned long);


void __real_Sys_werrstr(void *);


int __real_dvm_runtime_rc_defer(void *);


DVM_HeapKey __real_dvm_heap_pool_key(Pool *, void *);


DVM_RetirementResult __real_dvm_retirement_reserve(DVM_RetirementQueue *, unsigned long, DVM_RetirementReservation *);


int __real_dvm_retirement_contains_key(DVM_RetirementQueue *, DVM_HeapKey);


int __real_dvm_retirement_commit_ready(DVM_RetirementQueue *, DVM_RetirementReservation *, void *, int *);


int __real_dvm_retirement_commit_key(DVM_RetirementQueue *, DVM_RetirementReservation *, void *, DVM_HeapKey, int *);


int __real_dvm_runtime_root_add(const void *);


int __real_dvm_runtime_root_remove(const void *);


int __real_dvm_runtime_root_batch_publish(DVM_RootBatch *, void *const *, size_t);


int __real_dvm_runtime_root_batch_remove(DVM_RootBatch *, size_t);


int __real_dvm_runtime_root_batch_clear(DVM_RootBatch *);


unsigned int __real_dvm_runtime_rc_shared_candidates(DVM_NativeContext *, size_t);


unsigned int __real_dvm_runtime_rc_shared_leaves(DVM_NativeContext *, size_t);
DVM_CellResult
__real_dvm_gc_fast_confirm_graph(const RecordGroup *, DVM_CellDomain *, DVM_NativeWorld *, unsigned long *, int *);


void __real_poolfree(Pool *, void *);


int __real_dvm_world_try_quiescent(DVM_NativeWorld *, DVM_NativeContext *);

#include "test_packet_cycle_relay.inc"


/* [GC:] EN: Capture the actual envelope's Fast identity before its field packet
 * exists. For the outside ticket require FIFO staging while its source root
 * remains published; only queue metadata is read after handoff.
 * RU: Зафиксировать настоящий Fast контейнера до появления пакета. value живёт
 * через ticket/RC; внешний ticket остаётся в FIFO до снятия исходного корня.
 * После передачи читаются только метаданные очереди. */
int __wrap_dvm_runtime_rc_defer(void *value)
{
    pthread_mutex_lock(&sync);
    if (envelope && !envelope_freed && D2H(value) == envelope &&
        !dvm_native_context_process(dvm_native_context_current()))
    {
        __atomic_store_n(&fast_context, dvm_native_context_current(), __ATOMIC_RELEASE);
    }
    ticket_candidate = profile() == 7 && D2H(value) == nodes[1];
    pthread_mutex_unlock(&sync);
    int result = __real_dvm_runtime_rc_defer(value);
    ticket_candidate = 0;
    return result;
}


/* [GC:] EN: Reject indexed publication inside the outside ticket's source-root
 * interval before Cycles can consume it. queue/token/value/key/ready pass through
 * unchanged otherwise; no payload is accessed after the actual commit.
 * RU: Запретить публикацию индекса до снятия исходного корня внешнего ticket,
 * прежде чем Cycles сможет его потребить. Иначе передать аргументы без изменения;
 * после настоящего commit данные не читаются. */
int __wrap_dvm_retirement_commit_key(
    DVM_RetirementQueue *queue, DVM_RetirementReservation *token, void *value, DVM_HeapKey key, int *ready)
{
    require(!ticket_candidate, "ticket cycle published before source-root withdrawal");
    return __real_dvm_retirement_commit_key(queue, token, value, key, ready);
}


/* [GC:] EN: Tag a real live candidate generation lookup on Fast. Other owners
 * pass directly, including setup under the fixture mutex. pool/value stay live;
 * this hook grants no pointer ownership.
 * RU: Отметить настоящее чтение поколения на Fast. Другие владельцы проходят
 * напрямую, включая подготовку под mutex. pool/value живут независимо;
 * hook не получает владения указателем. */
DVM_HeapKey __wrap_dvm_heap_pool_key(Pool *pool, void *value)
{
    DVM_HeapKey key = __real_dvm_heap_pool_key(pool, value);
    if (dvm_native_context_current() != __atomic_load_n(&fast_context, __ATOMIC_ACQUIRE))
    {
        return key;
    }
    pthread_mutex_lock(&sync);
    if (freed_nodes < node_count)
    {
        for (unsigned index = 0; index < node_count; index++)
        {
            if (value == nodes[index])
            {
                key_target = value;
                break;
            }
        }
    }
    pthread_mutex_unlock(&sync);
    return key;
}


/* [GC:] EN: Refuse capacity at actual tagged reservations without changing token.
 * Hold mandatory pressure across real retries so startup work cannot conceal a
 * missing retry. All other queue calls remain unchanged.
 * RU: Отказать месту в отмеченной резервации, не меняя token. Удержать давление
 * через настоящие повторы, чтобы начальная работа не скрыла отсутствие retry.
 * Все остальные вызовы очереди остаются настоящими. */
DVM_RetirementResult
__wrap_dvm_retirement_reserve(DVM_RetirementQueue *queue, unsigned long bytes, DVM_RetirementReservation *token)
{
    pthread_mutex_lock(&sync);
    if (freed_nodes < node_count && dvm_native_context_current() == fast_context && key_target)
    {
        candidate_queue = queue;
        key_target = NULL;
        int reject = profile() == 1 ? 0 : profile() == 4 || profile() == 5 ? snapshot_refused && spills < 2 : !spills;
        if (reject)
        {
            direct_refusals++;
            declined = 1;
            pthread_mutex_unlock(&sync);
            return DVM_RETIRE_FULL;
        }
    }
    else if (freed_nodes < node_count && dvm_native_context_current() == fast_context && declined &&
             queue != candidate_queue)
    {
        declined = 0;
        mandatory_queue = queue;
        if (profile() == 3 && slot_passes < 8)
        {
            slot_refused = 1;
            pthread_mutex_unlock(&sync);
            return DVM_RETIRE_FULL;
        }
        staging = 1;
    }
    pthread_mutex_unlock(&sync);
    return __real_dvm_retirement_reserve(queue, bytes, token);
}


/* [GC:] EN: Observe real duplicate index metadata and the next preserving call.
 * queue/key do not grant a managed alias. Return the unmodified lookup result.
 * RU: Наблюдать настоящий дубликат и следующий сохраняющий вызов. queue/key не
 * дают управляемого алиаса. Вернуть исходный результат поиска. */
int __wrap_dvm_retirement_contains_key(DVM_RetirementQueue *queue, DVM_HeapKey key)
{
    int found = __real_dvm_retirement_contains_key(queue, key);
    pthread_mutex_lock(&sync);
    if (found && dvm_native_context_current() == fast_context && queue == candidate_queue)
    {
        key_target = NULL;
        declined = 1;
        duplicate_wait = 1;
    }
    pthread_mutex_unlock(&sync);
    return found;
}


/* [GC:] EN: Refuse one real replacement root; the existing packet still owns
 * heap and its root. No registry mutation or reference decrement occurs here.
 * RU: Отказать одному настоящему замещающему корню. Пакет всё ещё владеет heap
 * и корнем. Здесь нет изменения реестра или снятия ссылки. */
int __wrap_dvm_runtime_root_add(const void *heap)
{
    if (staging)
    {
        staging = 0;
        pthread_mutex_lock(&sync);
        if ((profile() == 2 || profile() == 5) && !root_refused)
        {
            require(dvm_runtime_root_count(heap) >= 1 && dvm_refcount_load_u(&((Heap *)heap)->ref) > 1,
                    "replacement-root refusal must preserve source root and owner");
            root_refused = 1;
            pthread_mutex_unlock(&sync);
            return 0;
        }
        pthread_mutex_unlock(&sync);
    }
    return __real_dvm_runtime_root_add(heap);
}


/* [GC:] EN: Observe real candidate-root withdrawal while an independently
 * rooted native owner keeps the immutable cycle reachable. key is metadata;
 * no extra managed reference is acquired. Require that outside root still exists.
 * RU: Наблюдать настоящее снятие корня кандидата, пока независимый native-владелец
 * сохраняет достижимость неизменного цикла. key — метаданные; ссылка не добавляется.
 * Требовать, чтобы внешний корень всё ещё существовал.
 */
int __wrap_dvm_runtime_root_remove(const void *key)
{
    int result = __real_dvm_runtime_root_remove(key);
    pthread_mutex_lock(&sync);
    if (result && profile() == 7 && outside_live && key == nodes[0] && dvm_native_context_current() != fast_context)
    {
        require(dvm_runtime_root_count(nodes[1]) == 1, "independent live cycle owner retains its root");
        live_seen = 1;
    }
    pthread_mutex_unlock(&sync);
    return result;
}


/* [GC:] EN: Count real mandatory raw-owner commits, not packet metadata. Input
 * value remains caller-owned until the original commit consumes its reservation.
 * RU: Учесть настоящие commit обязательных ссылок, не метаданные пакетов. value
 * принадлежит вызывающему до потребления резервации настоящим commit. */
int __wrap_dvm_retirement_commit_ready(DVM_RetirementQueue *queue,
                                       DVM_RetirementReservation *token,
                                       void *value,
                                       int *ready)
{
    int result = __real_dvm_retirement_commit_ready(queue, token, value, ready);
    pthread_mutex_lock(&sync);
    if (result && queue == mandatory_queue && dvm_native_context_current() == fast_context)
    {
        spills++;
    }
    pthread_mutex_unlock(&sync);
    return result;
}


/* [GC:] EN: Observe actual envelope/snapshot root batches; copy only native key
 * metadata before publication. Source lives until its consuming owner finishes.
 * RU: Наблюдать настоящие пакеты контейнера/снимка; скопировать только ключи до
 * публикации. Источник живёт до завершения потребляющего владельца. */
int __wrap_dvm_runtime_root_batch_publish(DVM_RootBatch *batch, void *const *values, size_t count)
{
    pthread_mutex_lock(&sync);
    if (node_count && count && values[0] == nodes[0] &&
        ((dvm_native_context_current() == fast_context && !envelope_freed) ||
         ((profile() == 4 || profile() == 5) && !snapshot_refused)))
    {
        require(count <= MaxNodes, "actual source root vector fits diagnostic storage");
        source_batch = batch;
        source_remaining = (unsigned)count;
        for (size_t index = 0; index < count; index++)
        {
            source_keys[index] = values[index];
        }
    }
    pthread_mutex_unlock(&sync);
    return __real_dvm_runtime_root_batch_publish(batch, values, count);
}


/* [GC:] EN: Require a replacement before selected packet root withdrawal.
 * batch/index stay live; this observer neither retains nor removes extra owners.
 * RU: Требовать замену перед снятием корня пакета. batch/index живут весь вызов;
 * наблюдатель не создаёт и не снимает дополнительных владельцев. */
int __wrap_dvm_runtime_root_batch_remove(DVM_RootBatch *batch, size_t index)
{
    pthread_mutex_lock(&sync);
    int selected = batch == source_batch && source_remaining && (profile() < 4 || snapshot_refused || profile() == 6);
    if (selected)
    {
        require(dvm_runtime_root_count(source_keys[index]) >= 2, "packet cycle owner lost its replacement root");
    }
    int result = __real_dvm_runtime_root_batch_remove(batch, index);
    if (selected && result)
    {
        source_remaining--;
    }
    pthread_mutex_unlock(&sync);
    return result;
}


/* [GC:] EN: Reject cursor completion while a selected source occurrence remains.
 * batch stays private/live; refusal does not mutate its roots.
 * RU: Запретить завершение курсора при оставшемся вхождении источника. batch
 * остаётся частным и живым; отказ не меняет корни. */
int __wrap_dvm_runtime_root_batch_clear(DVM_RootBatch *batch)
{
    pthread_mutex_lock(&sync);
    require(!(batch == source_batch && source_remaining && (profile() < 4 || snapshot_refused || profile() == 6)),
            "packet cursor advanced past an untransferred owner");
    pthread_mutex_unlock(&sync);
    return __real_dvm_runtime_root_batch_clear(batch);
}


/* [GC:] EN: Refuse one real rooted snapshot without changing counts or fields.
 * Its genuine discovery pins must then pass through the packet owner path.
 * RU: Отклонить настоящий снимок без изменения счётчиков/полей. Его настоящие
 * удержания затем должны пройти через путь владельцев пакета. */
DVM_CellResult __wrap_dvm_gc_fast_confirm_graph(
    const RecordGroup *group, DVM_CellDomain *cells, DVM_NativeWorld *world, unsigned long *epoch, int *reserved)
{
    pthread_mutex_lock(&sync);
    if ((profile() == 4 || profile() == 5) && !snapshot_refused && group->nodes_used == 2 &&
        D2H(group->nodes[0]) == nodes[0])
    {
        snapshot_refused = 1;
        *reserved = 0;
        pthread_mutex_unlock(&sync);
        return DVM_CELL_REFUSED;
    }
    pthread_mutex_unlock(&sync);
    return __real_dvm_gc_fast_confirm_graph(group, cells, world, epoch, reserved);
}


/* [GC:] EN: Count generation-matched physical frees beside admitted peers.
 * Envelope must publish fields first; all retiring allocations have no roots.
 * RU: Учесть физические освобождения по поколению при допущенных соседях.
 * Контейнер сначала публикует поля; у освобождаемых аллокаций нет корней. */
void __wrap_poolfree(Pool *pool, void *value)
{
    pthread_mutex_lock(&sync);
    DVM_HeapKey key = __real_dvm_heap_pool_key(pool, value);
    int container = envelope && value == envelope && key.generation == envelope_key.generation;
    unsigned index = 0;
    while (index < node_count && (value != nodes[index] || key.generation != keys[index].generation))
    {
        index++;
    }
    if (container || index < node_count)
    {
        DVM_WorldState state;
        require(!expired && dvm_world_state(dvm_gc_runtime.world, &state) && !state.stopping && !state.stopped &&
                    state.active >= 2,
                "packet graph destruction must overlap admitted peers");
        require(index == node_count || !outside_live, "reachable packet graph was reclaimed");
        require(dvm_runtime_root_count(value) == 0, "target allocation still has a registry root");
        require(container ? !envelope_freed : envelope_freed && !destroyed[index],
                "target allocation freed exactly once after envelope");
    }
    __real_poolfree(pool, value);
    if (container)
    {
        envelope_freed = 1;
    }
    if (index < node_count)
    {
        destroyed[index] = 1;
        freed_nodes++;
    }
    pthread_mutex_unlock(&sync);
}

#include "test_packet_cycle_case.inc"
