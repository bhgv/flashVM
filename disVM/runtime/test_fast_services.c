/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real Fast service fairness across independent reader cutoffs.
 * RU: Справедливость настоящих служб Fast с независимыми эпохами читателей.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "managed_owner.h"
#include "exception_host.h"
#include "runtime_gc.h"
#include "root_registry.h"
#include "gc/collector_passes.h"
#include "gc/common/runtime_state.h"
#include "disvm/native_owner.h"
#include "disvm/refcount.h"
#include "thread_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define Scenario ((CONTROL % 100) / 4)                    /* Both queues, tickets only, or reserves only. */
#define WantTickets (Scenario != 2)                       /* This scenario publishes a real managed ticket. */
#define WantReserves (Scenario != 1)                      /* This scenario retires a real emergency string. */

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;  /* Fixture state, not heap exclusion. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Actual reader and peer handoffs. */
static unsigned peers;                                    /* Admitted real guest frames, under sync. */
static unsigned turn;                                     /* Only this peer may advance its reader epoch. */
static int peers_release;                                 /* Parent permits both guests to resume. */
static int armed;                                         /* Parent is preparing independent queue cutoffs. */
static int enabled;                                       /* All real owners are published. */
static int finished;                                      /* Normal coordinator budget can resume. */
static int inside;                                        /* Fast has entered a production shared pass. */
static unsigned passes;                                   /* Completed small-budget attempts during the probe. */
static int reader_held;                                   /* Actual borrow protects ticket and reserve payloads. */
static int reader_release;                                /* Parent has observed unrelated RC progress. */
static unsigned ticket_retries;                           /* Genuine reader-blocked ticket attempts. */
static unsigned reserve_retries;                          /* Genuine reader-blocked reserve attempts. */
static unsigned physical_frees;                           /* Actual last-reference target poolfree count. */
static Heap *target;                                      /* Earlier RC owner, cleared at its physical free. */
static Heap *ticket_heap;                                 /* Real managed ticket owner protected by reader. */
static Heap *reserve_heap;                                /* Real emergency FIFO owner protected by reader. */


/* EN: Forward borrowed Sys frame with its original result/error behavior.
 * RU: Передать заимствованный Sys frame с прежними результатом и ошибками. */
void __real_Sys_werrstr(void *frame);


/* [GC:] EN: Release owned allocation value in borrowed live pool; allocator
 * synchronization and actual physical release remain production operations.
 * RU: Освободить value в заимствованном живом pool; синхронизация аллокатора
 * и физическое освобождение остаются настоящими операциями production. */
void __real_poolfree(Pool *pool, void *value);


/* EN: Run the actual native sleep for duration; return its original status.
 * No managed owner transfers through this scalar argument.
 * RU: Выполнить настоящий native sleep(duration), вернуть исходный статус.
 * Скалярный аргумент не передаёт управляемое владение. */
int __real_dvm_runtime_guest_sleep(unsigned long duration);


/* [GC:] EN: Borrow bound Fast context/user and the budget; return genuine
 * progress/retry. Each production service retains its owners on refusal.
 * RU: Заимствовать привязанный Fast context/user и budget; вернуть настоящую
 * работу/повтор. При отказе production-службы сохраняют владельцев. */
unsigned int __real_dvm_gc_fast_shared(DVM_NativeContext *context, size_t budget, void *user);


/* [GC:] EN: On bound Fast context, attempt budget managed ticket handoffs;
 * return progress/retry without withdrawing roots before the reader cutoff.
 * RU: На привязанном Fast context попробовать budget передач tickets;
 * вернуть работу/повтор без снятия корней до завершения эпохи читателя. */
unsigned int __real_dvm_managed_owner_drain_shared(DVM_NativeContext *context, size_t budget);


/* [GC:] EN: On bound Fast context, attempt budget actual reserve releases;
 * return progress/retry, retaining string/root ownership while readers remain.
 * RU: На привязанном Fast context попробовать budget освобождений reserve;
 * вернуть работу/повтор, сохраняя строку и корень при живых читателях. */
unsigned int __real_dvm_runtime_exception_retire_shared(DVM_NativeContext *context, size_t budget);


/* [GC:] EN: Fail false condition with borrowed message and exact status 12.
 * No ownership recovery; failures cannot stand in for successful progress.
 * RU: При ложном condition вывести message и код 12 без восстановления
 * владения; отказ не заменяет успешную обработку. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL Fast services: %s\n", message);
        exit(12);
    }
}


/* [GC:] EN: Keep two genuine sleep frames admitted; duration is forwarded
 * unchanged after release. Alternate epochs while the other peer stays shared.
 * RU: Держать два настоящих sleep-кадра допущенными; duration передать после
 * разрешения. Чередовать эпохи, пока другой сосед остаётся shared. */
