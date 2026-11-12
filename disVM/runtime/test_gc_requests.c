/* Copyright (c) 2026 bhgv
 * EN: Real parallel GC requests with rooted native frames and exact empty-skip counts.
 * RU: Настоящие параллельные запросы GC с корнями native-кадров и точным счётчиком.
 */
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "runtime_gc.h"
#include "gc/common/runtime_state.h"
#include "managed_owner.h"
#include "channel_sync.h"
#include "disvm/native_context.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;  /* Fixture predicates, not VM state. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Native frame/drain/count milestones. */
static unsigned outside;                 /* Owners with actual published roots and released admission. */
static unsigned finished;                /* Owners completed all real requests, under sync. */
static int drained;                      /* Genuine Fast/collector service reached idle. */
static int sampled;                      /* Final counter captured before any callback returns. */
static unsigned long before;             /* Stable pre-request count, after real GC drain. */
static unsigned long before_cycle_skips; /* Event skips after actual quiescent color rotations. */
static unsigned expected_cycle_skips;    /* Remaining clean-request allowance before fallback. */
static unsigned long observed;           /* Actual empty-skip delta at the final milestone. */


void __real_Sys_werrstr(void *);


/* EN: Require condition or fail with borrowed message before unsafe VM access.
 * RU: Проверить condition либо завершить с message до опасного доступа к VM. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL GC request counter: %s\n", message);
        exit(12);
    }
}


/* EN: Forward frame once, publish its real roots while draining native owners,
 * then issue 4000 genuine requests on each of eight original OS residents.
 * The fixture uses quiescent-only collectors; no register or Prog replacement.
 * RU: Выполнить frame, опубликовать настоящие корни при дренировании владельцев,
 * затем сделать по 4000 настоящих запросов из восьми исходных потоков ОС.
 * Сборщики теста работают при quiescence; регистры/Prog не заменяются. */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    if (!process || process->pid <= 1)
    {
        return;
    }
    require(dvm_runtime_idle_cycles || dvm_runtime_disable_cycles, "fixture barriers require quiescent-only tracing");
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "real native frame roots are published");
    pthread_mutex_lock(&sync);
    outside++;
    if (outside == 8)
    {
        pthread_mutex_unlock(&sync);
        require(dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_FAST) &&
                    (!dvm_runtime_event_cycles || dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_CYCLES)) &&
                    dvm_gc_workers_wait_idle(dvm_gc_runtime.workers),
                "genuine GC service drains while all frames stay rooted");
        require(!dvm_managed_owner_pending(), "managed native owner queue is genuinely empty");
        pthread_mutex_lock(&sync);
        if (dvm_runtime_event_cycles)
        {
            fvm_port_registry_lock();
            extern ulong gcepochs;
            if (dvm_world_stops_forbidden(dvm_gc_runtime.world))
            {
                require(!dvm_gc_runtime.passes && !dvm_gc_runtime.snapshots, "strict GC never traces or parks");
                before_cycle_skips = dvm_gc_runtime.cycle_skips;
                expected_cycle_skips = 32000;
            }
            else
            {
                require(gcepochs - dvm_gc_runtime.cycle_epoch >= 3 && dvm_gc_runtime.clean_requests < 256,
                        "real idle tracing completes color rotations before event burst");
                before_cycle_skips = dvm_gc_runtime.cycle_skips;
                expected_cycle_skips = 255 - dvm_gc_runtime.clean_requests;
            }
            fvm_port_registry_unlock();
        }
        before = __atomic_load_n(&dvm_gc_runtime.fast_empty_skips, __ATOMIC_RELAXED);
        drained = 1;
        pthread_cond_broadcast(&changed);
    }
    while (!drained)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    dvm_runtime_gc_native_enter(native);
    unsigned request;
    for (request = 0; request < 4000; request++)
    {
        require(dvm_runtime_gc_request(), "real current-owner GC request succeeds");
        DVM_WorldState world;
        require(dvm_world_state(dvm_gc_runtime.world, &world) && !world.stopping && !world.stopped && world.active,
                "an admitted dedicated owner never receives a tracing park request");
    }
    pthread_mutex_lock(&sync);
    finished++;
    if (finished == 8)
    {
        observed = __atomic_load_n(&dvm_gc_runtime.fast_empty_skips, __ATOMIC_RELAXED) - before;
        fprintf(stderr, "GC_REQUEST_COUNTER observed=%lu expected=32000\n", observed);
        require(observed == 32000, "empty Fast request statistics lose no concurrent increments");
        if (dvm_runtime_event_cycles)
        {
            fvm_port_registry_lock();
            require(dvm_gc_runtime.cycle_skips - before_cycle_skips == expected_cycle_skips,
                    "real clean event requests coalesce until the bounded fallback request");
            fprintf(stderr,
                    "GC_EVENT_COUNTER observed=%lu expected=%u\n",
                    dvm_gc_runtime.cycle_skips - before_cycle_skips,
                    expected_cycle_skips);
            fvm_port_registry_unlock();
        }
        sampled = 1;
        pthread_cond_broadcast(&changed);
    }
    while (!sampled)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    if (process->pid == 2)
    {
        fprintf(stderr, "PASS real GC requests owners=8 requests=%lu rooted_drain=1\n", observed);
    }
}
