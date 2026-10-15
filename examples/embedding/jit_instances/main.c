/* Tutorial: two VMs in one process retain independent modes, breakpoints and native caches. */
#include "flashvm/execution_mode.h" /* Select interpretation for one context and JIT for another. */
#include "../jit/body.h"            /* Use the same portable x*2+2 source on every architecture. */
#include <stdio.h>                  /* Report each VM's actual native entry count. */
#define FVM_JIT_EXAMPLE_INSTANCES 2U /* Two independently owned runtime/context pairs. */
/* Interleave calls on the owning thread; concurrent use requires separate owning threads. */
int main(void)
{                                                      /* Begin process-local VM ownership. */
    const unsigned char body[] = FVM_JIT_EXAMPLE_BODY; /* Both VMs parse their own source copy. */
    const char *parameters[] = {"x"}; /* Give the numeric argument its local name. */
    FVM_Config config;                /* Initialize a reusable configuration value. */
    FVM_Runtime *runtimes[FVM_JIT_EXAMPLE_INSTANCES] = {0, 0}; /* Track partial creation safely. */
    FVM_Context *contexts[FVM_JIT_EXAMPLE_INSTANCES] = {
        0, 0}; /* Never exchange managed values between contexts. */
    FVM_Object
        *functions[FVM_JIT_EXAMPLE_INSTANCES]; /* Each function belongs to its own context. */
    FVM_Value argument, result; /* Only the primitive number is reused between calls. */
    FVM_ExecutionStats stats;   /* Inspect each context's independently owned counters. */
    unsigned int i;             /* Visit both independent VM instances. */
    if (fvm_config_init(&config) != FVM_OK)
        goto failed;                                /* Initialize the configuration ABI. */
    for (i = 0; i < FVM_JIT_EXAMPLE_INSTANCES; ++i) /* Create both VMs before executing either. */
    { /* Begin the lifetime of one runtime/context pair. */
        if (fvm_runtime_create(&config, &runtimes[i]) != FVM_OK)
            goto failed; /* Own an independent runtime. */
        if (fvm_context_create(runtimes[i], &contexts[i]) != FVM_OK)
            goto failed; /* Own its graph and policy. */
        if (fvm_function_create_avm1(contexts[i], body, sizeof(body), 1, parameters, 0,
                                     &functions[i]) != FVM_OK)
            goto failed; /* Keep callable ownership local. */
        if (fvm_execution_set_mode(
                contexts[i], i == 0 ? FVM_EXECUTION_INTERPRETER : FVM_EXECUTION_JIT, 0) != FVM_OK)
            goto failed; /* Choose different policies in the same process. */
    } /* Both VMs remain alive for the following calls. */
    for (i = 0; i < FVM_JIT_EXAMPLE_INSTANCES; ++i) /* Run each independently configured VM. */
    {                                               /* Begin one ordinary function invocation. */
        fvm_value_number(&argument,
                         FVM_JIT_EXAMPLE_INPUT); /* Reuse only a primitive input value. */
        if (fvm_function_invoke(contexts[i], functions[i], 0, 1, &argument, &result) != FVM_OK)
            goto failed; /* Invoke in the function's owning context. */
        if (result.type != FVM_VALUE_NUMBER || result.number != FVM_JIT_EXAMPLE_EXPECTED)
            goto failed;                   /* Check interpreted/native parity. */
        stats.struct_size = sizeof(stats); /* Initialize the statistics ABI before reading it. */
        if (fvm_execution_get_stats(contexts[i], &stats) != FVM_OK)
            goto failed; /* Read this VM's counters only. */
        printf("VM %u: result=%.0f native=%lu backend=%s\n", i, result.number, stats.native_entries,
               fvm_jit_backend_name()); /* Identify actual execution. */
        if (i == 0 && stats.native_entries != 0)
            goto failed; /* Confirm the interpreter VM ignored the other's policy. */
    } /* Finish calls while both contexts still exist. */
    for (i = 0; i < FVM_JIT_EXAMPLE_INSTANCES; ++i) /* Release every independent pair. */
    {                                     /* Destroy the context before its enclosing runtime. */
        fvm_context_destroy(contexts[i]); /* Free this VM's objects and automatic native cache. */
        fvm_runtime_destroy(runtimes[i]); /* Release only this VM's runtime. */
    } /* All process-local resources are released. */
    return 0; /* Report successful independent execution. */
failed:       /* Clean up a partially created set without relying on assertions. */
    fputs("Multiple-VM JIT example failed\n", stderr); /* Identify release-mode errors. */
    for (i = 0; i < FVM_JIT_EXAMPLE_INSTANCES; ++i)    /* Visit every initialized pointer slot. */
    { /* Unwind one possibly partial runtime/context pair. */
        if (contexts[i])
            fvm_context_destroy(contexts[i]); /* Drop any created context first. */
        if (runtimes[i])
            fvm_runtime_destroy(runtimes[i]); /* Then drop its runtime. */
    } /* Every successfully allocated resource is released. */
    return 1; /* Report failure to the process launcher. */
} /* Finish the example. */
