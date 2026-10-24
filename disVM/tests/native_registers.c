/* Run the retained addw instruction concurrently with independent real REG files. */
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "disvm/native_context.h"
#include "disvm/native_process.h"
#include "disvm/flashvm_host.h"
#ifdef NDEBUG
#undef NDEBUG /* Product checks must execute in Release builds too. */
#endif
#include <assert.h>

/* Reusable native barrier; condition predicates prevent lost/spurious wakeups. */
typedef struct Barrier {
    FVM_PortSync *sync;
    unsigned int arrived;
    unsigned int generation;
} Barrier;
/* Each worker owns a native context; only the test barrier is shared. */
typedef struct Worker {
    DVM_NativeContext *context;
    Barrier *barrier;
    UWORD identity;
    void *register_address;
} Worker;

/* Both OS threads must reach every phase before either evaluates its operands. */
static void rendezvous(Barrier *barrier)
{
    unsigned int generation;
    dvm_host_sync_lock(barrier->sync);
    generation = barrier->generation;
    barrier->arrived++;
    if (barrier->arrived == 2) {
        barrier->arrived = 0;
        barrier->generation++;
        dvm_host_sync_wake_all(barrier->sync);
    } else {
        while (barrier->generation == generation)
            dvm_host_sync_wait(barrier->sync);
    }
    dvm_host_sync_unlock(barrier->sync);
}

PRODUCTION_ADDW

/* Instruction accesses use the same DVM_R macro as the experimental whole core. */
static void execute(void *argument)
{
    Worker *worker = (Worker *)argument;
    UWORD left;
    UWORD right;
    UWORD result;
    unsigned int index;
    assert(dvm_native_context_bind(worker->context));
    assert(!dvm_native_context_bind(worker->context));
    assert(!dvm_native_context_destroy(worker->context));
    assert(dvm_native_context_runtime(worker->context) == worker);
    assert(dvm_native_context_set_process(worker->context, worker));
    assert(dvm_native_context_process(worker->context) == worker);
    assert(up && up->iprog == worker);
    up->nerr = (int)worker->identity;
    worker->register_address = &DVM_R;
    for (index = 0; index < 2000; index++) {
        left = worker->identity;
        right = index;
        result = 0;
        DVM_R.m = &left;
        DVM_R.s = &right;
        DVM_R.d = &result;
        DVM_R.t = worker->identity;
        rendezvous(worker->barrier);
        addw();
        assert(result == left + right && DVM_R.t == worker->identity);
        assert(up->nerr == (int)worker->identity && up->iprog == worker);
        rendezvous(worker->barrier);
    }
    up->nerr = 0;
    assert(dvm_native_context_set_process(worker->context, 0));
    assert(dvm_native_context_unbind(worker->context));
    assert(!dvm_native_context_current());
    assert(!dvm_native_context_unbind(worker->context));
}

/* This tests real parallel instruction bodies, not a complete shared-heap Dis VM. */
int main(void)
{
    Barrier barrier = {0, 0, 0};
    Worker workers[2];
    FVM_PortThread *threads[2];
    unsigned int index;
    barrier.sync = dvm_host_sync_create();
    assert(barrier.sync && !dvm_native_context_current());
    for (index = 0; index < 2; index++) {
        workers[index].barrier = &barrier;
        workers[index].identity = 1000 * (index + 1);
        workers[index].register_address = 0;
        workers[index].context = dvm_native_process_create(&workers[index]);
        assert(workers[index].context);
        assert(!dvm_native_context_registers(workers[index].context));
        threads[index] = dvm_host_thread_start(execute, &workers[index]);
        assert(threads[index]);
    }
    for (index = 0; index < 2; index++)
        dvm_host_thread_join(threads[index]);
    assert(workers[0].register_address != workers[1].register_address);
    for (index = 0; index < 2; index++)
        assert(dvm_native_context_destroy(workers[index].context));
    dvm_host_sync_destroy(barrier.sync);
    return 0;
}
