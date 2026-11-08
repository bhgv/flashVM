/* Exercise native GC worker fairness, startup failure and callback failure cleanup. */
#include <stdio.h>
#include <stdlib.h>
#include "disvm/gc_workers.h"
#include "disvm/flashvm_host.h"
#include <string.h>

#ifdef NDEBUG
#undef NDEBUG
#endif


/* Callbacks run under one heap lease; factory counters have their own native lock. */
typedef struct Backend
{
    FVM_PortSync *sync;
    DVM_GCWorkers *workers;
    unsigned int factories;
    int fail_factory;
    int fail_pass;
    unsigned int Fast;
    unsigned int Cycles;
    unsigned int trace[16];
    unsigned int count;
    DVM_NativeWorld *shared_world; /* Borrowed through all shared callbacks. */
    unsigned int entered;          /* Shared callbacks waiting at the test barrier. */
    int release;                   /* Main observed three simultaneous heap owners. */
} Backend;


/* Create distinct collector contexts without interpreter-specific SDK headers. */
static DVM_NativeContext *create_context(void *user)
{
    Backend *backend = user;
    int failed;
    fvm_port_sync_lock(backend->sync);
    backend->factories++;
    failed = backend->fail_factory && backend->factories == 2;
    fvm_port_sync_unlock(backend->sync);
    return failed ? 0 : dvm_native_context_create(sizeof(unsigned long), user);
}