int __wrap_dvm_runtime_guest_sleep(unsigned long duration)
{
    require(!duration && dvm_runtime_gc_dispatch_share(), "real peer shared admission");
    DVM_HeapOwner *owner = dvm_native_owner_current();
    pthread_mutex_lock(&sync);
    unsigned id = peers++;
    require(id < 2, "exactly two permanent guest peers");
    pthread_cond_broadcast(&changed);
    while (!peers_release)
    {
        if (peers == 2 && turn == id)
        {
            require(dvm_native_owner_leave_shared(owner) && dvm_native_owner_enter_shared(owner), "peer epoch");
            turn = 1 - id;
        }
        pthread_mutex_unlock(&sync);
        fvm_port_thread_sleep_us(1000);
        pthread_mutex_lock(&sync);
    }
    pthread_mutex_unlock(&sync);
    dvm_runtime_gc_dispatch_finish();
    return __real_dvm_runtime_guest_sleep(duration);
}


/* [GC:] EN: Borrow actual ticket/reserve payloads until parent observes older
 * RC reclamation. world survives join. End before detach; never borrow target.
 * RU: Заимствовать настоящие данные ticket/reserve до освобождения более
 * старого RC. world живёт до join. Завершить borrow до detach; target не читать. */
static void hold_reader(void *argument)
{
    DVM_NativeWorld *world = argument;
    DVM_NativeContext *context = dvm_native_context_create(sizeof(unsigned long), NULL);
    require(context && dvm_native_context_bind(context), "bind actual reader");
    DVM_WorldParticipant *reader = dvm_world_join_service(world, context);
    require(reader && dvm_world_read_begin(reader), "reader starts after old RC publication");
    pthread_mutex_lock(&sync);
    String *ticket = H2D(String *, ticket_heap);
    String *reserve = H2D(String *, reserve_heap);
    int ticket_length = ticket->len;
    int reserve_length = reserve->len;
    reader_held = 1;
    pthread_cond_broadcast(&changed);
    while (!reader_release)
    {
        pthread_cond_wait(&changed, &sync);
    }
    require(ticket->len == ticket_length && reserve->len == reserve_length, "blocked payloads remain readable");
    require(dvm_world_read_end(reader), "reader ends after final payload access");
    reader_held = 0;
    pthread_mutex_unlock(&sync);
    require(dvm_world_detach(reader) && dvm_native_context_unbind(context) && dvm_native_context_destroy(context),
            "reader detach and context destruction");
}


/* [GC:] EN: Observe actual ticket attempts without replacing queue work.
 * context/budget are forwarded. Count a real reader retry with its owner intact.
 * RU: Наблюдать настоящие попытки tickets без подмены работы. Передать context/
 * budget; учитывать настоящий повтор читателя с сохранённым владельцем. */
unsigned int __wrap_dvm_managed_owner_drain_shared(DVM_NativeContext *context, size_t budget)
{
    unsigned int work = __real_dvm_managed_owner_drain_shared(context, budget);
    pthread_mutex_lock(&sync);
    if (enabled && !finished && reader_held && work)
    {
        require(dvm_managed_owner_pending() == 1, "reader preserves actual ticket FIFO");
        ticket_retries++;
    }
    pthread_mutex_unlock(&sync);
    return work;
}


/* [GC:] EN: Observe actual reserve attempts; forward context/budget unchanged.
 * The reader must still retain the real queued string through each retry.
 * RU: Наблюдать реальные попытки reserve, передавая context/budget без изменений.
 * Читатель должен сохранять настоящую строку очереди при каждом повторе. */
unsigned int __wrap_dvm_runtime_exception_retire_shared(DVM_NativeContext *context, size_t budget)
{
    unsigned int work = __real_dvm_runtime_exception_retire_shared(context, budget);
    pthread_mutex_lock(&sync);
    if (enabled && !finished && reader_held && work)
    {
        require(dvm_runtime_exception_retirement_ready(), "reader preserves actual reserve FIFO");
        reserve_retries++;
    }
    pthread_mutex_unlock(&sync);
    return work;
}


/* [GC:] EN: Gate preparation before any new Fast admission, then forward real
 * production passes with budgets 1/2/3/256. The bound attempt limit diagnoses
 * starvation, never permits a timeout to pass. user/context remain borrowed.
 * RU: Задержать подготовку до нового допуска Fast, затем выполнять настоящие
 * проходы с бюджетами 1/2/3/256. Число попыток выявляет голодание; таймаут не
 * считается успехом. user/context остаются заимствованными. */
unsigned int __wrap_dvm_gc_fast_shared(DVM_NativeContext *context, size_t budget, void *user)
{
    pthread_mutex_lock(&sync);
    while (armed && !enabled)
    {
        pthread_cond_wait(&changed, &sync);
    }
    int probing = enabled && !finished;
    inside = 1;
    pthread_mutex_unlock(&sync);
    size_t selected[] = {1, 2, 3, 256};
    unsigned int work = __real_dvm_gc_fast_shared(context, probing ? selected[CONTROL % 4] : budget, user);
    pthread_mutex_lock(&sync);
    inside = 0;
    if (probing && !finished)
    {
        passes++;
        require(physical_frees || passes < 96, "eligible RC starved behind reader-blocked services");
    }
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    return work;
}


