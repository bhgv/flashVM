/* Copyright (c) 2026 bhgv
 * [GC:] EN: Probe forbidden fallback entries on actual native DisVM processes.
 * RU: Проверить запрещённые входы fallback на настоящих нативных процессах DisVM.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runtime_rc.h"
#include "runtime_gc.h"
#include "root_registry.h"
#include "gc/collector_passes.h"
#include "disvm/native_owner.h"
#include "disvm/native_process.h"
#include "disvm/refcount.h"
#include "thread_port.h"
#include <pthread.h>
#include "gc/common/runtime_state.h"
#include <stdio.h>
#include <stdlib.h>


#define Case                                                                                                           \
    (CONTROL == 104   ? 10                                                                                             \
     : CONTROL == 105 ? 7                                                                                              \
     : CONTROL >= 100 ? CONTROL - 95                                                                                   \
                      : CONTROL) /* Map six real omission controls to their native admission cases. */

static Heap *protected_heap;     /* Registry-rooted source owned through the attempted release. */
static Type *protected_type;     /* Creator keeps the descriptor alive through observations. */
static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER; /* Fixture observations only. */
static unsigned peers;                                   /* Real shared peer frames, under sync. */
static unsigned turn;                                    /* Peer that may advance its epoch next, under sync. */
static int release_peers;                                /* Parent completed physical free, under sync. */
static _Thread_local int forbid_drain_requests;          /* Only this thread probes invalid drain admission. */
static int physically_freed;                             /* Actual poolfree of the positive source, under sync. */
extern Pool *heapmem;                                    /* Real managed arena through all observations. */


void __real_Sys_werrstr(void *);


void __real__Exit(int);


void __real_poolfree(Pool *, void *);


int __real_dvm_runtime_guest_sleep(unsigned long);


/* EN: Fail false condition with borrowed message; no recovery or owner transfer.
 * RU: При ложном condition завершиться с заимствованным message без восстановления. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL strict retirement: %s\n", message);
        exit(12);
    }
}


/* [GC:] EN: Observe production fatal status before forwarding the real _Exit.
 * The independently rooted source must keep ref/root/type counts unchanged.
 * RU: Проверить статус настоящей защиты до передачи в _Exit. Независимо
 * укоренённый источник должен сохранить счётчик ссылки, корня и типа. */
void __wrap__Exit(int status)
{
    if (status == 70 && (Case == 0 || (Case >= 5 && Case <= 8)))
    {
        pthread_mutex_lock(&sync);
        Heap *heap = protected_heap;
        Type *type = protected_type;
        pthread_mutex_unlock(&sync);
        if (!heap || !type || dvm_refcount_load_u(&heap->ref) != 1 || dvm_runtime_root_count(heap) != 1 ||
            dvm_refcount_load_i(&type->ref) != 2)
        {
            fputs("FAIL strict retirement: rejection changed protected ownership\n", stderr);
            __real__Exit(12);
        }
        fputs("GUARD_OWNERS_PRESERVED ref=1 roots=1 type=2\n", stderr);
        fflush(stderr);
    }
    __real__Exit(status);
    abort();
}


/* [GC:] EN: Hold two actual guest sleep frames shared in the positive case.
 * duration is forwarded after release; alternate epochs without replacing Prog.
 * RU: В положительном случае держать два настоящих sleep-кадра shared.
 * duration передать после разрешения; чередовать эпохи без замены Prog. */
