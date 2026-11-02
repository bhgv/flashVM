/* Run the retained addw instruction concurrently with independent real REG files. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
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
    void *execution_address; /* Owner-only guards must have disjoint native storage. */
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

/* The retained instruction's invalid-span branch must remain linked and fatal.
 * These leaf fixtures supply error reporting, not a replacement VM exception ABI. */
char exBounds[] = "array bounds error"; /* Actual addw guard identity in this fixture. */


/* Reject an unexpected production instruction fault; message is borrowed.
 * No interpreter unwinding is mocked as success in this focused fixture. */
void error(char *message)
{
    fprintf(stderr,"FAIL native instruction: %s\n",message);
    exit(EXIT_FAILURE);
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
    up->env = &up->defenv;
    up->env->errstr = up->env->errbuf1;
    up->env->syserrstr = up->env->errbuf0;
    worker->register_address = &DVM_R;
    worker->execution_address = dvm_native_execution();
    if (dvm_native_execution()->idle_slices || dvm_native_execution()->instruction_boundary_error)
    {
        fprintf(stderr, "FAIL native execution guards not initially zero\n");
        exit(EXIT_FAILURE);
    }
    for (index = 0; index < 2000; index++)
    {
        char expected[64];
        REG saved;
        unsigned long slices = (unsigned long)worker->identity * 2000UL + index;
        int boundary = worker->identity == 1000;
        dvm_native_execution()->idle_slices = slices;
        dvm_native_execution()->instruction_boundary_error = boundary;
        snprintf(expected, sizeof(expected), "owner %lu turn %u", (unsigned long)worker->identity, index);
        strcpy(up->env->errstr, expected);
        strcpy(up->env->syserrstr, expected);
        left = worker->identity;
        right = index;
        result = 0;
        DVM_R.m = &left;
        DVM_R.s = &right;
        DVM_R.d = &result;
        DVM_R.t = worker->identity;
        rendezvous(worker->barrier);
        addw();
        saved = DVM_R;
        memset(&DVM_R, 0, sizeof(DVM_R));
        DVM_R = saved;
        if (dvm_native_execution()->idle_slices != slices ||
            dvm_native_execution()->instruction_boundary_error != boundary ||
            strcmp(up->env->errstr, expected) || strcmp(up->env->syserrstr, expected))
        {
            fprintf(stderr, "FAIL foreign guard/error state or REG copy overwrote native owner\n");
            exit(EXIT_FAILURE);
        }
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
        workers[index].execution_address = 0;
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
    if (!(workers[0].register_address != workers[1].register_address) ||
        workers[0].execution_address == workers[1].execution_address)
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (index = 0; index < 2; index++)
    {
        /* The former workers have joined. Transfer each unbound native context
         * to this real OS thread; its guards/environment must follow the owner,
         * not the previous worker's TLS slot or a saved register snapshot. */
        unsigned long expected = (unsigned long)workers[index].identity * 2000UL + 1999;
        if (!dvm_native_context_bind(workers[index].context) ||
            dvm_native_execution()->idle_slices != expected ||
            dvm_native_execution()->instruction_boundary_error != (index == 0) ||
            up->env != &up->defenv || up->env->errstr != up->env->errbuf1 ||
            up->env->syserrstr != up->env->errbuf0 || !dvm_native_context_unbind(workers[index].context))
        {
            fprintf(stderr, "FAIL joined owner guard/environment transfer\n");
            exit(EXIT_FAILURE);
        }
        if (!(dvm_native_context_destroy(workers[index].context)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    dvm_host_sync_destroy(barrier.sync);
    return 0;
}
