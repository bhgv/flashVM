/* Copyright (c) 2026 bhgv
 * EN: Actual cyclic records survive mandatory/index retries and die beside admitted peers.
 * RU: Настоящие циклические записи переживают повторы очереди и освобождаются при активных соседях.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "heap_pool.h"
#include "runtime_gc.h"
#include "managed_owner.h"
#include "root_registry.h"
#include "retirement_queue.h"
#include "gc/common/runtime_state.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "thread_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern Pool *heapmem;                                     /* Production arena, alive through the joined runtime. */

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;  /* Relay state, never managed fields. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Peer admission notification. */
static unsigned admitted;                                 /* Shared peers at the barrier, under sync. */
static unsigned turn;                                     /* Only this peer can advance its epoch, under sync. */
static int released; /* Parent allows original sleep frames to resume, under sync. */
static int expired;  /* Peer watchdog observed missing parent progress, under sync. */


enum
{
    MaxNodes = 2300
}; /* Exceeds the initial Duke capture and bounded Cycles capacities. */

static unsigned node_count;                  /* Published target count, under sync. */
static unsigned char deallocated[MaxNodes];  /* Exact physical-release identity, under sync. */
static unsigned duplicate_passes;            /* Actual Fast responses before opening the duplicate barrier. */
static DVM_HeapKey generations[MaxNodes];    /* Distinguish target allocation from a later address reuse. */
static void *nodes[MaxNodes];                /* Heap identities only after physical free, under sync. */
static unsigned freed;                       /* Exact physically released record count, under sync. */
static unsigned handoffs;                    /* Real Fast RC publications completed, under sync. */
static unsigned rescued;                     /* Real root-preserving Fast-to-Cycles commits, under sync. */
static int duplicate_wait;                   /* Fast observed the still-indexed prior candidate, under sync. */
static int injected;                         /* One candidate reservation refusal, under sync. */
static int cycles_open;                      /* Permit actual Cycles consumption, under sync. */
static DVM_NativeContext *fast_context;      /* Service identity borrowed through runtime join. */
static DVM_RetirementQueue *candidate_queue; /* Candidate identity captured at reserve, under sync. */
static DVM_RetirementQueue *mandatory_queue; /* Ticket staging FIFO, under sync. */
static _Thread_local int publishing;         /* Only this actual Fast RC publication may inject refusal. */


int __real_dvm_runtime_guest_sleep(unsigned long);


void __real_Sys_werrstr(void *);


int __real_dvm_runtime_rc_defer(void *);


DVM_RetirementResult __real_dvm_retirement_reserve(DVM_RetirementQueue *, unsigned long, DVM_RetirementReservation *);


int __real_dvm_retirement_contains_key(DVM_RetirementQueue *, DVM_HeapKey);


int __real_dvm_retirement_commit_key(DVM_RetirementQueue *, DVM_RetirementReservation *, void *, DVM_HeapKey, int *);


unsigned int __real_dvm_runtime_rc_shared_candidates(DVM_NativeContext *, size_t);


unsigned int __real_dvm_runtime_rc_shared_leaves(DVM_NativeContext *, size_t);


void __real_poolfree(Pool *, void *);


int __real_dvm_world_try_quiescent(DVM_NativeWorld *, DVM_NativeContext *);


/* EN: Fail the exact ownership predicate; message is borrowed constant text.
 * RU: Завершить проверку конкретного владения; message — заимствованный текст. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL cycle owner rescue: %s\n", message);
        exit(12);
    }
}


/* EN: Return monotonic milliseconds for watchdogs, never evidence of success.
 * RU: Вернуть монотонные миллисекунды для watchdog, не признака успеха. */
