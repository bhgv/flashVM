/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real Duke hard-limit refusal preserves owners and permits an independent graph.
 * RU: Настоящий отказ предела Duke сохраняет владельцев и пропускает независимый граф.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "heap_pool.h"
#include "runtime_gc.h"
#include "runtime_rc.h"
#include "retirement_queue.h"
#include "gc/common/candidate_pass.h"
#include "root_registry.h"
#include "gc/common/runtime_state.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "thread_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern Pool *heapmem;                                     /* Real managed arena through all joins. */
static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;  /* Fixture predicates only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Peer readiness. */
static unsigned admitted;                                 /* Real shared peers, under sync. */
static unsigned turn;                                     /* Peer permitted to advance an epoch, under sync. */
static int released;                        /* Parent permits original sleep frames to return, under sync. */
static int expired;                         /* Watchdog failure, under sync. */
static Heap *targets[2];                    /* Original array identities; queue owns references after publication. */
static DVM_HeapKey keys[2];                 /* Generations safe after physical destruction. */
static unsigned freed[2];                   /* Physical frees under sync. */
static unsigned refusals;                   /* Real reserve refusals while moving the large graph. */
static int pressure = 1;                    /* Hold both fallback destinations unavailable until smaller graph frees. */
static DVM_RetirementQueue *fallback_queue; /* Exact retained destination borrowed from actual Duke callback. */
static DVM_RetirementQueue *duke_queue;     /* Borrowed through all collector joins, under sync. */
static _Thread_local int in_duke_run;       /* Only this collector is inside the real Duke callback. */


int __real_dvm_runtime_guest_sleep(unsigned long);


void __real_Sys_werrstr(void *);


void __real_poolfree(Pool *, void *);


DVM_RetirementResult __real_dvm_retirement_reserve(DVM_RetirementQueue *, unsigned long, DVM_RetirementReservation *);
unsigned int
__real_dvm_gc_duke_run(DVM_NativeContext *, size_t, DVM_NativeWorld *, DVM_RetirementQueue *, DVM_RetirementQueue *);


/* EN: Fail on false condition with borrowed message; no recovery or ownership transfer.
 * RU: При ложном condition завершиться с заимствованным message; восстановления и передачи ссылок нет. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL Duke queue progress: %s\n", message);
        exit(12);
    }
}


/* EN: Return monotonic milliseconds for failure deadlines only; no arguments or shared state.
 * RU: Вернуть монотонные миллисекунды только для предела отказа; аргументов и общих данных нет. */
