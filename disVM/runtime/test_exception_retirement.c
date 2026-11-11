/* Copyright (c) 2026 bhgv
 * EN: Keep real residents admitted while Fast releases emergency string owners.
 * RU: Настоящие потоки остаются допущенными, пока Fast освобождает резервные строки.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runtime_gc.h"
#include "exception_host.h"
#include "root_registry.h"
#include "disvm/refcount.h"
#include "gc/common/runtime_state.h"
#include "disvm/native_owner.h"
#include "thread_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;  /* Protect relay state, never VM payload. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Relay and final release notifications. */
static unsigned admitted;                                 /* Peers have published shared roots, under sync. */
static unsigned turn;                                     /* Only this peer may briefly advance its admission epoch. */
static int released;                                      /* Parent completed the live-owner check. */
static int expired;                                       /* A finite peer watchdog observed missing parent progress. */


int __real_dvm_runtime_guest_sleep(unsigned long);


void __real_Sys_werrstr(void *);


void __real_poolfree(Pool *, void *);


unsigned int __real_dvm_runtime_exception_retire_shared(DVM_NativeContext *, size_t);
static void *target;         /* Strongly owned by reserve/Fast until observed poolfree. */
static unsigned int frees;   /* Exact target destruction, protected by sync. */
static int borrow_held;      /* Explicit reader protects target, under sync. */
static int borrow_release;   /* Parent permits the reader to finish, under sync. */
static int borrow_wait_seen; /* Fast actually observed a protected pending entry. */


/* EN: Report a precise native fixture failure; message is borrowed constant text.
 * RU: Сообщить точную нативную ошибку теста; message — заимствованный текст. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL shared exception retirement: %s\n", message);
        exit(12);
    }
}


/* EN: Return monotonic milliseconds for a bounded diagnostic, never a success oracle.
 * RU: Вернуть монотонное время для ограничения диагностики, не критерия успеха. */