/* This host-control fixture intentionally has no managed mutator roots. */
static void roots(DVM_NativeContext *context, void *registers, size_t size, void *process, void *user)
{
    (void)context;
    (void)registers;
    (void)size;
    (void)process;
    (void)user;
    if (!(0 && "unexpected mutator in host control fixture"))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* Queue the other lane inside the first pass to make pending-work fairness deterministic. */
static DVM_GCWorkResult fast(DVM_NativeContext *context, size_t budget, void *user)
{
    Backend *backend = user;
    if (!(context == dvm_native_context_current() && budget == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!dvm_gc_workers_wait_idle(backend->workers)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!dvm_gc_workers_destroy(backend->workers)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (backend->fail_pass)
    {
        return DVM_GC_FAILED;
    }
    if (!(backend->count < 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    backend->trace[backend->count++] = DVM_GC_FAST;
    backend->Fast++;
    if (backend->Fast == 1)
    {
        if (!(dvm_gc_workers_request(backend->workers, DVM_GC_CYCLES)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return backend->Fast < 4 ? DVM_GC_MORE : DVM_GC_DONE;
}


/* A bounded synthetic callback checks scheduling only, not heap algorithms. */
static DVM_GCWorkResult cycles(DVM_NativeContext *context, size_t budget, void *user)
{
    Backend *backend = user;
    if (!(context == dvm_native_context_current() && budget == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(backend->count < 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    backend->trace[backend->count++] = DVM_GC_CYCLES;
    backend->Cycles++;
    return backend->Cycles < 4 ? DVM_GC_MORE : DVM_GC_DONE;
}


/* Hold a real shared world turn until main observes all three owners.
 * Context and backend are borrowed; this callback needs no later pass. */
static unsigned int overlap_shared(DVM_NativeContext *context, size_t budget, void *user)
{
    Backend *backend = user;
    DVM_WorldParticipant *participant = dvm_world_join_service(backend->shared_world, context);
    if (!budget || !participant || !dvm_world_try_enter_shared(participant))
    {
        fputs("FAIL shared GC admission\n", stderr);
        exit(EXIT_FAILURE);
    }
    fvm_port_sync_lock(backend->sync);
    backend->entered++;
    fvm_port_sync_signal_all(backend->sync);
    while (!backend->release)
    {
        fvm_port_sync_wait(backend->sync);
    }
    fvm_port_sync_unlock(backend->sync);
    if (!dvm_world_leave(participant) || !dvm_world_detach(participant))
    {
        fputs("FAIL shared GC detach\n", stderr);
        exit(EXIT_FAILURE);
    }
    return 0;
}


/* The Fast test has no retained work after its shared callback. */
static int no_retained_work(void *user)
{
    (void)user;
    return 0;
}


/* Exercise all three worker callbacks simultaneously under shared admission. */
static void parallel_case(Backend *backend, DVM_GCWorkersConfig config)
{
    backend->shared_world = dvm_world_create_serial(0);
    config.world = backend->shared_world;
    config.parallel_shared = 1;
    config.fast_shared = overlap_shared;
    config.fast_ready = no_retained_work;
    config.cycles_shared = overlap_shared;
    config.duke_shared = overlap_shared;
    config.duke_budget = 1;
    backend->workers = dvm_gc_workers_create(&config);
    if (!backend->workers || !dvm_gc_workers_request(backend->workers, DVM_GC_FAST) ||
        !dvm_gc_workers_request_candidates(backend->workers) ||
        !dvm_gc_workers_request(backend->workers, DVM_GC_DUKE))
    {
        fputs("FAIL start shared GC lanes\n", stderr);
        exit(EXIT_FAILURE);
    }
    fvm_port_sync_lock(backend->sync);
    while (backend->entered != 3)
    {
        fvm_port_sync_wait(backend->sync);
    }
    DVM_WorldState world;
    DVM_GCWorkersState lanes;
    if (!dvm_world_state(config.world, &world) || world.active != 3 ||
        dvm_world_has_active_guest(config.world) ||
        !dvm_gc_workers_state(backend->workers, &lanes) || lanes.shared_peak != 3)
    {
        fputs("FAIL three simultaneous GC heap owners\n", stderr);
        exit(EXIT_FAILURE);
    }
    backend->release = 1;
    fvm_port_sync_signal_all(backend->sync);
    fvm_port_sync_unlock(backend->sync);
    if (!dvm_gc_workers_wait_idle(backend->workers) || !dvm_gc_workers_destroy(backend->workers) ||
        !dvm_world_destroy(config.world))
    {
        fputs("FAIL shared GC shutdown\n", stderr);
        exit(EXIT_FAILURE);
    }
}


/* Start isolated service instances and require all workers to join on every path. */
int main(void)
{
    Backend backend;
    DVM_GCWorkersConfig config;
    DVM_GCWorkersState state;
    DVM_WorldState world_state;
    DVM_NativeWorld *world = dvm_world_create();
    unsigned int index;
    memset(&backend, 0, sizeof(backend));
    memset(&config, 0, sizeof(config));
    backend.sync = dvm_host_sync_create();
    if (!(world && backend.sync))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    config.world = world;
    config.create_context = create_context;
    config.roots = roots;
    config.Fast.run = fast;
    config.Fast.budget = 1;
    config.Cycles.run = cycles;
    config.Cycles.budget = 1;
    config.Cycles.pause_us = 1000;
    config.user = &backend;
    config.Fast.pause_us = DVM_GC_MAX_PAUSE_US + 1;
    if (!(!dvm_gc_workers_create(&config)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    config.Fast.pause_us = 0;
    backend.workers = dvm_gc_workers_create(&config);
    if (!(backend.workers))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_gc_workers_request(backend.workers, DVM_GC_FAST)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_gc_workers_wait_idle(backend.workers)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(backend.count == 8 && backend.Fast == 4 && backend.Cycles == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (index = 0; index < 8; index++)
    {
        if (!(backend.trace[index] == (index % 2 ? DVM_GC_CYCLES : DVM_GC_FAST)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(dvm_gc_workers_state(backend.workers, &state)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!state.failed && !state.active && state.Fast.passes == 4 && state.Cycles.passes == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_gc_workers_destroy(backend.workers)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    backend.factories = 0;
    backend.fail_factory = 1;
    if (!(!dvm_gc_workers_create(&config)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    backend.fail_factory = 0;
    backend.fail_pass = 1;
    backend.workers = dvm_gc_workers_create(&config);
    if (!(backend.workers && dvm_gc_workers_request(backend.workers, DVM_GC_FAST)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!dvm_gc_workers_wait_idle(backend.workers)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_gc_workers_state(backend.workers, &state) && state.failed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!dvm_gc_workers_request(backend.workers, DVM_GC_CYCLES)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!dvm_gc_workers_destroy(backend.workers)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_world_state(world, &world_state) && !world_state.stopping && !world_state.stopped))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_world_destroy(world)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    parallel_case(&backend, config);
    dvm_host_sync_destroy(backend.sync);
    return 0;
}
