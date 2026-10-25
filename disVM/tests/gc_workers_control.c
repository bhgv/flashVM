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
    dvm_host_sync_destroy(backend.sync);
    return 0;
}
