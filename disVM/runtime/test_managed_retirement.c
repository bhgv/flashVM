/* Copyright (c) 2026 bhgv
 * EN: Keep real residents admitted while Fast releases managed ticket owners.
 * RU: Настоящие потоки остаются допущенными, пока Fast освобождает managed tickets.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runtime_gc.h"
#include "managed_owner.h"
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


unsigned int __real_dvm_managed_owner_drain_shared(DVM_NativeContext *, size_t);


int __real_dvm_runtime_rc_defer(void *);
static void *target;                  /* Identity kept alive by its managed ticket and then RC FIFO. */
static void *external;                /* Independently owned and rooted through the test. */
static DVM_OwnerTransfer transferred; /* Native alias published before observer startup. */
static int borrow_held;               /* Old explicit reader, under sync. */
static int borrow_release;            /* Parent permits read_end, under sync. */
static int borrow_wait_seen;          /* Actual Fast retry observed, under sync. */
static int final_handoffs;            /* Exact final-reference RC handoff count, under sync. */
static int external_handoffs;         /* Exact nonfinal-reference RC handoff count, under sync. */
static int refusal;                   /* Injected RC refusal was ownership-neutral, under sync. */


/* EN: Report a precise native fixture failure; message is borrowed constant text.
 * RU: Сообщить точную нативную ошибку теста; message — заимствованный текст. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL shared managed retirement: %s\n", message);
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
        fprintf(stderr, "MANAGED_RETIRE_PEERS_ADMITTED\n");
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
    require(dvm_owner_transfer_cancel(&transferred), "last native alias drops on detached reader");
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


/* [GC:] EN: Observe real RC handoffs only on the Fast service. Refusal control
 * leaves the reference, root and canonical guard untouched. All arguments remain
 * borrowed until the real RC operation consumes the existing reference.
 * RU: Наблюдать передачу в RC только на Fast. Контроль отказа сохраняет ссылку,
 * корень и каноническую блокировку. Аргумент заимствован до передачи владения RC.
 */
int __wrap_dvm_runtime_rc_defer(void *value)
{
    if (dvm_native_context_process(dvm_native_context_current()))
    {
        return __real_dvm_runtime_rc_defer(value);
    }
    pthread_mutex_lock(&sync);
    if ((!target || value != H2D(void *, (Heap *)target)) && value != external)
    {
        pthread_mutex_unlock(&sync);
        return __real_dvm_runtime_rc_defer(value);
    }
    require(!borrow_held, "ticket handoff before reader epoch elapsed");
    DVM_WorldState state;
    require(dvm_world_state(dvm_gc_runtime.world, &state) && !state.stopping && !state.stopped && state.active >= 2 &&
                !expired,
            "ticket handoff must overlap admitted peers without a world stop");
    int final = value != external;
    if (CONTROL == 4 && final && !refusal)
    {
        require(dvm_managed_owner_pending() == 2 && dvm_managed_owner_guard_entries() == 2 &&
                    dvm_runtime_root_count(target) == 1 && dvm_runtime_root_count(D2H(external)) == 2,
                "RC refusal preserves ticket references, roots and guards");
        refusal = 1;
        pthread_mutex_unlock(&sync);
        return 0;
    }
    require(dvm_runtime_root_count(D2H(value)) == (unsigned long)(final ? 1 : 2),
            "ticket root is retained before RC publication");
    pthread_mutex_unlock(&sync);
    int result = __real_dvm_runtime_rc_defer(value);
    require(result == 1 && dvm_runtime_root_count(D2H(value)) == 2, "RC owns a root before ticket root removal");
    pthread_mutex_lock(&sync);
    if (final)
    {
        final_handoffs++;
    }
    else
    {
        external_handoffs++;
    }
    pthread_mutex_unlock(&sync);
    return result;
}


/* [GC:] EN: Forward the bound Fast pass unless the disabled-path control applies.
 * Record a real epoch retry without granting the parent any payload permission.
 * RU: Передать проход Fast кроме контроля отключения. Учесть настоящий повтор
 * эпохи, не выдавая родителю разрешений на чтение управляемых данных.
 */
