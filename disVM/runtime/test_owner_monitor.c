/* Copyright (c) 2026 bhgv
 * [GC:] EN: Prove owner-local callbacks and Fast progress while hooks are installed.
 * RU: Доказать локальность callback и прогресс Fast при установленных hooks.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runtime_gc.h"
#include "disvm/native_context.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define Workers 8 /* Actual permanent residents, not executor slots. */


/* EN: Borrowed static code pointer; no managed reference or callback migration.
 * RU: Заимствованный указатель статического кода, без managed-ссылок и миграции. */
typedef void (*Monitor)(int, void *, ulong);

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;  /* Fixture metadata only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Phase/free observations. */
static unsigned arrivals[4];                              /* Phase arrivals, under sync. */
static unsigned assigned;                                 /* Unique owner indexes, under sync. */
static unsigned physical_frees;                           /* Watched String headers actually freed, under sync. */
static Heap *watched[Workers];                            /* Borrowed identities through their actual poolfree. */
static Monitor shared_selector;                      /* Atomic deliberately wrong selector in the omission control. */
static __thread DVM_NativeContext *expected_context; /* This thread's permanent context. */
static __thread Prog *expected_process;              /* This thread's permanent guest. */
static __thread int owner_index;                     /* -1 for controller; otherwise one resident index. */
static __thread unsigned events;                     /* Genuine allocation callbacks on this owner. */
static __thread int parent_started;                  /* Only the controller receives two fixture calls. */


/* EN: Linker's genuine Sys entry borrows frame through its synchronous call.
 * RU: Настоящий Sys-вход linker заимствует frame на время синхронного вызова. */
void __real_Sys_werrstr(void *frame);


/* [GC:] EN: Genuine allocator consumes value in borrowed pool; no retained alias.
 * RU: Настоящий allocator снимает value в заимствованном pool без живого alias. */
void __real_poolfree(Pool *pool, void *value);


/* EN: Fail condition with borrowed message; terminate all fixture peers with 12.
 * RU: При ложном condition завершить всех партнёров кодом 12 с message. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL owner monitor: %s\n", message);
        fflush(stderr);
        _Exit(12);
    }
}


/* EN: Produce a bounded native deadline; no heap access or transferred owner.
 * RU: Получить ограниченный native deadline, без доступа к куче и передачи владельца. */
static struct timespec deadline(void)
{
    struct timespec limit;
    require(!clock_gettime(CLOCK_REALTIME, &limit), "native clock");
    limit.tv_sec += 30;
    return limit;
}


/* [GC:] EN: Meet all real residents in phase with roots published for native wait.
 * Hold only fixture metadata; resume the same context/REG. Timeout fails.
 * RU: Совместить настоящие потоки в phase с публикацией корней для native wait.
 * Удерживать лишь тестовые данные; вернуть тот же контекст/REG. Таймаут — отказ. */
static void barrier(unsigned phase)
{
    require(phase < 4, "phase range");
    int released = dvm_runtime_gc_native_try_leave();
    require(released > 0, "phase publishes native roots");
    struct timespec limit = deadline();
    pthread_mutex_lock(&sync);
    arrivals[phase]++;
    pthread_cond_broadcast(&changed);
    while (arrivals[phase] != Workers)
    {
        int result = pthread_cond_timedwait(&changed, &sync, &limit);
        require(result == 0 || (result == ETIMEDOUT && arrivals[phase] == Workers), "phase timeout");
    }
    pthread_mutex_unlock(&sync);
    dvm_runtime_gc_native_enter(released);
}


/* [GC:] EN: Keep every installed hook while Fast physically frees all first
 * allocations. Release only this native admission; no global stop or retry pass.
 * RU: Сохранить все hooks до физического освобождения первых объектов Fast.
 * Снять только свой native-допуск, без общей остановки и повторного прогона. */
static void await_frees(void)
{
    int released = dvm_runtime_gc_native_try_leave();
    require(released > 0, "free wait publishes native roots");
    struct timespec limit = deadline();
    pthread_mutex_lock(&sync);
    while (physical_frees != Workers)
    {
        int result = pthread_cond_timedwait(&changed, &sync, &limit);
        require(result == 0 || (result == ETIMEDOUT && physical_frees == Workers), "Fast progress timeout");
    }
    pthread_mutex_unlock(&sync);
    dvm_runtime_gc_native_enter(released);
}


/* EN: Validate event/value/bytes on this callback's original native owner.
 * tag distinguishes independent callback identities; payload is never retained.
 * RU: Проверить event/value/bytes на исходном native-владельце callback.
 * tag различает callbacks; payload не удерживается. */
static void observe(int tag, int event, void *value, ulong bytes)
{
    require(expected_context && dvm_native_context_current() == expected_context &&
                dvm_native_context_process(expected_context) == expected_process,
            "callback owner changed");
    require(tag == (owner_index < 0 ? 2 : owner_index % 2), "foreign callback selected");
    require(value != NULL, "callback has a real allocation");
    if (event == 0)
    {
        require(bytes > 0, "allocation extent");
        events++;
    }
}