static unsigned long long milliseconds(void)
{
    struct timespec now;
    require(!clock_gettime(CLOCK_MONOTONIC, &now), "monotonic clock available");
    return (unsigned long long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}


/* EN: Hold real sleep(0) with published shared roots. Two peers alternate
 * epochs while the other stays admitted, preventing any quiescent reclamation.
 * Forward duration unchanged when parent releases the original native frames.
 * RU: Удержать настоящий sleep(0) с shared-корнями. Два соседа чередуют эпохи,
 * пока другой остаётся допущенным, исключая освобождение при общем покое.
 * После разрешения родителя передать duration исходным нативным кадрам. */
int __wrap_dvm_runtime_guest_sleep(unsigned long duration)
{
    require(!duration && dvm_runtime_gc_dispatch_share(), "real peer shared admission");
    DVM_HeapOwner *owner = dvm_native_owner_current();
    unsigned long long deadline = milliseconds() + 15000;
    pthread_mutex_lock(&sync);
    unsigned id = admitted++;
    require(id < 2, "exactly two peer residents");
    pthread_cond_broadcast(&changed);
    while (!released && milliseconds() < deadline)
    {
        if (admitted == 2 && turn == id)
        {
            require(dvm_native_owner_leave_shared(owner) && dvm_native_owner_enter_shared(owner),
                    "advance one peer epoch");
            turn = 1 - id;
        }
        pthread_mutex_unlock(&sync);
        fvm_port_thread_sleep_us(1000);
        pthread_mutex_lock(&sync);
    }
    expired |= !released;
    pthread_mutex_unlock(&sync);
    dvm_runtime_gc_dispatch_finish();
    return __real_dvm_runtime_guest_sleep(duration);
}


/* [GC:] EN: Observe accepted ticket references to the real cycle seed. The
 * reference must be queued/rooted, not dropped as an ordinary nonfinal owner.
 * value remains independently live while this bound Fast callback runs.
 * RU: Наблюдать настоящую передачу ticket в seed цикла. Ссылка должна попасть
 * в очередь с корнем, а не сниматься как обычная непоследняя. value живёт весь
 * вызов на привязанном Fast через независимые ссылки графа и ticket. */
int __wrap_dvm_runtime_rc_defer(void *value)
{
    pthread_mutex_lock(&sync);
    int selected = nodes[0] && freed < node_count && D2H(value) == nodes[0] &&
                   !dvm_native_context_process(dvm_native_context_current());
    unsigned long roots = selected ? dvm_runtime_root_count(nodes[0]) : 0;
    if (selected)
    {
        fast_context = dvm_native_context_current();
    }
    pthread_mutex_unlock(&sync);
    publishing = selected;
    int result = __real_dvm_runtime_rc_defer(value);
    publishing = 0;
    if (selected)
    {
        pthread_mutex_lock(&sync);
        require(result == 1 && dvm_runtime_root_count(nodes[0]) == roots + 1,
                "shared cycle owner was dropped before publication");
        handoffs++;
        pthread_mutex_unlock(&sync);
    }
    return result;
}


/* [GC:] EN: Identify ticket staging FIFO, then refuse one later Cycles slot
 * in mandatory-rescue profiles. Source ticket roots are withdrawn before rescue.
 * queue/token remain unchanged on refusal; all other reservations are real.
 * RU: Определить FIFO ticket, затем отказать одному месту Cycles при
 * восстановлении. Корни ticket снимаются до передачи кандидату.
 * queue/token при отказе не меняются; остальные резервации настоящие. */
DVM_RetirementResult
__wrap_dvm_retirement_reserve(DVM_RetirementQueue *queue, unsigned long bytes, DVM_RetirementReservation *token)
{
    pthread_mutex_lock(&sync);
    if (publishing)
    {
        mandatory_queue = queue;
    }
    else if (mandatory_queue && queue != mandatory_queue && dvm_native_context_current() == fast_context && !rescued)
    {
        candidate_queue = queue;
        if ((CONTROL == 1 || CONTROL == 4 || CONTROL == 5) && !injected)
        {
            injected = 1;
            pthread_mutex_unlock(&sync);
            return DVM_RETIRE_FULL;
        }
    }
    pthread_mutex_unlock(&sync);
    return __real_dvm_retirement_reserve(queue, bytes, token);
}


/* [GC:] EN: Observe the real duplicate index while Fast still owns its FIFO
 * reference. queue/key are metadata; this callback never acquires a managed alias.
 * RU: Наблюдать настоящий дубликат индекса при живой ссылке Fast. queue/key —
 * метаданные; callback не получает управляемого алиаса. */
int __wrap_dvm_retirement_contains_key(DVM_RetirementQueue *queue, DVM_HeapKey key)
{
    int found = __real_dvm_retirement_contains_key(queue, key);
    pthread_mutex_lock(&sync);
    if (found && !publishing && queue == candidate_queue && dvm_native_context_current() == fast_context)
    {
        duplicate_wait = 1;
    }
    pthread_mutex_unlock(&sync);
    return found;
}


/* [GC:] EN: Validate that Fast hands off the existing FIFO root unchanged.
 * No payload access follows the real commit; Cycles can consume immediately.
 * RU: Проверить сохранение исходного корня при передаче Fast. После настоящего
 * commit данные не читаются: Cycles может сразу потребить опубликованную ссылку. */
int __wrap_dvm_retirement_commit_key(
    DVM_RetirementQueue *queue, DVM_RetirementReservation *token, void *value, DVM_HeapKey key, int *ready)
{
    pthread_mutex_lock(&sync);
    int selected = nodes[0] && freed < node_count && D2H(value) == nodes[0] && queue == candidate_queue &&
                   !publishing && dvm_native_context_current() == fast_context;
    if (selected)
    {
        unsigned expected_roots = (CONTROL == 2 || CONTROL == 6) && !rescued ? 2 : 1;
        require(dvm_runtime_root_count(nodes[0]) == expected_roots,
                "mandatory cycle root disappeared before destination commit");
        rescued++;
    }
    pthread_mutex_unlock(&sync);
    return __real_dvm_retirement_commit_key(queue, token, value, key, ready);
}


/* [GC:] EN: Delay consumption until the intended handoff/duplicate overlap
 * actually occurs. Returning retry keeps this test barrier off the stopped path.
 * After opening, run the unchanged real Cycles algorithm with context/budget.
 * RU: Задержать потребление до настоящего перекрытия передачи/дубликата. Возврат
 * повтора оставляет барьер вне остановленного пути. Затем выполнить настоящий
 * Cycles с исходными context/budget. */
unsigned int __wrap_dvm_runtime_rc_shared_candidates(DVM_NativeContext *context, size_t budget)
{
    pthread_mutex_lock(&sync);
    int held = nodes[0] && !cycles_open;
    pthread_mutex_unlock(&sync);
    return held ? 1 : __real_dvm_runtime_rc_shared_candidates(context, budget);
}


/* [GC:] EN: Observe each real record allocation free after shared graph claim.
 * pool/value remain borrowed until real poolfree returns. Publish only afterwards.
 * RU: Наблюдать освобождение каждой настоящей записи после shared-захвата графа.
 * pool/value заимствованы до возврата poolfree. Опубликовать завершение после него. */
void __wrap_poolfree(Pool *pool, void *value)
{
    pthread_mutex_lock(&sync);
    unsigned index = 0;
    while (index < node_count && (value != nodes[index] || !dvm_heap_pool_key_matches(pool, value, generations[index])))
    {
        index++;
    }
    if (index < node_count)
    {
        DVM_WorldState state;
        require(cycles_open && !expired && !deallocated[index] && dvm_world_state(dvm_gc_runtime.world, &state) &&
                    !state.stopping && !state.stopped && state.active >= 2,
                "cycle destruction must overlap admitted peers");
        require(dvm_runtime_root_count(value) == 0, "cycle graph still has a retirement root at physical free");
    }
    __real_poolfree(pool, value);
    if (index < node_count)
    {
        deallocated[index] = 1;
        freed++;
    }
    pthread_mutex_unlock(&sync);
}


/* [GC:] EN: Keep the duplicate candidate gated for eight actual Fast retries,
 * so unrelated startup work cannot conceal a missing retry result. context and
 * budget pass unchanged; this observer does not manufacture collector progress.
 * RU: Удержать дубликат на восемь настоящих ответов Fast, чтобы начальная работа
 * не скрыла отсутствие повтора. context/budget передаются без изменений;
 * наблюдатель не создаёт фиктивную работу сборщика.
 */
unsigned int __wrap_dvm_runtime_rc_shared_leaves(DVM_NativeContext *context, size_t budget)
{
    unsigned int work = __real_dvm_runtime_rc_shared_leaves(context, budget);
    pthread_mutex_lock(&sync);
    if (work && duplicate_wait && !cycles_open && !dvm_managed_owner_pending())
    {
        duplicate_passes++;
    }
    pthread_mutex_unlock(&sync);
    return work;
}


/* [GC:] EN: Reject a stopped Fast fallback while the pending graph still needs
 * rescue. Other collectors keep the existing mixed-runtime policy.
 * RU: Запретить остановленный fallback Fast, пока граф ждёт восстановления.
 * Остальные сборщики сохраняют существующую смешанную политику runtime. */
int __wrap_dvm_world_try_quiescent(DVM_NativeWorld *world, DVM_NativeContext *context)
{
    pthread_mutex_lock(&sync);
    require(!(context == fast_context && handoffs && freed != node_count && !released),
            "pending cycle owner requested stopped Fast admission");
    pthread_mutex_unlock(&sync);
    return __real_dvm_world_try_quiescent(world, context);
}


/* [GC:] EN: Build actual small/large cyclic records while privately owned, capture
 * one/two native ticket owners, and release them with no external managed root.
 * Real Cycles/Fast must physically free every record before peer release. frame remains
 * production-owned; the independent type creator owner survives all record frees.
 * RU: Построить настоящий малый/большой цикл под частным владением, захватить один/
 * два ticket и снять их без внешнего корня. Настоящие Cycles/Fast должны освободить
 * все узлы до выхода соседей. frame принадлежит runtime; ссылка создателя Type
 * живёт до освобождения всех записей. */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    pthread_mutex_lock(&sync);
    while (admitted != 2)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    uchar map = 0x80;
    Type *type = dtype(freeheap, sizeof(void *), &map, 1);
    require(type != NULL, "actual cyclic record descriptor");
    unsigned records = CONTROL == 7 ? MaxNodes : 2;
    void *values[MaxNodes];
    for (unsigned index = 0; index < records; index++)
    {
        values[index] = H2D(void *, heap(type));
    }
    for (unsigned index = 0; index < records; index++)
    {
        void *next = values[(index + 1) % records];
        require(dvm_refcount_retain_u(&D2H(next)->ref) == DVM_REFCOUNT_LIVE,
                "each private cycle edge owns one strong reference");
        memcpy(values[index], &next, sizeof(next));
        Setmark(D2H(next));
    }
    unsigned count = (CONTROL == 2 || CONTROL == 6) ? 2 : 1;
    DVM_OwnerTransfer tickets[2] = {{0}, {0}};
    for (unsigned index = 0; index < count; index++)
    {
        require(dvm_managed_owner_capture(values[0], &tickets[index]), "capture actual cyclic owner");
    }
    for (unsigned index = 0; index < records; index++)
    {
        require(dvm_refcount_drop_nonfinal_u(&D2H(values[index])->ref), "consume only private creator references");
    }
    pthread_mutex_lock(&sync);
    for (unsigned index = 0; index < records; index++)
    {
        nodes[index] = D2H(values[index]);
        generations[index] = dvm_heap_pool_key(heapmem, nodes[index]);
        require(generations[index].generation != 0, "real allocation generation remains stable until retirement");
    }
    node_count = records;
    pthread_mutex_unlock(&sync);
    for (unsigned index = 0; index < count; index++)
    {
        require(dvm_owner_transfer_cancel(&tickets[index]), "publish native cycle owner retirement");
    }
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent publishes native roots while waiting");
    unsigned long long deadline = milliseconds() + 5000;
    int success = 0;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        if (handoffs == count && !dvm_managed_owner_pending() &&
            (((CONTROL == 2 || CONTROL == 6) && duplicate_wait && duplicate_passes >= 8) ||
             ((CONTROL == 1 || CONTROL == 4 || CONTROL == 5) && rescued) || CONTROL == 0 || CONTROL == 7))
        {
            cycles_open = 1;
        }
        success = freed == node_count && !expired;
        pthread_mutex_unlock(&sync);
        if (success)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    require(success, "cycle owner rescue still requires heap quiescence");
    require(dvm_managed_owner_guard_entries() == 0, "native cycle guards retired");
    pthread_mutex_lock(&sync);
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    dvm_runtime_gc_native_enter(native);
    require(dvm_refcount_load_i(&type->ref) == 1, "all physical record owners released their descriptor");
    freetype(type);
    fprintf(stderr,
            "PASS cycle owner rescue records=%u handoffs=%u rescued=%u duplicate=%d injected=%d\n",
            records,
            handoffs,
            rescued,
            duplicate_wait,
            injected);
}
