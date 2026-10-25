/* Tutorial: two native OS workers own separate VMs and independent automatic JIT caches. */
#include "flashvm/execution_mode.h" /* Select policy separately inside each VM. */
#include "flashvm/worker.h"         /* Start OS threads and join before releasing host state. */
#include "../jit/body.h"            /* Compute x*2+2 without shared mutable script objects. */
#include <stdio.h>                  /* Report results on the main thread after both joins. */
#define FVM_JIT_EXAMPLE_WORKERS 2U  /* Demonstrate two independently owned concurrent VMs. */


/* Execute on a worker-owned context; only primitive values cross the worker boundary. */
static FVM_Result task(FVM_Context *context, const FVM_Value *input, FVM_Value *output, void *data)
{                                       /* Begin execution on this worker's native thread. */
    const unsigned char body[] =
        FVM_JIT_EXAMPLE_BODY;           /* Each worker retains its own function source. */
    const char *parameters[] = {"x"};   /* Bind the copied numeric argument. */
    FVM_Object *function;               /* Keep this function entirely inside the worker VM. */
    FVM_ExecutionStats stats;           /* Confirm this worker actually entered native code when available. */
    FVM_Result status;                  /* Return every failure through the worker result status. */
    (void)data;                         /* No host mutable state is shared between the workers. */
    status = fvm_execution_set_mode(context, FVM_EXECUTION_JIT,
                                    0); /* Set this VM's own execution policy. */
    if (status != FVM_OK)
    {                                   /* Begin conditional branch: if (status != FVM_OK). */
        return status;                  /* Stop before constructing script state on failure. */
    } /* End conditional branch: if (status != FVM_OK). */
    status = fvm_function_create_avm1(context, body, sizeof(body), 1, parameters, 0,
                                      &function); /* Allocate a context-local function. */
    if (status != FVM_OK)
    {                                             /* Begin conditional branch: if (status != FVM_OK). */
        return status;                            /* Preserve allocation and validation failures. */
    } /* End conditional branch: if (status != FVM_OK). */
    status = fvm_function_invoke(context, function, 0, 1, input,
                                 output); /* Use only this worker's function and context. */
    if (status != FVM_OK)
    {                                     /* Begin conditional branch: if (status != FVM_OK). */
        return status;                    /* Preserve script and compiler failures. */
    } /* End conditional branch: if (status != FVM_OK). */
    stats.struct_size = sizeof(stats);                 /* Initialize the context statistics record. */
    status = fvm_execution_get_stats(context, &stats); /* Read counters on the owning thread. */
    if (status != FVM_OK)
    {                                                  /* Begin conditional branch: if (status != FVM_OK). */
        return status;                                 /* Preserve an invalid statistics request. */
    } /* End conditional branch: if (status != FVM_OK). */
    if (fvm_jit_available() && stats.native_entries != 1)
    {                     /* Begin conditional branch: if (fvm_jit_available() && stats.native_entries != 1). */
        return FVM_ERROR; /* Require native entry for this supported body. */
    } /* End conditional branch: if (fvm_jit_available() && stats.native_entries != 1). */
    return FVM_OK; /* Return the primitive answer after verifying execution. */
} /* The worker framework later destroys this VM and its owned cache. */