/* EN/RU: Forward a controller event/value/bytes without owner transfer.
 * Передать event/value/bytes контроллера без передачи владельца. */
static void parent_monitor(int event, void *value, ulong bytes)
{
    observe(2, event, value, bytes);
}


/* EN/RU: Forward an even resident's event/value/bytes on its own thread.
 * Передать event/value/bytes чётного потока на его владельце. */
static void even_monitor(int event, void *value, ulong bytes)
{
    observe(0, event, value, bytes);
}


/* EN/RU: Forward an odd resident's event/value/bytes on its own thread.
 * Передать event/value/bytes нечётного потока на его владельце. */
static void odd_monitor(int event, void *value, ulong bytes)
{
    observe(1, event, value, bytes);
}


/* [GC:] EN: Allocate/destroy one real String on this owner. track retains only
 * the header identity for poolfree reconciliation, never a stale dereference.
 * RU: Выделить/снять настоящий String своим владельцем. track сохраняет лишь
 * адрес header для сверки poolfree, без чтения устаревшего объекта. */
static void allocate(int track)
{
    String *value = c2string("owner-monitor", 13);
    require(value && value != H && value->len == 13, "real String construction");
    if (track)
    {
        pthread_mutex_lock(&sync);
        watched[owner_index] = D2H(value);
        pthread_mutex_unlock(&sync);
    }
    destroy(value);
}


/* [GC:] EN: Observe a borrowed pool/value before the genuine free. A tracked
 * header must be reclaimed on a collector with no inherited observer hook.
 * RU: Наблюдать pool/value до настоящего free. Отслеживаемый header должен
 * освобождаться сборщиком, не унаследовавшим hook владельца. */
void __wrap_poolfree(Pool *pool, void *value)
{
    int tracked = 0;
    pthread_mutex_lock(&sync);
    for (unsigned index = 0; index < Workers; index++)
    {
        if (value && value == watched[index])
        {
            DVM_NativeContext *context = dvm_native_context_current();
            require(context && !dvm_native_context_process(context) && !heapmonitor,
                    "collector has independent empty observer");
            watched[index] = NULL;
            tracked = 1;
            break;
        }
    }
    pthread_mutex_unlock(&sync);
    __real_poolfree(pool, value);
    if (tracked)
    {
        pthread_mutex_lock(&sync);
        physical_frees++;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&sync);
    }
}


/* EN: Forward borrowed frame unchanged after real allocation/owner checks.
 * Each resident installs, clears and restores only its callback; the parent
 * keeps its distinct callback until every resident and watched free completed.
 * CONTROL=1 atomically shares a selector to prove foreign-callback detection.
 * RU: Передать frame без изменений после настоящих проверок выделения/владельца.
 * Каждый поток меняет лишь свой callback; родитель сохраняет отдельный hook
 * до завершения детей и освобождений. CONTROL=1 проверяет обнаружение чужого hook. */
void __wrap_Sys_werrstr(void *frame)
{
    DVM_NativeContext *context = dvm_native_context_current();
    Prog *process = dvm_native_context_process(context);
    require(process != NULL, "bound guest");
    if (process->pid == 1)
    {
        if (!parent_started)
        {
            require(!heapmonitor, "initial controller hook is empty");
            expected_context = context;
            expected_process = process;
            owner_index = -1;
            heapmonitor = parent_monitor;
            parent_started = 1;
        }
        else
        {
            require(heapmonitor == parent_monitor && events > 0, "controller hook survives residents");
            pthread_mutex_lock(&sync);
            require(assigned == Workers && physical_frees == Workers, "exact watched frees");
            pthread_mutex_unlock(&sync);
            heapmonitor = NULL;
            fprintf(stderr, "OWNER_MONITOR parent=1 residents=8 physical_frees=8 isolated=1\n");
        }
    }
    else
    {
        require(!heapmonitor, "new resident inherited a monitor");
        expected_context = context;
        expected_process = process;
        pthread_mutex_lock(&sync);
        owner_index = assigned++;
        pthread_mutex_unlock(&sync);
        require(owner_index < Workers, "resident count");
        Monitor local = owner_index % 2 ? odd_monitor : even_monitor;
        heapmonitor = local;
        if (CONTROL == 1)
        {
            __atomic_store_n(&shared_selector, local, __ATOMIC_RELEASE);
        }
        barrier(0);
        if (CONTROL == 1)
        {
            heapmonitor = __atomic_load_n(&shared_selector, __ATOMIC_ACQUIRE);
        }
        unsigned before = events;
        allocate(1);
        require(events > before, "installed callback executes");
        barrier(1);
        await_frees();
        heapmonitor = owner_index % 2 ? local : NULL;
        barrier(2);
        before = events;
        allocate(0);
        require(owner_index % 2 ? events > before : events == before, "clearing affects only this owner");
        barrier(3);
        heapmonitor = local;
        before = events;
        allocate(0);
        require(events > before, "restored callback executes");
        heapmonitor = NULL;
    }
    __real_Sys_werrstr(frame);
}