unsigned int __wrap_dvm_managed_owner_drain_shared(DVM_NativeContext *context, size_t budget)
{
#if CONTROL == 1
    (void)context;
    (void)budget;
    return 0;
#else
    unsigned int work = __real_dvm_managed_owner_drain_shared(context, budget);
    pthread_mutex_lock(&sync);
    if (work && borrow_held && dvm_managed_owner_pending() == 2)
    {
        borrow_wait_seen = 1;
        pthread_cond_broadcast(&changed);
    }
    pthread_mutex_unlock(&sync);
    return work;
#endif
}


/* [GC:] EN: Capture actual strings, export a native alias to a real reader thread
 * and hand off both final/nonfinal owners while two resident peers exclude heap
 * quiescence. frame stays production-owned; all external references are rooted.
 * RU: Захватить настоящие строки, передать нативный алиас потоку-читателю и
 * освободить последних/непоследних владельцев при двух допущенных соседях.
 * frame принадлежит runtime; независимые ссылки имеют собственные корни.
 */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    pthread_mutex_lock(&sync);
    while (admitted != 2)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    String *final = newstring(32);
    String *shared = newstring(64);
    pthread_mutex_lock(&sync);
    target = D2H(final);
    external = shared;
    pthread_mutex_unlock(&sync);
    require(dvm_runtime_root_add(D2H(shared)), "independent external owner root");
    DVM_OwnerTransfer captured = {0};
    DVM_OwnerTransfer peer_owner = {0};
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry alias = {0};
    require(dvm_managed_owner_capture(final, &captured) && dvm_managed_owner_capture(shared, &peer_owner),
            "capture real managed owners");
    require(dvm_owner_scope_open(&scope) && dvm_owner_scope_retain(&scope, &alias, captured.object, captured.ops),
            "retain a distinct native alias without duplicating the managed root");
    require(dvm_owner_transfer_cancel(&captured) && dvm_managed_owner_pending() == 0,
            "nonfinal native alias drop must not enqueue retirement");
    require(dvm_owner_scope_export(&alias, &transferred) && dvm_owner_scope_close(&scope),
            "export native alias without moving the resident process");
    destroy(final);
    FVM_PortThread *observer = fvm_port_thread_start(hold_borrow, dvm_gc_runtime.world);
    require(observer != NULL, "reader starts with exclusively received native alias");
    pthread_mutex_lock(&sync);
    while (!borrow_held)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    require(dvm_owner_transfer_cancel(&peer_owner), "queue second real managed ticket");
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent native roots published while waiting for handoffs");
    unsigned long long deadline = milliseconds() + 5000;
    int success = 0;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        if (borrow_wait_seen && !borrow_release)
        {
            require(dvm_managed_owner_pending() == 2 && dvm_runtime_root_count(target) == 1 &&
                        dvm_runtime_root_count(D2H(shared)) == 2,
                    "old reader keeps both queued ticket roots intact");
            borrow_release = 1;
            pthread_cond_broadcast(&changed);
        }
        success = final_handoffs == 1 && external_handoffs == 1 && dvm_managed_owner_pending() == 0 && !expired;
        pthread_mutex_unlock(&sync);
        if (success)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    require(success, "managed ticket retirement still requires heap quiescence");
    require(dvm_runtime_root_count(target) == 1 && dvm_runtime_root_count(D2H(shared)) == 1,
            "managed ticket root did not transfer exactly once");
    require(dvm_managed_owner_guard_entries() == 0, "canonical ticket guards retired");
    require(CONTROL != 4 || refusal, "RC refusal path was not exercised");
    pthread_mutex_lock(&sync);
    released = 1;
    borrow_release = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    fvm_port_thread_join(observer);
    dvm_runtime_gc_native_enter(native);
    require(dvm_refcount_load_u(&D2H(shared)->ref) == 1,
            "independent external managed reference survives ticket retirement");
    require(dvm_runtime_root_remove(D2H(shared)), "external owner removes only its own root");
    pthread_mutex_lock(&sync);
    external = NULL;
    target = NULL;
    pthread_mutex_unlock(&sync);
    destroy(shared);
    fprintf(stderr, "PASS shared managed retirement final=1 external=1 admitted=2 reader_wait=1 refusal=%d\n", refusal);
}