/* [GC:] EN: Forward every actual poolfree. The older RC target must be claimed
 * exactly once while selected FIFO owners and two admitted peers remain live.
 * RU: Передать каждый настоящий poolfree. Более старый RC target захватывается
 * ровно раз при живых владельцах выбранных FIFO и двух допущенных соседях. */
void __wrap_poolfree(Pool *pool, void *value)
{
    pthread_mutex_lock(&sync);
    if (value == target)
    {
        DVM_WorldState state;
        require(reader_held && !reader_release && !peers_release && dvm_world_state(dvm_gc_runtime.world, &state) &&
                    state.active >= 2 && !state.stopped && !state.stopping && dvm_refcount_load_u(&target->ref) == 0,
                "physical RC free overlaps actual blocked services and admitted peers");
        require((dvm_managed_owner_pending() != 0) == WantTickets &&
                    (dvm_runtime_exception_retirement_ready() != 0) == WantReserves,
                "selected services still pending");
        physical_frees++;
        require(physical_frees == 1, "exactly one target physical free");
        target = NULL;
        pthread_cond_broadcast(&changed);
    }
    pthread_mutex_unlock(&sync);
    __real_poolfree(pool, value);
}


/* [GC:] EN: Forward frame, publish an older RC owner, start a later raw reader,
 * then retire selected managed ticket/reserve owners. Wait without admission
 * until RC frees while both newer epochs remain blocked; release/join reader
 * and resume the same native context/registers before allowing guest returns.
 * RU: Передать frame, опубликовать старого RC-владельца, начать поздний borrow,
 * затем передать выбранные настоящие ticket/reserve. Без допуска дождаться RC при
 * заблокированных новых эпохах; завершить/join читателя, вернуть тот же native
 * контекст/регистры и лишь затем разрешить возврат гостей. */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    pthread_mutex_lock(&sync);
    while (peers != 2)
    {
        pthread_cond_wait(&changed, &sync);
    }
    armed = 1;
    while (inside)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    String *old = newstring(17);
    String *managed = newstring(31);
    DVM_OwnerTransfer transfer = {0};
    require(WantTickets ? dvm_managed_owner_capture(managed, &transfer) : dvm_runtime_root_add(D2H(managed)),
            "capture ticket or root independently retained reader payload");
    String *reserve = dvm_runtime_exception_reserve_value();
    require(reserve != NULL, "current real emergency reserve");
    pthread_mutex_lock(&sync);
    target = D2H(old);
    ticket_heap = D2H(managed);
    reserve_heap = D2H(reserve);
    pthread_mutex_unlock(&sync);
    destroy(old);
    if (WantTickets)
    {
        destroy(managed);
    }
    FVM_PortThread *reader = fvm_port_thread_start(hold_reader, dvm_gc_runtime.world);
    require(reader != NULL, "start actual later reader");
    pthread_mutex_lock(&sync);
    while (!reader_held)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    if (WantTickets)
    {
        require(dvm_owner_transfer_cancel(&transfer), "publish real final managed ticket after reader begin");
    }
    if (WantReserves)
    {
        dvm_runtime_exception_reserve_stop();
        dvm_runtime_exception_reserve_start();
    }
    DVM_NativeContext *context = dvm_native_context_current();
    Prog *process = up->prog;
    void *registers = dvm_native_context_registers(context);
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "publish parent roots while waiting");
    pthread_mutex_lock(&sync);
    enabled = 1;
    pthread_cond_broadcast(&changed);
    while (!physical_frees || (WantTickets && !ticket_retries) || (WantReserves && !reserve_retries))
    {
        pthread_cond_wait(&changed, &sync);
    }
    require(reader_held && dvm_runtime_root_count(ticket_heap) == 1 && dvm_runtime_root_count(reserve_heap) == 1 &&
                dvm_refcount_load_u(&ticket_heap->ref) == 1 && dvm_refcount_load_u(&reserve_heap->ref) == 1,
            "blocked services preserve roots and strong owners");
    finished = 1;
    reader_release = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    fvm_port_thread_join(reader);
    dvm_runtime_gc_native_enter(native);
    require(context == dvm_native_context_current() && process == up->prog &&
                registers == dvm_native_context_registers(context),
            "stable resident and live registers");
    if (!WantTickets)
    {
        require(dvm_runtime_root_remove(ticket_heap), "release independently owned reader payload root");
        destroy(managed);
    }
    pthread_mutex_lock(&sync);
    peers_release = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    fprintf(stderr,
            "PASS Fast services: budget=%u RC=1 ticket_retry=%u reserve_retry=%u peers=2 registers=stable\n",
            (unsigned int[]){1, 2, 3, 256}[CONTROL % 4],
            (unsigned int)WantTickets,
            (unsigned int)WantReserves);
}