/* Launch all workers before joining, allowing their native/interpreted execution to overlap. */
int main(void)
{                                                     /* Begin main-thread ownership of joinable worker handles. */
    FVM_Config config;                                /* Worker configuration is copied before each thread starts. */
    FVM_Worker *workers[FVM_JIT_EXAMPLE_WORKERS] = {
        0, 0};                                        /* Track partially started workers safely. */
    FVM_Value input, output[FVM_JIT_EXAMPLE_WORKERS]; /* Exchange primitive values only. */
    FVM_Result status;                                /* Distinguish unavailable OS ports from actual execution failures. */
    unsigned int i;                                   /* Visit each owned worker handle. */
    if (fvm_config_init(&config) != FVM_OK)
    {                                                 /* Begin conditional branch: if (fvm_config_init(&config) != FVM_OK). */
        goto failed;                                  /* Prepare the runtime defaults. */
    } /* End conditional branch: if (fvm_config_init(&config) != FVM_OK). */
    for (i = 0; i < FVM_JIT_EXAMPLE_WORKERS;
         ++i)                                             /* Start every thread before waiting for results. */
    {                                                     /* Begin one independent worker lifetime. */
        fvm_value_number(&input,
                         FVM_JIT_EXAMPLE_INPUT + i);      /* Copy x=20 or x=21 into this worker. */
        status = fvm_worker_run(&config, task, &input, 0,
                                &workers[i]);             /* Create a separate runtime/context/thread. */
        if (status == FVM_UNSUPPORTED &&
            i == 0)                                       /* Minimal builds deliberately exclude OS workers. */
        {                                                 /* Explain this build's limitation without hiding another error. */
            puts("OS workers unavailable in this build"); /* Enable OS ports for this tutorial. */
            return 0;                                     /* No worker handles were created. */
        } /* Finish the explicitly unsupported path. */
        if (status != FVM_OK)
        {                /* Begin conditional branch: if (status != FVM_OK). */
            goto failed; /* Clean up any earlier workers on partial failure. */
        } /* End conditional branch: if (status != FVM_OK). */
    } /* Every successfully created worker is now independently executing. */
    for (i = 0; i < FVM_JIT_EXAMPLE_WORKERS; ++i) /* Join and validate each numeric result. */
    {                                             /* Begin one join operation. */
        if (fvm_worker_join(workers[i], &output[i]) != FVM_OK)
        {                                         /* Begin conditional branch: if (fvm_worker_join(workers[i], &output[i]) != FVM_OK). */
            goto failed;                          /* Wait without touching the worker context. */
        } /* End conditional branch: if (fvm_worker_join(workers[i], &output[i]) != FVM_OK). */
        if (output[i].type != FVM_VALUE_NUMBER ||
            output[i].number != FVM_JIT_EXAMPLE_EXPECTED + i * 2)
        {                /* Begin scope: output[i].number != FVM_JIT_EXAMPLE_EXPECTED + i * 2). */
            goto failed; /* Check x*2+2. */
        } /* End scope: output[i].number != FVM_JIT_EXAMPLE_EXPECTED + i * 2). */
        printf("worker %u (%s): %.0f\n", i, fvm_jit_backend_name(),
               output[i].number); /* Print after the worker has stopped. */
    } /* Both workers have joined before handle destruction. */
    for (i = 0; i < FVM_JIT_EXAMPLE_WORKERS; ++i)
    {                                   /* Begin loop body: for (i = 0; i < FVM_JIT_EXAMPLE_WORKERS; ++i). */
        fvm_worker_destroy(workers[i]); /* Release joined handles. */
    } /* End loop body: for (i = 0; i < FVM_JIT_EXAMPLE_WORKERS; ++i). */
    return 0;                                     /* Report successful concurrent independent execution. */
failed:                                           /* Join even partially started workers before releasing their handles. */
    fputs("JIT worker example failed\n", stderr); /* Make failures visible in release builds. */
    for (i = 0; i < FVM_JIT_EXAMPLE_WORKERS; ++i) /* Visit only initialized array entries. */
    {                                             /* Begin cleanup of one possible worker. */
        if (workers[i])                           /* A failed start leaves no owned handle. */
        {                                         /* Keep userdata and VM lifetimes valid until completion. */
            fvm_worker_join(workers[i], 0);       /* Wait even if another worker failed. */
            fvm_worker_destroy(workers[i]);       /* Destroy only after join. */
        } /* Finish cleanup of a created handle. */
    } /* All created worker resources are released. */
    return 1; /* Report the original failure. */
} /* Finish the main-thread example. */
