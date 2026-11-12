/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real frame packets bypass an unsupported child beside admitted residents.
 * RU: Настоящие пакеты кадров обходят неподдержанного ребёнка при активных соседях.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "heap_pool.h"
#include "runtime_gc.h"
#include "runtime_rc.h"
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
static int released;         /* Parent permits original sleep frames to return, under sync. */
static int expired;          /* Watchdog failure, under sync. */
static Heap *targets[5];     /* Independent original owners until packet publication. */
static DVM_HeapKey keys[5];  /* Generations retained after actual physical frees. */
static unsigned freed[5];    /* Exactly-once physical-free observations, under sync. */
static unsigned zero_passes; /* Real no-progress shared passes with only blocked suffix, under sync. */
static unsigned callbacks;   /* Actual unsupported destructor calls, under sync. */


int __real_dvm_runtime_guest_sleep(unsigned long);


void __real_Sys_werrstr(void *);


void __real_poolfree(Pool *, void *);


unsigned int __real_dvm_runtime_rc_shared_leaves(DVM_NativeContext *, size_t);


/* EN: Fail on false condition with borrowed message; no recovery or ownership transfer.
 * RU: При ложном condition завершиться с заимствованным message; восстановления и передачи ссылок нет. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL packet fairness: %s\n", message);
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
    if (CONTROL == 3 || CONTROL == 102)
    {
        require(!expired, "shared retry must not wait for unrelated Fast backlog");
    }
    pthread_mutex_unlock(&sync);
    dvm_runtime_gc_dispatch_finish();
    return __real_dvm_runtime_guest_sleep(duration);
}


/* [GC:] EN: Finalize borrowed heap only under actual retained admission after peer release.
 * Forward swept to freeheap; caller owns header/type, no callback runs shared.
 * RU: Завершить heap только под настоящим резервным допуском после ухода соседей.
 * Передать swept в freeheap; вызывающий владеет заголовком/типом, shared-вызов запрещён. */
static void unknown_destructor(Heap *heap, int swept)
{
    pthread_mutex_lock(&sync);
    DVM_WorldState state;
    require(heap == targets[1] && released && !callbacks && dvm_world_state(dvm_gc_runtime.world, &state) &&
                state.stopped && !state.active,
            "unknown callback requires retained admission");
    callbacks++;
    pthread_mutex_unlock(&sync);
    freeheap(heap, swept);
}


/* [GC:] EN: Observe actual frees of tracked generations before forwarding pool/value unchanged.
 * Require roots/count withdrawal and exactly-once freeing. Early independent children
 * require admitted peers; blocked suffix may free only after peer release.
 * RU: Учесть реальные free выбранных поколений до передачи pool/value без изменения.
 * Проверить снятые корни/счётчик и однократность. Независимые дети освобождаются
 * при активных соседях; заблокированный остаток — только после их ухода. */
void __wrap_poolfree(Pool *pool, void *value)
{
    pthread_mutex_lock(&sync);
    DVM_HeapKey key = dvm_heap_pool_key(pool, value);
    for (unsigned index = 0; index < 5; index++)
    {
        if (value == targets[index] && key.generation == keys[index].generation)
        {
            require(!freed[index] && !dvm_runtime_root_count(value) && !dvm_refcount_load_u(&((Heap *)value)->ref),
                    "physical free withdraws root/count exactly once");
            if (index == 0 || index >= 3)
            {
                DVM_WorldState state;
                require(!released && !expired && dvm_world_state(dvm_gc_runtime.world, &state) && !state.stopped &&
                            state.active >= 2,
                        "independent packets free beside admitted peers");
            }
            else
            {
                require(released, "blocked suffix stays owned until retained cleanup");
            }
            freed[index]++;
        }
    }
    pthread_mutex_unlock(&sync);
    __real_poolfree(pool, value);
}


/* [GC:] EN: Run the real Fast callback on context with its budget, lowering it
 * to one/two units for selected positive profiles. Observe zero-work returns
 * after independent frees; rotation cannot manufacture shared progress.
 * RU: Выполнить настоящий Fast на context с budget, уменьшая его до одного/двух
 * шагов в выбранных профилях. Учесть нулевые ответы после независимых free;
 * перемещение пакета не должно создавать видимость выполненной работы. */
unsigned int __wrap_dvm_runtime_rc_shared_leaves(DVM_NativeContext *context, size_t budget)
{
    if (CONTROL == 1 || CONTROL == 2)
    {
        budget = CONTROL;
    }
    unsigned int result = __real_dvm_runtime_rc_shared_leaves(context, budget);
    pthread_mutex_lock(&sync);
    if (freed[0] && freed[3] && freed[4] && !released && !result)
    {
        zero_passes++;
    }
    pthread_mutex_unlock(&sync);
    return result;
}


/* [GC:] EN: Publish three actual mapped frame owners in explicit capture order.
 * Values are borrowed until successful transfer, then caller clears its frame.
 * Type metadata stays locally owned; refuse no partial transfer or synthetic roots.
 * RU: Опубликовать три настоящих владельца кадра в заданном порядке захвата.
 * values заимствованы до успешной передачи, затем кадр очищается. Тип остаётся
 * личным; частичная передача и искусственные корни не допускаются. */
