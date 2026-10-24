/* Real instructions on two native up objects while a third thread scans stable roots. */
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
#include <assert.h>

/* Each worker owns its mutable fields; collector touches them only while stopped. */
typedef struct Worker {
    DVM_NativeWorld *world;
    DVM_NativeContext *context;
    unsigned int identity;
    unsigned long iterations;
    int stop;
} Worker;
/* One collector scan counts exact root sets and optionally requests final exit. */
typedef struct Scan {
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
    assert(dvm_native_context_bind(worker->context));
    assert(dvm_native_context_set_process(worker->context, worker));
    assert(up && up->iprog == worker);
    up->nerr = worker->identity;
    DVM_R.m = &left;
    DVM_R.s = &right;
    DVM_R.d = &result;
    DVM_R.t = result;
    participant = dvm_world_join(worker->world, worker->context);
    assert(participant && !dvm_native_context_unbind(worker->context));
    assert(!dvm_world_stop_begin(worker->world, worker->context));
    assert(dvm_world_enter(participant));
    assert(!dvm_world_enter(participant));
    for (;;) {
        assert(dvm_native_context_poll());
        if (worker->stop)
            break;
        right = (UWORD)(worker->iterations & 1023);
        addw();
        DVM_R.t = result;
        worker->iterations++;
    }
    assert(up->nerr == (int)worker->identity && up->iprog == worker);
    assert(dvm_world_leave(participant));
    assert(!dvm_world_leave(participant));
    /* Final stop has already ended before this owner is allowed to detach. */
    while (!dvm_world_detach(participant))
        fvm_port_thread_sleep_us(100);
    up->nerr = 0;
    assert(dvm_native_context_unbind(worker->context));
}

/* [GC:] Inspect actual REG/native-stack operands only with all mutators parked. */
static void inspect_roots(DVM_NativeContext *context, void *registers, size_t size,
                          void *process, void *user)
{
    REG *reg = (REG *)registers;
    Worker *worker = (Worker *)process;
    Scan *scan = (Scan *)user;
    UWORD left;
    UWORD right;
    UWORD result;
    assert(worker && worker->context == context && size == sizeof(REG));
    memmove(&left, reg->m, sizeof(left));
    memmove(&right, reg->s, sizeof(right));
    memmove(&result, reg->d, sizeof(result));
    assert(left == worker->identity && result == left + right && reg->t == (WORD)result);
    scan->count++;
    if (scan->finish)
        worker->stop = 1;
}

#ifdef DVM_GC_WORKERS
/* These callbacks test scheduling/root publication; actual destroy/rungc are
 * tested separately against the explicit allocator fixture. */
static DVM_GCWorkResult check_pass(DVM_NativeContext *context, Scan *scan)
{
    DVM_WorldState state;
    assert(context == dvm_native_context_current());
    assert(scan->count == 2 && dvm_world_state(scan->world, &state));
    assert(state.stopped && state.active == 2 && state.parked == 2);
    return DVM_GC_DONE;
}
/* Record the native fast collector's distinct up/context. */
static DVM_GCWorkResult fast_pass(DVM_NativeContext *context, size_t budget, void *user)
{
    Scan *scan = user;
    assert(budget == 1);
    scan->Fast = context;
    return check_pass(context, scan);
}
/* Record the separate cycle collector's distinct up/context. */
static DVM_GCWorkResult cycle_pass(DVM_NativeContext *context, size_t budget, void *user)
{
    Scan *scan = user;
    assert(budget == 1);
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
    assert(gc);
#endif
    assert(world && collector && dvm_native_context_bind(collector));
    for (index = 0; index < 2; index++) {
        memset(&workers[index], 0, sizeof(workers[index]));
        workers[index].world = world;
        workers[index].identity = index + 1;
        workers[index].context = dvm_native_process_create(&workers[index]);
        assert(workers[index].context);
        threads[index] = dvm_host_thread_start(execute, &workers[index]);
        assert(threads[index]);
    }
    for (index = 0; index < 10000; index++) {
        assert(dvm_world_state(world, &state));
        if (state.participants == 2 && state.active == 2)
            break;
        fvm_port_thread_sleep_us(100);
    }
    assert(state.active == 2 && !dvm_world_destroy(world));
    assert(!dvm_world_visit(world, inspect_roots, &scan));
    for (cycle = 0; cycle < 100; cycle++) {
        scan.count = 0;
        scan.finish = cycle == 99;
#ifdef DVM_GC_WORKERS
        assert(dvm_gc_workers_request(gc, cycle % 2 ? DVM_GC_CYCLES : DVM_GC_FAST));
        assert(dvm_gc_workers_wait_idle(gc) && scan.count == 2);
#else
        assert(dvm_world_stop_begin(world, collector));
        assert(!dvm_world_stop_begin(world, collector));
        assert(!dvm_native_context_unbind(collector));
        assert(dvm_world_state(world, &state));
        assert(state.stopped && state.active == 2 && state.parked == 2);
        assert(dvm_world_visit(world, inspect_roots, &scan) && scan.count == 2);
        assert(dvm_world_stop_end(world));
        assert(!dvm_world_stop_end(world));
#endif
    }
#ifdef DVM_GC_WORKERS
    assert(scan.Fast && scan.Cycles && scan.Fast != scan.Cycles);
    assert(dvm_gc_workers_state(gc, &gc_state));
    assert(gc_state.Fast.passes == 50 && gc_state.Cycles.passes == 50);
    assert(dvm_gc_workers_destroy(gc));
#endif
    for (index = 0; index < 2; index++) {
        dvm_host_thread_join(threads[index]);
        assert(workers[index].iterations > 0);
        assert(dvm_native_context_destroy(workers[index].context));
    }
    assert(dvm_world_state(world, &state) && !state.participants && !state.active);
    assert(dvm_native_context_unbind(collector));
    assert(dvm_native_context_destroy(collector));
    assert(dvm_world_destroy(world));
    return 0;
}