static unsigned long long milliseconds(void)
{
    struct timespec now;
    require(!clock_gettime(CLOCK_MONOTONIC, &now), "monotonic clock available");
    return (unsigned long long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}


/* EN: Hold the real sleep(0) before its native release, without touching registers.
 * Two peers alternate epoch advancement while the other stays admitted/shared;
 * quiescent-only fallback GC therefore cannot explain successful retirement.
 * Forward the original scalar duration unchanged after the parent releases both.
 * RU: Задержать настоящий sleep(0) до native release, не меняя регистры.
 * Два соседа по очереди обновляют эпоху, пока другой остаётся допущенным;
 * сборка только при общем покое не может объяснить успешное освобождение.
 * После разрешения родителя передать исходную длительность без изменений. */
int __wrap_dvm_runtime_guest_sleep(unsigned long duration)
{
    require(duration == 0, "fixture uses only the genuine zero-duration sleep");
    require(dvm_runtime_gc_dispatch_share(), "peer enters shared admission with real roots");
    DVM_HeapOwner *owner = dvm_native_owner_current();
    unsigned long long deadline = milliseconds() + 15000;
    pthread_mutex_lock(&sync);
    unsigned id = admitted++;
    require(id < 2, "exactly two admitted peers");
    if (admitted == 2)
    {
        fprintf(stderr, "EXCEPTION_RETIRE_PEERS_ADMITTED\n");
        pthread_cond_broadcast(&changed);
    }
    while (!released && milliseconds() < deadline)
    {
        if (admitted == 2 && turn == id)
        { /* The other peer is still shared and cannot leave while sync is held. */
            require(dvm_native_owner_leave_shared(owner) && dvm_native_owner_enter_shared(owner),
                    "advance one grace period without heap quiescence");
            turn = 1 - id;
            pthread_cond_broadcast(&changed);
        }
        pthread_mutex_unlock(&sync);
        fvm_port_thread_sleep_us(1000);
        pthread_mutex_lock(&sync);
    }
    if (!released)
    {
        expired = 1;
        released = 1;
        pthread_cond_broadcast(&changed);
    }
    pthread_mutex_unlock(&sync);
    dvm_runtime_gc_dispatch_finish();
    return __real_dvm_runtime_guest_sleep(duration);
}


/* [GC:] EN: Borrow target under a genuine registered reader epoch until parent
 * permits completion. world is borrowed through join; access no pointer after
 * read_end. The original reserve owns the string when this thread starts.
 * RU: Заимствовать target под настоящей зарегистрированной эпохой до разрешения
 * родителя. world живёт до join; после read_end указатель не используется.
 * При запуске строкой владеет первоначальный резерв.
 */
static void hold_borrow(void *argument)
{
    DVM_NativeWorld *world = argument;
    DVM_NativeContext *context = dvm_native_context_create(sizeof(unsigned long), NULL);
    require(context && dvm_native_context_bind(context), "borrow observer binds");
    DVM_WorldParticipant *reader = dvm_world_join_service(world, context);
    require(reader && dvm_world_read_begin(reader), "registered reader epoch begins");
    pthread_mutex_lock(&sync);
    String *value = H2D(String *, (Heap *)target);
    int length = value->len;
    borrow_held = 1;
    pthread_cond_broadcast(&changed);
    while (!borrow_release)
    {
        pthread_cond_wait(&changed, &sync);
    }
    require(value->len == length, "borrowed string remains readable before read_end");
    require(dvm_world_read_end(reader), "reader epoch ends after final access");
    borrow_held = 0;
    pthread_mutex_unlock(&sync);
    require(dvm_world_detach(reader), "borrow observer detaches");
    require(dvm_native_context_unbind(context) && dvm_native_context_destroy(context),
            "borrow observer releases its native context");
}


/* [GC:] EN: Observe actual poolfree of target, forwarding every allocation unchanged.
 * Verify reclamation under shared admission with live peers, never a world stop.
 * RU: Наблюдать настоящий poolfree target, сохраняя все освобождения.
 * Проверить shared-допуск при живых соседях без общей остановки.
 */
void __wrap_poolfree(Pool *pool, void *value)
{
    pthread_mutex_lock(&sync);
    if (value == target)
    {
        require(!borrow_held, "emergency string freed before reader epoch elapsed");
        DVM_WorldState state;
        require(dvm_world_state(dvm_gc_runtime.world, &state), "inspect actual free admission");
        require(!state.stopping && !state.stopped && state.active >= 2 && !expired,
                "emergency string freed only under shared admission with live peers");
        require(dvm_refcount_load_u(&((Heap *)value)->ref) == 0, "last reference claimed exactly once");
        frees++;
        target = NULL;
    }
    pthread_mutex_unlock(&sync);
    __real_poolfree(pool, value);
}


/* [GC:] EN: Run production shared retirement unless the negative control disables
 * it. context/budget remain unchanged; the control must fail before peers release.
 * RU: Выполнить настоящий shared-проход, кроме отрицательного контроля.
 * context/budget не меняются; контроль должен отказать до освобождения соседей.
 */
unsigned int __wrap_dvm_runtime_exception_retire_shared(DVM_NativeContext *context, size_t budget)
{
#if CONTROL
    (void)context;
    (void)budget;
    return 0;
#else
    unsigned int work = __real_dvm_runtime_exception_retire_shared(context, budget);
    pthread_mutex_lock(&sync);
    if (work && borrow_held && target)
    {
        borrow_wait_seen = 1;
        pthread_cond_broadcast(&changed);
    }
    pthread_mutex_unlock(&sync);
    return work;
#endif
}


/* [GC:] EN: Retire a final reserve and a reserve with an independent rooted owner.
 * Keep frame production-owned. The parent's native interval permits epoch renewal
 * while two peers prevent both STW and quiescent-only fallback from succeeding.
 * RU: Освободить последний резерв и резерв с независимым укоренённым владельцем.
 * Кадр принадлежит runtime. Native-интервал родителя разрешает смену эпох, пока
 * два соседа исключают и STW, и резервную сборку при общем покое.
 */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    pthread_mutex_lock(&sync);
    while (admitted != 2)
    {
        pthread_cond_wait(&changed, &sync);
    }
    String *final = dvm_runtime_exception_reserve_value();
    require(final != NULL, "real initial emergency owner");
    target = D2H(final);
    pthread_mutex_unlock(&sync);
    FVM_PortThread *observer = fvm_port_thread_start(hold_borrow, dvm_gc_runtime.world);
    require(observer != NULL, "native borrow observer starts");
    pthread_mutex_lock(&sync);
    while (!borrow_held)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    dvm_runtime_exception_reserve_stop();
    dvm_runtime_exception_reserve_start();
    String *shared = dvm_runtime_exception_reserve_value();
    require(shared != NULL, "real second emergency owner");
    Heap *heap = D2H(shared);
    require(dvm_refcount_retain_u(&heap->ref) == DVM_REFCOUNT_LIVE && dvm_runtime_root_add(heap),
            "independent retained/rooted string owner");
    dvm_runtime_exception_reserve_stop();
    dvm_runtime_exception_reserve_start();
    require(dvm_runtime_exception_reserve_value() != NULL, "replacement emergency owner");
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent publishes native roots during retirement");
    unsigned long long deadline = milliseconds() + 5000;
    int success = 0;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        if (borrow_wait_seen && !borrow_release)
        {
            require(frees == 0 && dvm_refcount_load_u(&heap->ref) == 2,
                    "Fast retained both queue owners while the old reader remained");
            borrow_release = 1;
            pthread_cond_broadcast(&changed);
        }
        success = frees == 1 && !expired && dvm_refcount_load_u(&heap->ref) == 1;
        pthread_mutex_unlock(&sync);
        if (success)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    require(success, "emergency retirement still requires heap quiescence");
    pthread_mutex_lock(&sync);
    released = 1;
    borrow_release = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    fvm_port_thread_join(observer);
    dvm_runtime_gc_native_enter(native);
    require(success, "emergency retirement still requires heap quiescence");
    require(dvm_runtime_root_count(heap) == 1, "Fast removed only its own root occurrence");
    require(dvm_runtime_root_remove(heap), "independent root removed by its owner");
    destroy(shared);
    fprintf(stderr, "PASS shared exception retirement final=1 external=1 admitted=2 reader_wait=1\n");
}