static void publish_mixed(void **values)
{
    enum
    {
        Slots = sizeof(Frame) / sizeof(void *) + 3
    }; /* Header plus three mapped owners. */

    void *frame[Slots];
    uchar map[(Slots + 7) / 8];
    memset(frame, 0, sizeof(frame));
    memset(map, 0, sizeof(map));
    for (unsigned slot = sizeof(Frame) / sizeof(void *); slot < Slots; slot++)
    {
        map[slot / 8] |= 0x80U >> (slot % 8);
    }
    unsigned next = 0;
    for (unsigned byte = 0; byte < sizeof(map); byte++)
    {
        for (unsigned bit = 0; bit < 8; bit++)
        {
            if (map[byte] & (1U << bit))
            {
                frame[byte * 8 + 7 - bit] = values[next++];
            }
        }
    }
    Type *type = dtype(freeheap, sizeof(frame), map, sizeof(map));
    require(type && next == 3 && dvm_runtime_rc_transfer_frame(frame, type, NULL) == 1,
            "publish real mixed mapped frame packet");
    memset(frame, 0, sizeof(frame));
    freetype(type);
}


/* [GC:] EN: Forward frame, publish mixed and independent owner packets, then require
 * physical independent frees while peers stay admitted. Selected profiles retry
 * shared publication with an unrelated retained backlog. Preserve unknown/suffix roots
 * and counts until real retained cleanup; own descriptor until its heap is gone.
 * RU: Выполнить frame, передать смешанный и независимые пакеты, потребовать реальные
 * free при активных соседях. Выбранные профили повторяют shared-публикацию при
 * непустой посторонней резервной очереди. Корни и ссылки unknown/остатка живут до настоящей
 * резервной очистки; владеть дескриптором до удаления его объекта. */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    pthread_mutex_lock(&sync);
    while (admitted != 2)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    Type *unknown_type = dtype(unknown_destructor, sizeof(void *), NULL, 0);
    require(unknown_type != NULL, "unsupported native descriptor");
    Heap *objects[5];
    for (unsigned index = 0; index < 5; index++)
    {
        objects[index] = index == 1 ? heapz(unknown_type) : D2H(newstring(16));
    }
    pthread_mutex_lock(&sync);
    for (unsigned index = 0; index < 5; index++)
    {
        targets[index] = objects[index];
        keys[index] = dvm_heap_pool_key(heapmem, objects[index]);
    }
    pthread_mutex_unlock(&sync);
    require((CONTROL == 3 || CONTROL == 102) ? dvm_runtime_gc_shared_try_enter() : dvm_runtime_gc_dispatch_share(),
            "parent shared frame publication");
    void *mixed[] = {H2D(void *, objects[0]), H2D(void *, objects[1]), H2D(void *, objects[2])};
    publish_mixed(mixed);
    memset(mixed, 0, sizeof(mixed));
    for (unsigned index = 3; index < 5; index++)
    {
        require(dvm_runtime_rc_transfer_frame(NULL, NULL, H2D(void *, objects[index])) == 1,
                "independent actual owner packet");
    }
    if (CONTROL == 3 || CONTROL == 102)
    {
        DVM_NativeContext *context = dvm_native_context_current();
        dvm_runtime_gc_shared_publish();
        dvm_runtime_gc_shared_drain();
        DVM_HeapOwnerStats phase;
        require(dvm_native_context_current() == context &&
                    dvm_native_owner_statistics(dvm_native_owner_current(), &phase) && phase.phase == DVM_HEAP_SHARED,
                "shared retry preserves context and admission");
    }
    if (CONTROL == 3 || CONTROL == 102)
    {
        dvm_runtime_gc_shared_finish();
    }
    else
    {
        dvm_runtime_gc_dispatch_finish();
    }
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent leaves with real native frame roots");
    unsigned long long deadline = milliseconds() + 7000;
    int done = 0;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        done = freed[0] && freed[3] && freed[4] && zero_passes >= 4 && !expired;
        pthread_mutex_unlock(&sync);
        if (done)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    pthread_mutex_lock(&sync);
    require(freed[0] && freed[3] && freed[4], "independent packets must bypass unsupported head before quiescence");
    require(done, "blocked rotation must yield without inventing progress");
    require(!callbacks && !freed[1] && !freed[2] && dvm_runtime_root_count(objects[1]) == 1 &&
                dvm_runtime_root_count(objects[2]) == 1 && dvm_refcount_load_u(&objects[1]->ref) == 1 &&
                dvm_refcount_load_u(&objects[2]->ref) == 1,
            "blocked cursor and suffix preserve exact owners/roots");
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    require(dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_FAST) &&
                dvm_gc_workers_wait_idle(dvm_gc_runtime.workers),
            "real retained packet cleanup completes");
    dvm_runtime_gc_native_enter(native);
    pthread_mutex_lock(&sync);
    require(callbacks == 1 && freed[1] && freed[2], "unknown and suffix free once after peer release");
    pthread_mutex_unlock(&sync);
    require(dvm_refcount_load_i(&unknown_type->ref) == 1, "descriptor creator retains sole type reference");
    freetype(unknown_type);
    fprintf(stderr, "PASS packet fairness independent=3 blocked=2 retained=1 peers=2\n");
}
