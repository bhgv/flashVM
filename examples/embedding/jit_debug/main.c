/* Tutorial: a breakpoint interprets its body while the surrounding context requests JIT. */
#include "flashvm/execution_mode.h" /* Select JIT and inspect actual entry counters. */
#include "flashvm/debug.h" /* Map source lines and set a synchronous breakpoint callback. */
#include "../jit/body.h"   /* Reuse x*2+2 and its named expected result. */
#include <stdio.h>         /* Report source locations and failures to the embedding host. */
/* Inspect a stopped instruction; returning OK resumes on the same owning thread. */
static FVM_Result stopped(FVM_Context *context, const FVM_DebugBreakpoint *at, void *data)
{                                              /* Begin the synchronous debugger callback. */
    unsigned int *hits = (unsigned int *)data; /* Borrow main's counter until execution returns. */
    (void)context; /* A real debugger can query its permitted inspection APIs here. */
    ++*hits;       /* Record this interpreted stop independently of native statistics. */
    printf("breakpoint: %s:%lu offset=%lu\n", at->source, at->line,
           at->offset); /* Display mapped metadata. */
    return FVM_OK;      /* Resume; return an error status instead to stop this invocation. */
} /* Finish the debugger callback. */
/* Execute the same function before, during and after breakpoint installation. */
int main(void)
{ /* Begin resources owned by this embedding thread. */
    const unsigned char body[] =
        FVM_JIT_EXAMPLE_BODY;         /* Retain original actions for interpretation. */
    const char *parameters[] = {"x"}; /* Give the single argument its local binding. */
    FVM_Config config;                /* Configure the independent runtime. */
    FVM_Runtime *runtime = 0;         /* Allow cleanup after partial initialization. */
    FVM_Context *context = 0;         /* Own breakpoints, code cache and script objects. */
    FVM_Object *function;             /* Invoke through the ordinary activation builder. */
    FVM_Value argument, result;       /* Pass primitive input and receive the answer. */
    FVM_ExecutionStats before, after; /* Check actual native/interpreted dispatch. */
    unsigned int hits = 0;            /* Keep callback state alive until context cleanup. */
    if (fvm_config_init(&config) != FVM_OK)
        goto failed; /* Validate configuration creation. */
    if (fvm_runtime_create(&config, &runtime) != FVM_OK)
        goto failed; /* Own the runtime. */
    if (fvm_context_create(runtime, &context) != FVM_OK)
        goto failed; /* Own its context. */
    if (fvm_function_create_avm1(context, body, sizeof(body), 1, parameters, 0, &function) !=
        FVM_OK)
        goto failed; /* Create an anonymous script body. */
    if (fvm_execution_set_mode(context, FVM_EXECUTION_JIT, 0) != FVM_OK)
        goto failed;                                    /* Request automatic JIT. */
    fvm_value_number(&argument, FVM_JIT_EXAMPLE_INPUT); /* Supply x=20. */
    if (fvm_function_invoke(context, function, 0, 1, &argument, &result) != FVM_OK)
        goto failed;                     /* Warm the native cache when supported. */
    before.struct_size = sizeof(before); /* Initialize the statistics ABI size. */
    if (fvm_execution_get_stats(context, &before) != FVM_OK)
        goto failed; /* Save the original native count. */
    if (fvm_debug_map_action(context, "<function>", 0, "compute.as", 1) != FVM_OK)
        goto failed; /* The interpreter names anonymous C-created bodies <function>. */
    if (fvm_debug_set_breakpoint_handler(context, stopped, &hits) != FVM_OK)
        goto failed; /* Install host-owned inspection state. */
    if (fvm_debug_set_breakpoint(context, "<function>", 0, 1) != FVM_OK)
        goto failed; /* Exclude this name from native entry. */
    if (fvm_function_invoke(context, function, 0, 1, &argument, &result) != FVM_OK)
        goto failed;                   /* Interpret the entire stopped body. */
    after.struct_size = sizeof(after); /* Initialize the second statistics record. */
    if (fvm_execution_get_stats(context, &after) != FVM_OK)
        goto failed; /* Observe actual dispatch. */
    if (hits != 1 || after.native_entries != before.native_entries ||
        result.number != FVM_JIT_EXAMPLE_EXPECTED)
        goto failed; /* Check breakpoint and result parity. */
    if (fvm_debug_set_breakpoint(context, "<function>", 0, 0) != FVM_OK)
        goto failed; /* Re-enable native entry on the next call. */
    if (fvm_function_invoke(context, function, 0, 1, &argument, &result) != FVM_OK)
        goto failed; /* Reuse the cached code on supported hosts. */
    printf("%s: breakpoint interpreted, result=%.0f\n", fvm_jit_backend_name(),
           result.number);        /* Report the selected backend. */
    fvm_context_destroy(context); /* Release context-owned cache and debugger state. */
    fvm_runtime_destroy(runtime); /* Finish the runtime lifetime. */
    return 0;                     /* Report success without depending on assertions. */
failed:                           /* Unwind any partially created resources. */
    fputs("JIT debugger example failed\n", stderr); /* Keep release-mode failures visible. */
    if (context)
        fvm_context_destroy(context); /* Release a created context first. */
    if (runtime)
        fvm_runtime_destroy(runtime); /* Then release its runtime. */
    return 1;                         /* Report failure to the caller. */
} /* Finish the example. */
