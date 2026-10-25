/* Tutorial: two VMs in one process retain independent modes, breakpoints and native caches. */
#include "flashvm/execution_mode.h"  /* Select interpretation for one context and JIT for another. */
#include "../jit/body.h"             /* Use the same portable x*2+2 source on every architecture. */
#include <stdio.h>                   /* Report each VM's actual native entry count. */
#include <string.h>                  /* Compare the optional strict native-mode argument. */
#define FVM_JIT_EXAMPLE_INSTANCES 2U /* Two independently owned runtime/context pairs. */
#define FVM_JIT_EXAMPLE_SURVIVOR 1U  /* Keep the second VM alive after destroying the first. */


/* Interleave calls on the owning thread; concurrent use requires separate owning threads. */
int main(int argc, char **argv)                                /* Accept no option or --both-jit for strict native evidence. */
{                                                              /* Begin process-local VM ownership. */
    const unsigned char body[] = FVM_JIT_EXAMPLE_BODY;         /* Both VMs parse their own source copy. */
    const char *parameters[] = {"x"};                          /* Give the numeric argument its local name. */
    FVM_Config config;                                         /* Initialize a reusable configuration value. */
    FVM_Runtime *runtimes[FVM_JIT_EXAMPLE_INSTANCES] = {0, 0}; /* Track partial creation safely. */
    FVM_Context *contexts[FVM_JIT_EXAMPLE_INSTANCES] = {       /* Initialize both context owners before any acquisition. */
                                                        0, 0}; /* Never exchange managed values between contexts. */
    FVM_Object *functions[FVM_JIT_EXAMPLE_INSTANCES];          /* Each function belongs to its own context. */
    FVM_Value argument, result;                                /* Only the primitive number is reused between calls. */
    FVM_ExecutionStats stats;                                  /* Inspect each context's independently owned counters. */
    unsigned int i;                                            /* Visit both independent VM instances. */
    int both_jit = 0;                                          /* Default to the original interpreter/JIT policy comparison. */
    unsigned long previous_native;                             /* Observe a fresh native entry in the surviving VM. */
    if (argc == 2 && strcmp(argv[1], "--both-jit") == 0)       /* Recognize the explicit native-only demonstration. */
    {                                                          /* Begin conditional branch: Recognize the explicit native-only demonstration. */
        both_jit = 1;                                          /* Require real native entries from both independently owned contexts. */
    } /* End conditional branch: Recognize the explicit native-only demonstration. */
    else if (argc != 1)                                       /* Reject unknown arguments before acquiring any VM resources. */
    {                                                         /* Explain the accepted command line. */
        fputs("usage: jit_instances [--both-jit]\n", stderr); /* Keep invalid requests visible. */
        return 1;                                             /* No cleanup is needed before VM creation. */
    } /* Finish argument validation. */
    if (both_jit && !fvm_jit_available())                                      /* Reject unavailable native builds before initialization. */
    {                                                                          /* A strict demonstration cannot silently pass through interpreter fallback. */
        fputs("Both-JIT mode requires an available native backend\n", stderr); /* Explain the missing prerequisite. */
        return 1;                                                              /* No runtime/context owner exists yet. */
    } /* Signing denial during later compilation is checked through native-entry evidence. */
    if (fvm_config_init(&config) != FVM_OK) /* Initialize the public configuration ABI before creating a runtime. */
    {                                       /* Begin conditional branch: Initialize the public configuration ABI before creating a runtime. */
        goto failed;                        /* Initialize the configuration ABI. */
    } /* End conditional branch: Initialize the public configuration ABI before creating a runtime. */
    for (i = 0; i < FVM_JIT_EXAMPLE_INSTANCES; ++i)              /* Create both VMs before executing either. */
    {                                                            /* Begin the lifetime of one runtime/context pair. */
        if (fvm_runtime_create(&config, &runtimes[i]) != FVM_OK) /* Acquire this VM's runtime owner. */
        {                                                        /* Begin conditional branch: Acquire this VM's runtime owner. */
            goto failed;                                         /* Own an independent runtime. */
        } /* End conditional branch: Acquire this VM's runtime owner. */
        if (fvm_context_create(runtimes[i], &contexts[i]) != FVM_OK) /* Acquire its independent context. */
        {                                                            /* Begin conditional branch: Acquire its independent context. */
            goto failed;                                             /* Own its graph and policy. */
        } /* End conditional branch: Acquire its independent context. */
        if (fvm_function_create_avm1(contexts[i], body, sizeof(body), 1, parameters, 0, /* Copy the bytecode and parameter declaration into this VM. */
                                     &functions[i]) != FVM_OK)                          /* Store the callable in its owning context only. */
        {                                                                               /* Begin scope: Store the callable in its owning context only. */
            goto failed;                                                                /* Keep callable ownership local. */
        } /* End scope: Store the callable in its owning context only. */
        if (fvm_execution_set_mode(                                                                                                /* Set each context's policy without sharing its cache. */
                                   contexts[i], !both_jit && i == 0 ? FVM_EXECUTION_INTERPRETER : FVM_EXECUTION_JIT, 0) != FVM_OK) /* Select both JIT policies only when explicitly requested. */
        {                                                                                                                          /* Begin scope: Select both JIT policies only when explicitly requested. */
            goto failed;                                                                                                           /* Choose different policies in the same process. */
        } /* End scope: Select both JIT policies only when explicitly requested. */
    } /* Both VMs remain alive for the following calls. */
    for (i = 0; i < FVM_JIT_EXAMPLE_INSTANCES; ++i)                                             /* Run each independently configured VM. */
    {                                                                                           /* Begin one ordinary function invocation. */
        fvm_value_number(&argument,                                                             /* Prepare the numeric input without sharing a managed value. */
                         FVM_JIT_EXAMPLE_INPUT);                                                /* Reuse only a primitive input value. */
        if (fvm_function_invoke(contexts[i], functions[i], 0, 1, &argument, &result) != FVM_OK) /* Invoke with the original primitive input. */
        {                                                                                       /* Begin conditional branch: Invoke with the original primitive input. */
            goto failed;                                                                        /* Invoke in the function's owning context. */
        } /* End conditional branch: Invoke with the original primitive input. */
        if (result.type != FVM_VALUE_NUMBER || result.number != FVM_JIT_EXAMPLE_EXPECTED) /* Check the independent numeric oracle. */
        {                                                                                 /* Begin conditional branch: Check the independent numeric oracle. */
            goto failed;                                                                  /* Check interpreted/native parity. */
        } /* End conditional branch: Check the independent numeric oracle. */
        stats.struct_size = sizeof(stats);                          /* Initialize the statistics ABI before reading it. */
        if (fvm_execution_get_stats(contexts[i], &stats) != FVM_OK) /* Inspect actual body-entry counters. */
        {                                                           /* Begin conditional branch: Inspect actual body-entry counters. */
            goto failed;                                            /* Read this VM's counters only. */
        } /* End conditional branch: Inspect actual body-entry counters. */
        printf("VM %u: result=%.0f native=%lu backend=%s\n", i, result.number, stats.native_entries, /* Report the independent VM and observed result. */
               fvm_jit_backend_name());                                                              /* Identify actual execution. */
        if (!both_jit && i == 0 && stats.native_entries != 0)                                        /* The default first VM must stay interpreted. */
        {                                                                                            /* Begin conditional branch: The default first VM must stay interpreted. */
            goto failed;                                                                             /* Confirm the interpreter VM ignored the other's policy. */
        } /* End conditional branch: The default first VM must stay interpreted. */
        if (both_jit && !stats.native_entries) /* Detect automatic interpreter fallback in strict mode. */
        {                                      /* Begin conditional branch: Detect automatic interpreter fallback in strict mode. */
            goto failed;                       /* A correct result alone does not prove JIT execution. */
        } /* End conditional branch: Detect automatic interpreter fallback in strict mode. */
    } /* Finish calls while both contexts still exist. */
    previous_native = stats.native_entries;                                                          /* The loop ended with the second VM's counters. */
    fvm_context_destroy(contexts[0]);                                                                /* Release the first VM and its automatic code cache. */
    contexts[0] = 0;                                                                                 /* Prevent cleanup from destroying this context twice. */
    fvm_runtime_destroy(runtimes[0]);                                                                /* Drop only the first VM's runtime owner. */
    runtimes[0] = 0;                                                                                 /* Keep later failure cleanup safe. */
    if (fvm_function_invoke(contexts[FVM_JIT_EXAMPLE_SURVIVOR], functions[FVM_JIT_EXAMPLE_SURVIVOR], /* Use only the surviving context's function. */
                            0, 1, &argument, &result) != FVM_OK)                                     /* Reuse the same primitive input after the other VM's teardown. */
    {                                                                                                /* Begin scope: Reuse the same primitive input after the other VM's teardown. */
        goto failed;                                                                                 /* Preserve a survivor invocation error. */
    } /* End scope: Reuse the same primitive input after the other VM's teardown. */
    if (result.type != FVM_VALUE_NUMBER || result.number != FVM_JIT_EXAMPLE_EXPECTED) /* Compare the survivor with the independent numeric oracle. */
    {                                                                                 /* Begin conditional branch: Compare the survivor with the independent numeric oracle. */
        goto failed;                                                                  /* Teardown must not corrupt its callable state. */
    } /* End conditional branch: Compare the survivor with the independent numeric oracle. */
    if (fvm_execution_get_stats(contexts[FVM_JIT_EXAMPLE_SURVIVOR], &stats) != FVM_OK) /* Read only the surviving VM's counters. */
    {                                                                                  /* Begin conditional branch: Read only the surviving VM's counters. */
        goto failed;                                                                   /* Report an invalid statistics result. */
    } /* End conditional branch: Read only the surviving VM's counters. */
    if (both_jit && stats.native_entries <= previous_native) /* Require a fresh native entry in strict mode. */
    {                                                        /* Begin conditional branch: Require a fresh native entry in strict mode. */
        goto failed;                                         /* Do not mislabel interpreter fallback as pool-lifetime acceptance. */
    } /* End conditional branch: Require a fresh native entry in strict mode. */
    printf("survivor VM %u: result=%.0f native=%lu\n", FVM_JIT_EXAMPLE_SURVIVOR, /* Identify the remaining owner. */
           result.number, stats.native_entries);                                 /* Show the result and actual entry count after teardown. */
    for (i = 0; i < FVM_JIT_EXAMPLE_INSTANCES; ++i)                              /* Release every independent pair. */
    {                                                                            /* Destroy the context before its enclosing runtime. */
        if (contexts[i])                                                         /* The first VM was already destroyed before the survivor check. */
        {                                                                        /* Begin conditional branch: The first VM was already destroyed before the survivor check. */
            fvm_context_destroy(contexts[i]);                                    /* Free this VM's objects and automatic native cache. */
        } /* End conditional branch: The first VM was already destroyed before the survivor check. */
        if (runtimes[i])                      /* Release only the remaining initialized runtime owner. */
        {                                     /* Begin conditional branch: Release only the remaining initialized runtime owner. */
            fvm_runtime_destroy(runtimes[i]); /* Release only this VM's runtime. */
        } /* End conditional branch: Release only the remaining initialized runtime owner. */
    } /* All process-local resources are released. */
    return 0;                                          /* Report successful independent execution. */
failed:                                                /* Clean up a partially created set without relying on assertions. */
    fputs("Multiple-VM JIT example failed\n", stderr); /* Identify release-mode errors. */
    for (i = 0; i < FVM_JIT_EXAMPLE_INSTANCES; ++i)    /* Visit every initialized pointer slot. */
    {                                                  /* Unwind one possibly partial runtime/context pair. */
        if (contexts[i])                               /* Ignore any context that was never created or was already released. */
        {                                              /* Begin conditional branch: Ignore any context that was never created or was already released. */
            fvm_context_destroy(contexts[i]);          /* Drop any created context first. */
        } /* End conditional branch: Ignore any context that was never created or was already released. */
        if (runtimes[i])                      /* Ignore any runtime that was never created or was already released. */
        {                                     /* Begin conditional branch: Ignore any runtime that was never created or was already released. */
            fvm_runtime_destroy(runtimes[i]); /* Then drop its runtime. */
        } /* End conditional branch: Ignore any runtime that was never created or was already released. */
    } /* Every successfully allocated resource is released. */
    return 1; /* Report failure to the process launcher. */
} /* Finish the example. */