static unsigned long long milliseconds(void)
{
    struct timespec now;
    require(!clock_gettime(CLOCK_MONOTONIC, &now), "monotonic clock");
    return (unsigned long long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}


/* [GC:] EN: Keep two real sleep frames shared while alternating reader epochs.
 * duration is forwarded unchanged after release; no Prog/register replacement.
 * RU: Удержать два настоящих sleep-кадра в shared, чередуя эпохи читателей.
 * duration передаётся после разрешения без изменения; Prog/регистры не заменяются. */
int __wrap_dvm_runtime_guest_sleep(unsigned long duration)
{
    require(!duration && dvm_runtime_gc_dispatch_share(), "peer shared admission");
    DVM_HeapOwner *owner = dvm_native_owner_current();
    unsigned long long deadline = milliseconds() + 12000;
    pthread_mutex_lock(&sync);
    unsigned id = admitted++;
    require(id < 2, "two permanent peers");
    pthread_cond_broadcast(&changed);
    while (!released && milliseconds() < deadline)
    {
        if (admitted == 2 && turn == id)
        {
            require(dvm_native_owner_leave_shared(owner) && dvm_native_owner_enter_shared(owner), "peer epoch");
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


/* [GC:] EN: Identify the actual Duke queue then forward every input unchanged.
 * Queue/world are borrowed through joins; never replace collector results.
 * RU: Определить настоящую очередь Duke, передать параметры без изменения.
 * Очередь/мир живут до join; ответы сборщика не заменяются. */
unsigned int __wrap_dvm_gc_duke_run(DVM_NativeContext *context,
                                    size_t budget,
                                    DVM_NativeWorld *world,
                                    DVM_RetirementQueue *queue,
                                    DVM_RetirementQueue *fallback)
{
    pthread_mutex_lock(&sync);
    require(!duke_queue || duke_queue == queue, "stable actual Duke queue");
    duke_queue = queue;
    fallback_queue = fallback;
    pthread_mutex_unlock(&sync);
    in_duke_run = 1;
    unsigned int result = __real_dvm_gc_duke_run(context, budget, world, queue, fallback);
    in_duke_run = 0;
    return result;
}


/* [GC:] EN: Refuse actual reservations for the oversized candidate without
 * modifying queue/token; forward other calls unchanged. CONTROL selects FULL/BYTES.
 * RU: Отклонить настоящий резерв слишком большого кандидата без изменения
 * очереди/token; прочие вызовы передать без изменений. CONTROL задаёт FULL/BYTES. */
DVM_RetirementResult
__wrap_dvm_retirement_reserve(DVM_RetirementQueue *queue, unsigned long bytes, DVM_RetirementReservation *token)
{
    pthread_mutex_lock(&sync);
    int hard_head = in_duke_run && targets[0] && dvm_retirement_peek(duke_queue) == H2D(void *, targets[0]);
    require(!hard_head || queue == fallback_queue, "hard-limit graph must leave the shared rescue loop");
    int decline = hard_head && pressure;
    refusals += decline;
    pthread_mutex_unlock(&sync);
    if (decline)
    {
        return CONTROL == 1 ? DVM_RETIRE_BYTES : DVM_RETIRE_FULL;
    }
    return __real_dvm_retirement_reserve(queue, bytes, token);
}


/* [GC:] EN: Observe actual generation-matched array destruction before forwarding
 * pool/value unchanged. Smaller graph must free under shared admission while
 * pressure and both peers remain; large graph may free only after release.
 * RU: Учесть настоящие free массивов нужного поколения до передачи pool/value.
 * Меньший граф обязан освободиться в shared при давлении и активных соседях;
 * большой — только после разрешения. */
void __wrap_poolfree(Pool *pool, void *value)
{
    pthread_mutex_lock(&sync);
    DVM_HeapKey key = dvm_heap_pool_key(pool, value);
    for (unsigned index = 0; index < 2; index++)
    {
        if (value == targets[index] && key.generation == keys[index].generation)
        {
            DVM_WorldState state;
            require(!freed[index] && !dvm_runtime_root_count(value) && dvm_world_state(dvm_gc_runtime.world, &state),
                    "physical free withdraws source root exactly once");
            if (index == 1)
            {
                require(pressure && !released && !expired && !state.stopped && state.active >= 2 &&
                            !dvm_refcount_load_u(&((Heap *)value)->ref),
                        "smaller graph frees shared behind refused head");
            }
            else
            {
                require(!pressure, "large graph survives fallback refusal");
            }
            freed[index]++;
        }
    }
    pthread_mutex_unlock(&sync);
    __real_poolfree(pool, value);
}


/* [GC:] EN: Create a real mapped array of count slots with one actual self-edge.
 * Creator owns returned heap; element descriptor is independently borrowed and
 * retained for the array. All other fields are legitimate nil values.
 * RU: Создать настоящий mapped-массив count с одним собственным ребром на себя.
 * Создатель владеет heap; дескриптор элемента заимствован и удерживается массивом.
 * Прочие поля содержат допустимый nil. */
static Heap *make_cycle(Type *element, size_t count)
{
    Heap *heap = nheap(sizeof(Array) + count * sizeof(void *));
    require(dvm_refcount_retain_i(&Tarray.ref) == DVM_REFCOUNT_LIVE &&
                dvm_refcount_retain_i(&element->ref) == DVM_REFCOUNT_LIVE,
            "array descriptor references");
    heap->t = &Tarray;
    Array *array = H2D(Array *, heap);
    array->root = H;
    array->t = element;
    array->len = count;
    array->data = (uchar *)(array + 1);
    void **fields = (void **)array->data;
    for (size_t index = 0; index < count; index++)
    {
        fields[index] = H;
    }
    require(dvm_refcount_retain_u(&heap->ref) == DVM_REFCOUNT_LIVE, "real self edge reference");
    fields[0] = array;
    Setmark(heap);
    return heap;
}


/* [GC:] EN: Publish a genuine hard-limit cycle, observe its refused moves, then
 * publish a smaller Duke cycle. Require physical shared completion before lifting
 * pressure, preserve large owner/root, and finish real quiescent tracing afterward.
 * frame is forwarded unchanged; creator keeps element descriptor through all frees.
 * RU: Передать цикл за настоящим пределом снимка, дождаться отказов передачи,
 * передать меньший цикл Duke. Проверить shared-free до снятия давления и сохранение
 * ссылки/корня большого; затем завершить настоящую трассировку в покое. frame
 * передать без изменения; владеть дескриптором до всех free. */
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
    Type *element = dtype(freeheap, sizeof(void *), &map, 1);
    require(element != NULL, "actual mapped element descriptor");
    Heap *big = make_cycle(element, 70000);
    Heap *small = make_cycle(element, 2300);
    pthread_mutex_lock(&sync);
    targets[0] = big;
    targets[1] = small;
    keys[0] = dvm_heap_pool_key(heapmem, big);
    keys[1] = dvm_heap_pool_key(heapmem, small);
    pthread_mutex_unlock(&sync);
    require(dvm_runtime_rc_defer(H2D(void *, big)), "publish real oversized cycle seed");
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent native root publication");
    unsigned long long deadline = milliseconds() + 7000;
    unsigned observed = 0;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        observed = refusals;
        pthread_mutex_unlock(&sync);
        if (observed >= 4)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    require(observed >= 4, "actual Duke hard-limit transfer encounters pressure");
    dvm_runtime_gc_native_enter(native);
    require(dvm_runtime_rc_defer(H2D(void *, small)), "publish independent Duke cycle");
    native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent leaves for concurrent smaller graph reclamation");
    deadline = milliseconds() + 7000;
    int done = 0;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        done = freed[1] && !expired;
        pthread_mutex_unlock(&sync);
        if (done)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    require(done, "independent Duke graph must bypass hard-refused head");
    pthread_mutex_lock(&sync);
    require(!freed[0] && dvm_runtime_root_count(big) == 1 && dvm_refcount_load_u(&big->ref) == 2,
            "oversized source retains exact seed root and self/queue references");
    fprintf(stderr, "DUKE_QUEUE_SHARED head_owned=1 follower_freed=1 refusals=%u\n", refusals);
    pressure = 0;
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    require(dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_CYCLES) &&
                dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_FAST) &&
                dvm_gc_workers_wait_idle(dvm_gc_runtime.workers),
            "actual collector cleanup after peer release");
    require(dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_CYCLES) &&
                dvm_gc_workers_wait_idle(dvm_gc_runtime.workers),
            "real color rotations after final queue-root withdrawal");
    dvm_runtime_gc_native_enter(native);
    pthread_mutex_lock(&sync);
    require(freed[0] && freed[1], "both actual arrays physically reclaimed");
    pthread_mutex_unlock(&sync);
    require(dvm_refcount_load_i(&element->ref) == 1, "independent element descriptor creator remains");
    freetype(element);
    fprintf(
        stderr, "PASS Duke queue progress hard=70000 independent=2300 pressure=%s\n", CONTROL == 1 ? "BYTES" : "FULL");
}