int __wrap_dvm_runtime_guest_sleep(unsigned long duration)
{
    if (Case != 9)
    {
        return __real_dvm_runtime_guest_sleep(duration);
    }
    require(!duration && dvm_runtime_gc_dispatch_share(), "positive peer admission");
    DVM_HeapOwner *owner = dvm_native_owner_current();
    pthread_mutex_lock(&sync);
    unsigned id = peers++;
    require(id < 2, "two permanent peers");
    while (!release_peers)
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


/* [GC:] EN: Observe the positive source's actual final free under shared
 * admission with real peers. pool/value remain borrowed through real poolfree.
 * RU: Проверить настоящее финальное free положительного источника под shared
 * при живых соседях. pool/value заимствованы до настоящего poolfree. */
void __wrap_poolfree(Pool *pool, void *value)
{
    if (Case == 9)
    {
        pthread_mutex_lock(&sync);
        if (value == protected_heap)
        {
            DVM_WorldState state;
            require(!physically_freed && !release_peers && dvm_world_state(dvm_gc_runtime.world, &state) &&
                        state.active >= 2 && !state.stopped && !dvm_runtime_root_count(value) &&
                        !dvm_refcount_load_u(&((Heap *)value)->ref),
                    "physical shared finalization preserves roots/counts");
            physically_freed = 1;
        }
        pthread_mutex_unlock(&sync);
    }
    __real_poolfree(pool, value);
}


int __real_dvm_gc_workers_request(DVM_GCWorkers *, DVM_GCKind);


int __real_dvm_runtime_gc_stop(int);


/* [GC:] EN: Forward requests outside a rejection probe. Borrow workers/kind;
 * an invalid drain must reject before requesting any collector work.
 * RU: Передать запрос вне проверки отказа. workers/kind заимствованы; неверный
 * drain должен отказать до любого запроса работы сборщика. */
int __wrap_dvm_gc_workers_request(DVM_GCWorkers *workers, DVM_GCKind kind)
{
    require(!forbid_drain_requests, "invalid drain must not queue collector work");
    return __real_dvm_gc_workers_request(workers, kind);
}


/* [GC:] EN: Check actual late controller SHARED rejection after guest joins.
 * diagnostics is forwarded to real shutdown after restoring its original phase.
 * No requests, root mutation or live register reload may occur in the refusal.
 * RU: После join гостей проверить отказ настоящего SHARED-контроллера.
 * diagnostics передать настоящей остановке после возврата исходной фазы.
 * При отказе запрещены запросы, изменение корней и восстановление регистров. */
int __wrap_dvm_runtime_gc_stop(int diagnostics)
{
    if (Case == 10)
    {
        DVM_HeapOwner *owner = dvm_native_owner_current();
        DVM_HeapOwnerStats before;
        require(dvm_native_owner_statistics(owner, &before), "late controller statistics");
        int active = before.phase == DVM_HEAP_ACTIVE;
        if (active)
        {
            require(dvm_native_owner_leave(owner), "late controller leaves ACTIVE");
        }
        require(dvm_native_owner_try_enter_idle_shared(owner), "late controller SHARED admission");
        DVM_NativeContext *context = dvm_native_context_current();
        forbid_drain_requests = 1;
        require(!dvm_runtime_gc_concurrent_drain(), "SHARED drain refuses before waiting on its own epoch");
        forbid_drain_requests = 0;
        DVM_HeapOwnerStats after;
        require(context == dvm_native_context_current() && dvm_native_owner_statistics(owner, &after) &&
                    after.phase == DVM_HEAP_SHARED && !after.pending_progress,
                "refused drain preserves controller phase");
        require(dvm_native_owner_leave_idle_shared(owner) && (!active || dvm_native_owner_enter(owner)),
                "restore late controller phase");
        fputs("PASS strict drain rejection shared=1 requests=0 phase=preserved\n", stderr);
    }
    return __real_dvm_runtime_gc_stop(diagnostics);
}


/* [GC:] EN: Borrow source from the controller's real root. Bind independent
 * native TLS, optionally enter a foreign world, then exercise real destroy.
 * Neither role owns this runtime's admission; the production guard must exit 70.
 * RU: Заимствовать источник из настоящего корня контроллера. Привязать своё
 * native TLS, при необходимости войти в чужой мир и вызвать настоящий destroy.
 * Допуска к runtime нет; настоящая защита обязана дать код 70. */
static void foreign_release(void *argument)
{
    DVM_NativeContext *context = dvm_native_process_create(NULL);
    require(context && dvm_native_context_bind(context), "foreign context binds");
    dvm_gc_collector_environment();
    if (Case == 8)
    {
        DVM_NativeWorld *other = dvm_world_create_concurrent(0);
        DVM_HeapOwner *owner = dvm_native_owner_current();
        require(other && dvm_native_owner_attach(owner, other) && dvm_native_owner_enter(owner),
                "foreign ACTIVE world");
    }
    forbid_drain_requests = 1;
    require(!dvm_runtime_gc_concurrent_drain(), "foreign drain rejected before collector requests");
    forbid_drain_requests = 0;
    destroy(argument);
    require(0, "foreign destroy unexpectedly returned");
}


/* [GC:] EN: This unknown callback must never run. heap/swept remain owned by
 * the real retirement path; fail explicitly rather than free them unsafely.
 * RU: Неизвестный callback не должен исполняться. heap/swept принадлежат
 * настоящему освобождению; явный отказ вместо небезопасного free. */
static void unknown(Heap *heap, int swept)
{
    (void)heap;
    (void)swept;
    fputs("FAIL strict guard: unknown callback executed\n", stderr);
    exit(12);
}


/* [GC:] EN: Forward borrowed frame and probe one real forbidden API according
 * to CONTROL. Its production guard must exit 70 before any legacy heap access.
 * RU: Выполнить заимствованный frame и проверить настоящий запрещённый API
 * по CONTROL. Его защита должна дать код 70 до прежнего доступа к куче. */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    if (Case == 10)
    {
        return;
    }
    if (Case == 9)
    {
        for (;;)
        {
            pthread_mutex_lock(&sync);
            int ready = peers == 2;
            pthread_mutex_unlock(&sync);
            if (ready)
            {
                break;
            }
            fvm_port_thread_sleep_us(1000);
        }
    }
    if (Case == 0)
    {
        Type *type = dtype(unknown, sizeof(void *), NULL, 0);
        Heap *heap = heapz(type);
        protected_type = type;
        protected_heap = heap;
        if (!dvm_runtime_rc_defer(H2D(void *, heap)))
        {
            exit(12);
        }
        for (;;)
        {
            dvm_native_context_poll();
        }
    }
    else if (Case == 1)
    {
        rungc(up->prog);
    }
    else if (Case == 2)
    {
        dvm_runtime_rc_drain(1);
    }
    else if (Case == 3)
    {
        dvm_world_stop_begin(dvm_gc_runtime.world, dvm_native_context_current());
    }
    else if (Case == 4)
    {
        dvm_world_try_quiescent(dvm_gc_runtime.world, dvm_native_context_current());
    }
    else
    {
        Type *type = dtype(Case == 9 ? freeheap : unknown, sizeof(void *), NULL, 0);
        require(type != NULL, "protected descriptor");
        Heap *heap = heapz(type);
        pthread_mutex_lock(&sync);
        protected_type = type;
        protected_heap = heap;
        pthread_mutex_unlock(&sync);
        require(protected_heap && dvm_runtime_root_add(protected_heap), "independent real source root");
        if (Case == 7 || Case == 8)
        {
            FVM_PortThread *thread = fvm_port_thread_start(foreign_release, H2D(void *, protected_heap));
            require(thread != NULL, "foreign release thread");
            fvm_port_thread_join(thread);
        }
        else
        {
            DVM_NativeContext *context = dvm_native_context_current();
            Prog *guest = dvm_native_context_process(context);
            Inst *pc = DVM_R.PC;
            uchar *fp = DVM_R.FP;
            int native = dvm_runtime_gc_native_try_leave();
            require(native > 0, "source remains rooted across native release");
            if (Case == 6)
            {
                require(dvm_gc_workers_wait_idle(dvm_gc_runtime.workers), "quiescent source setup without exclusion");
                dvm_runtime_gc_native_enter(native);
                dvm_runtime_rc_close();
            }
            else if (Case == 9)
            {
                dvm_runtime_gc_native_enter(native);
                require(context == dvm_native_context_current() && guest == up->prog && DVM_R.PC == pc &&
                            DVM_R.FP == fp,
                        "native return preserves original live registers and Prog");
            }
            destroy(H2D(void *, protected_heap));
            if (Case == 9)
            {
                require(dvm_runtime_root_remove(protected_heap), "withdraw source root after RC publication");
                native = dvm_runtime_gc_native_try_leave();
                require(native > 0 && dvm_gc_workers_wait_idle(dvm_gc_runtime.workers),
                        "shared ownership handoff drains");
                dvm_runtime_gc_native_enter(native);
                pthread_mutex_lock(&sync);
                require(physically_freed && peers == 2, "positive source frees beside actual peers");
                release_peers = 1;
                pthread_mutex_unlock(&sync);
                require(dvm_refcount_load_i(&protected_type->ref) == 1, "descriptor creator retains sole owner");
                freetype(protected_type);
                fputs("PASS strict release roundtrip rooted=1 freed=1 peers=2 registers=stable\n", stderr);
                return;
            }
        }
    }
    fputs("FAIL strict guard: forbidden operation returned\n", stderr);
    exit(12);
}
