/* Real instructions on two native up objects while a third thread scans stable roots. */
#include <stdio.h>
#include <stdlib.h>
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "disvm/native_process.h"
#include "disvm/native_world.h"

#ifdef DVM_GC_WORKERS
#include "disvm/gc_workers.h"

#endif
#include "disvm/flashvm_host.h"

#ifdef NDEBUG
#undef NDEBUG
#endif


/* Each worker owns its mutable fields; collector touches them only while stopped. */
typedef struct Worker
{
    DVM_NativeWorld *world;
    DVM_NativeContext *context;
    unsigned int identity;
    unsigned long iterations;
    int stop;
} Worker;


/* One collector scan counts exact root sets and optionally requests final exit. */
typedef struct Scan
{
    unsigned int count;
    int finish;
#ifdef DVM_GC_WORKERS
    DVM_NativeWorld *world;
    DVM_NativeContext *Fast;
    DVM_NativeContext *Cycles;
#endif
} Scan;

PRODUCTION_ADDW


/* The poll is before an instruction, never in the middle of its operand access. */
static void execute(void *argument)
{
    Worker *worker = (Worker *)argument;
    DVM_WorldParticipant *participant;
    UWORD left = worker->identity;
    UWORD right = 0;
    UWORD result = left;
    if (!(dvm_native_context_bind(worker->context)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_native_context_set_process(worker->context, worker)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(up && up->iprog == worker))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    up->nerr = worker->identity;
    DVM_R.m = &left;
    DVM_R.s = &right;
    DVM_R.d = &result;
    DVM_R.t = result;
    participant = dvm_world_join(worker->world, worker->context);
    if (!(participant && !dvm_native_context_unbind(worker->context)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!dvm_world_stop_begin(worker->world, worker->context)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_world_enter(participant)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!dvm_world_enter(participant)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (;;)
    {
        if (!(dvm_native_context_poll()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (worker->stop)
        {
            break;
        }
        right = (UWORD)(worker->iterations & 1023);
        addw();
        DVM_R.t = result;
        worker->iterations++;
    }
    if (!(up->nerr == (int)worker->identity && up->iprog == worker))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_world_leave(participant)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!dvm_world_leave(participant)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Final stop has already ended before this owner is allowed to detach. */
    while (!dvm_world_detach(participant))
    {
        fvm_port_thread_sleep_us(100);
    }
    up->nerr = 0;
    if (!(dvm_native_context_unbind(worker->context)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* [GC:] Inspect actual REG/native-stack operands only with all mutators parked. */
static void inspect_roots(DVM_NativeContext *context, void *registers, size_t size, void *process, void *user)
{
    REG *reg = (REG *)registers;
    Worker *worker = (Worker *)process;
    Scan *scan = (Scan *)user;
    UWORD left;
    UWORD right;
    UWORD result;
    if (!(worker && worker->context == context && size == sizeof(REG)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memmove(&left, reg->m, sizeof(left));
    memmove(&right, reg->s, sizeof(right));
    memmove(&result, reg->d, sizeof(result));
    if (!(left == worker->identity && result == left + right && reg->t == (WORD)result))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scan->count++;
    if (scan->finish)
    {
        worker->stop = 1;
    }
}

#ifdef DVM_GC_WORKERS


/* These callbacks test scheduling/root publication; actual destroy/rungc are
 * tested separately against the explicit allocator fixture. */
static DVM_GCWorkResult check_pass(DVM_NativeContext *context, Scan *scan)
{
    DVM_WorldState state;
    if (!(context == dvm_native_context_current()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(scan->count == 2 && dvm_world_state(scan->world, &state)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(state.stopped && state.active == 2 && state.parked == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return DVM_GC_DONE;
}


/* Record the native fast collector's distinct up/context. */
static DVM_GCWorkResult fast_pass(DVM_NativeContext *context, size_t budget, void *user)
{
    Scan *scan = user;
    if (!(budget == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scan->Fast = context;
    return check_pass(context, scan);
}


/* Record the separate cycle collector's distinct up/context. */
static DVM_GCWorkResult cycle_pass(DVM_NativeContext *context, size_t budget, void *user)
{
    Scan *scan = user;
    if (!(budget == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scan->Cycles = context;
    return check_pass(context, scan);
}
#endif


/* Collect 100 root snapshots while both workers remain genuinely active. */
int main(void)
{
    DVM_NativeWorld *world = dvm_world_create();
    DVM_NativeContext *collector = dvm_native_process_create(0);
    FVM_PortThread *threads[2];
    Worker workers[2];
    DVM_WorldState state;
    Scan scan;
    unsigned int index;
    unsigned int cycle;
#ifdef DVM_GC_WORKERS
    DVM_GCWorkersConfig config;
    DVM_GCWorkers *gc;
    DVM_GCWorkersState gc_state;
    memset(&scan, 0, sizeof(scan));
    memset(&config, 0, sizeof(config));
    scan.world = world;
    config.world = world;
    config.create_context = dvm_native_process_create;
    config.roots = inspect_roots;
    config.Fast.run = fast_pass;
    config.Fast.budget = 1;
    config.Cycles.run = cycle_pass;
    config.Cycles.budget = 1;
    config.user = &scan;
    gc = dvm_gc_workers_create(&config);
    if (!(gc))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    if (!(world && collector && dvm_native_context_bind(collector)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (index = 0; index < 2; index++)
    {
        memset(&workers[index], 0, sizeof(workers[index]));
        workers[index].world = world;
        workers[index].identity = index + 1;
        workers[index].context = dvm_native_process_create(&workers[index]);
        if (!(workers[index].context))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        threads[index] = dvm_host_thread_start(execute, &workers[index]);
        if (!(threads[index]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (index = 0; index < 10000; index++)
    {
        if (!(dvm_world_state(world, &state)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (state.participants == 2 && state.active == 2)
        {
            break;
        }
        fvm_port_thread_sleep_us(100);
    }
    if (!(state.active == 2 && !dvm_world_destroy(world)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!dvm_world_visit(world, inspect_roots, &scan)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (cycle = 0; cycle < 100; cycle++)
    {
        scan.count = 0;
        scan.finish = cycle == 99;
#ifdef DVM_GC_WORKERS
        if (!(dvm_gc_workers_request(gc, cycle % 2 ? DVM_GC_CYCLES : DVM_GC_FAST)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(dvm_gc_workers_wait_idle(gc) && scan.count == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
#else
        if (!(dvm_world_stop_begin(world, collector)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!dvm_world_stop_begin(world, collector)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!dvm_native_context_unbind(collector)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(dvm_world_state(world, &state)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(state.stopped && state.active == 2 && state.parked == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(dvm_world_visit(world, inspect_roots, &scan) && scan.count == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(dvm_world_stop_end(world)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!dvm_world_stop_end(world)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
#endif
    }
#ifdef DVM_GC_WORKERS
    if (!(scan.Fast && scan.Cycles && scan.Fast != scan.Cycles))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_gc_workers_state(gc, &gc_state)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(gc_state.Fast.passes == 50 && gc_state.Cycles.passes == 50))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_gc_workers_destroy(gc)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    for (index = 0; index < 2; index++)
    {
        dvm_host_thread_join(threads[index]);
        if (!(workers[index].iterations > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(dvm_native_context_destroy(workers[index].context)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(dvm_world_state(world, &state) && !state.participants && !state.active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_native_context_unbind(collector)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_native_context_destroy(collector)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_world_destroy(world)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
