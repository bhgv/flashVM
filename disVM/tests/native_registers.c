/* Run the retained addw instruction concurrently with independent real REG files. */
#include <stdio.h>
#include <stdlib.h>
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


/* Reusable native barrier; condition predicates prevent lost/spurious wakeups. */
typedef struct Barrier
{
    FVM_PortSync *sync;
    unsigned int arrived;
    unsigned int generation;
} Barrier;


/* Each worker owns a native context; only the test barrier is shared. */
typedef struct Worker
{
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
    if (barrier->arrived == 2)
    {
        barrier->arrived = 0;
        barrier->generation++;
        dvm_host_sync_wake_all(barrier->sync);
    }
    else
    {
        while (barrier->generation == generation)
        {
            dvm_host_sync_wait(barrier->sync);
        }
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
    if (!(dvm_native_context_bind(worker->context)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!dvm_native_context_bind(worker->context)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!dvm_native_context_destroy(worker->context)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_native_context_runtime(worker->context) == worker))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_native_context_set_process(worker->context, worker)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_native_context_process(worker->context) == worker))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(up && up->iprog == worker))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    up->nerr = (int)worker->identity;
    worker->register_address = &DVM_R;
    for (index = 0; index < 2000; index++)
    {
        left = worker->identity;
        right = index;
        result = 0;
        DVM_R.m = &left;
        DVM_R.s = &right;
        DVM_R.d = &result;
        DVM_R.t = worker->identity;
        rendezvous(worker->barrier);
        addw();
        if (!(result == left + right && DVM_R.t == worker->identity))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(up->nerr == (int)worker->identity && up->iprog == worker))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rendezvous(worker->barrier);
    }
    up->nerr = 0;
    if (!(dvm_native_context_set_process(worker->context, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_native_context_unbind(worker->context)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!dvm_native_context_current()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!dvm_native_context_unbind(worker->context)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* This tests real parallel instruction bodies, not a complete shared-heap Dis VM. */
int main(void)
{
    Barrier barrier = {0, 0, 0};
    Worker workers[2];
    FVM_PortThread *threads[2];
    unsigned int index;
    barrier.sync = dvm_host_sync_create();
    if (!(barrier.sync && !dvm_native_context_current()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (index = 0; index < 2; index++)
    {
        workers[index].barrier = &barrier;
        workers[index].identity = 1000 * (index + 1);
        workers[index].register_address = 0;
        workers[index].context = dvm_native_process_create(&workers[index]);
        if (!(workers[index].context))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!dvm_native_context_registers(workers[index].context)))
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
    for (index = 0; index < 2; index++)
    {
        dvm_host_thread_join(threads[index]);
    }
    if (!(workers[0].register_address != workers[1].register_address))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (index = 0; index < 2; index++)
    {
        if (!(dvm_native_context_destroy(workers[index].context)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    dvm_host_sync_destroy(barrier.sync);
    return 0;
}
