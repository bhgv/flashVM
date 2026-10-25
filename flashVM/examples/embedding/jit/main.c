/* Tutorial: report the compiled ABI, compare interpreter/native results and retain explicit code ownership. */
#include "flashvm/jit.h"            /* Import native availability, compilation and host-owned code lifetime. */
#include "flashvm/execution_mode.h" /* Select manual binding and verify actual native-entry counters. */
#include "flashvm/standard.h"       /* Install ordinary script objects before creating functions. */
#include "body.h"                   /* Keep the independent x*2+2 bytecode and input/output constants together. */
#include <stdio.h>                  /* Report real results and ordinary setup/cleanup errors. */


/* Run the same prepared function through interpretation and an optional native body. */
int main(void)                                                     /* Use only C89 declarations and public embedding APIs. */
{                                                                  /* Begin host-owned runtime, context and native-code lifetimes. */
    const unsigned char body[] = FVM_JIT_EXAMPLE_BODY;             /* Retain original bytecode for both paths. */
    const char *parameters[] = {"x"};                              /* Bind the supplied argument to the named script local. */
    FVM_Config config;                                             /* Initialize the ABI configuration before acquiring owners. */
    FVM_Runtime *runtime = 0;                                      /* Own the runtime after successful creation. */
    FVM_Context *context = 0;                                      /* Own function objects and activations inside this context. */
    FVM_Object *function;                                          /* Borrow the managed function while its context remains alive. */
    FVM_JitCode *code = 0;                                         /* Own compiled storage independently of managed function attachments. */
    FVM_Value argument, result;                                    /* Pass and receive ordinary prepared ABI values. */
    FVM_ExecutionStats stats;                                      /* Inspect native-entry evidence instead of assuming a build default. */
    FVM_Result status;                                             /* Distinguish unsupported compilation from an unrelated failure. */
    printf("backend: %s (compiled: %s)\n", fvm_jit_backend_name(), /* Identify the selected build ABI. */
           fvm_jit_available() ? "yes" : "no");                    /* Availability does not certify OS mapping permission. */
    if (fvm_config_init(&config) != FVM_OK)                        /* Initialize before runtime creation. */
    {                                                              /* Begin conditional branch: Initialize before runtime creation. */
        goto failed;                                               /* No owner exists yet. */
    } /* End conditional branch: Initialize before runtime creation. */
    if (fvm_runtime_create(&config, &runtime) != FVM_OK) /* Acquire the runtime owner. */
    {                                                    /* Begin conditional branch: Acquire the runtime owner. */
        goto failed;                                     /* Release only successfully acquired owners. */
    } /* End conditional branch: Acquire the runtime owner. */
    if (fvm_context_create(runtime, &context) != FVM_OK) /* Acquire the function's context. */
    {                                                    /* Begin conditional branch: Acquire the function's context. */
        goto failed;                                     /* Keep runtime ownership for centralized cleanup. */
    } /* End conditional branch: Acquire the function's context. */
    if (fvm_execution_set_mode(context, FVM_EXECUTION_MANUAL, 0) != FVM_OK) /* Disable automatic compilation before the interpreter comparison. */
    {                                                                       /* Begin conditional branch: Disable automatic compilation before the interpreter comparison. */
        goto failed;                                                        /* Manual mode interprets an unbound function and honors the later explicit attachment. */
    } /* End conditional branch: Disable automatic compilation before the interpreter comparison. */
    stats.struct_size = sizeof(stats);           /* Declare the exact statistics ABI once for subsequent snapshots. */
    if (fvm_standard_install(context) != FVM_OK) /* Install the standard script environment. */
    {                                            /* Begin conditional branch: Install the standard script environment. */
        goto failed;                             /* The context remains owned until cleanup. */
    } /* End conditional branch: Install the standard script environment. */
    if (fvm_function_create_avm1(context, body, sizeof(body),            /* Preserve the original script body. */
                                 1, parameters, 0, &function) != FVM_OK) /* Declare its one named argument. */
    {                                                                    /* Begin scope: Declare its one named argument. */
        goto failed;                                                     /* Do not invoke a partially constructed function. */
    } /* End scope: Declare its one named argument. */
    fvm_value_number(&argument, FVM_JIT_EXAMPLE_INPUT);                             /* Supply x=20 through ordinary argument preparation. */
    if (fvm_function_invoke(context, function, 0, 1, &argument, &result) != FVM_OK) /* Run interpretation first. */
    {                                                                               /* Begin conditional branch: Run interpretation first. */
        goto failed;                                                                /* Preserve actual invocation errors. */
    } /* End conditional branch: Run interpretation first. */
    if (result.type != FVM_VALUE_NUMBER || result.number != FVM_JIT_EXAMPLE_EXPECTED) /* Check the independent result 42. */
    {                                                                                 /* Begin conditional branch: Check the independent result 42. */
        goto failed;                                                                  /* Report a semantic mismatch without tutorial assertions. */
    } /* End conditional branch: Check the independent result 42. */
    if (fvm_execution_get_stats(context, &stats) != FVM_OK || stats.native_entries) /* Prove the first invocation did not enter automatic native code. */
    {                                                                               /* Begin conditional branch: Prove the first invocation did not enter automatic native code. */
        goto failed;                                                                /* Keep the interpreter label truthful even in a JIT-default build. */
    } /* End conditional branch: Prove the first invocation did not enter automatic native code. */
    printf("interpreter: %.0f\n", result.number);                                      /* Display the actual interpreted value. */
    status = fvm_jit_compile(body, sizeof(body), &code);                               /* Publish native code only after complete compilation. */
    if (status == FVM_OK)                                                              /* Enter native mode only after successful mapping and sealing. */
    {                                                                                  /* Keep the code owner alive across attachment and invocation. */
        if (fvm_function_set_body(context, function, fvm_jit_body, code, 0) != FVM_OK) /* Attach the prepared-body adapter. */
        {                                                                              /* Begin conditional branch: Attach the prepared-body adapter. */
            goto failed;                                                               /* Cleanup will drop context attachments before freeing code. */
        } /* End conditional branch: Attach the prepared-body adapter. */
        if (fvm_function_invoke(context, function, 0, 1, &argument, &result) != FVM_OK) /* Run rooted native helpers. */
        {                                                                               /* Begin conditional branch: Run rooted native helpers. */
            goto failed;                                                                /* Preserve native callback/status failures. */
        } /* End conditional branch: Run rooted native helpers. */
        if (result.type != FVM_VALUE_NUMBER || result.number != FVM_JIT_EXAMPLE_EXPECTED) /* Compare with the same oracle. */
        {                                                                                 /* Begin conditional branch: Compare with the same oracle. */
            goto failed;                                                                  /* Report a native/interpreter mismatch normally. */
        } /* End conditional branch: Compare with the same oracle. */
        if (fvm_execution_get_stats(context, &stats) != FVM_OK || !stats.native_entries) /* Confirm an actual native entry after the manual attachment. */
        {                                                                                /* Begin conditional branch: Confirm an actual native entry after the manual attachment. */
            goto failed;                                                                 /* Do not infer execution from successful compilation alone. */
        } /* End conditional branch: Confirm an actual native entry after the manual attachment. */
        printf("native: %.0f (%lu code bytes)\n", result.number,         /* Distinguish actual execution from build availability. */
               (unsigned long)fvm_jit_code_size(code));                  /* Exclude OS page and metadata overhead. */
        if (fvm_function_set_body(context, function, 0, 0, 0) != FVM_OK) /* Detach before destroying native storage. */
        {                                                                /* Begin conditional branch: Detach before destroying native storage. */
            goto failed;                                                 /* Do not free code while a function still references it. */
        } /* End conditional branch: Detach before destroying native storage. */
        if (fvm_jit_destroy(code) != FVM_OK) /* Release an inactive detached native handle. */
        {                                    /* Begin conditional branch: Release an inactive detached native handle. */
            goto failed;                     /* Keep ownership visible on an unexpected release error. */
        } /* End conditional branch: Release an inactive detached native handle. */
        code = 0; /* Prevent cleanup from releasing the completed owner twice. */
    } /* Finish the optional native lifetime. */
    else if (status == FVM_UNSUPPORTED)                           /* Permit the documented original-bytecode fallback. */
    {                                                             /* Explain why no native result is printed. */
        puts("native backend unavailable; interpreter retained"); /* Keep reported execution distinct from compiled ABI support. */
    } /* Finish ordinary unsupported-backend handling. */
    else             /* Allocation, malformed-input and other failures require host attention. */
    {                /* Begin conditional branch: Allocation, malformed-input and other failures require host attention. */
        goto failed; /* Preserve ordinary errors instead of calling them a successful fallback. */
    } /* End conditional branch: Allocation, malformed-input and other failures require host attention. */
    fvm_context_destroy(context);                      /* Release managed functions after all successful detach operations. */
    fvm_runtime_destroy(runtime);                      /* Release runtime ownership after its context. */
    return 0;                                          /* All requested operations and cleanup succeeded. */
failed:                                                /* Preserve dependency order during any partial setup or invocation failure. */
    fputs("JIT tutorial: operation failed\n", stderr); /* Identify the failed executable example. */
    if (context)                                       /* Destroy only an acquired context. */
    {                                                  /* Begin conditional branch: Destroy only an acquired context. */
        fvm_context_destroy(context);                  /* Drop attachments before releasing any remaining code. */
    } /* End conditional branch: Destroy only an acquired context. */
    if (runtime)                      /* Runtime ownership outlives its context. */
    {                                 /* Begin conditional branch: Runtime ownership outlives its context. */
        fvm_runtime_destroy(runtime); /* Release the acquired runtime. */
    } /* End conditional branch: Runtime ownership outlives its context. */
    if (code)                  /* A failed path may still own detached or formerly attached native code. */
    {                          /* Begin conditional branch: A failed path may still own detached or formerly attached native code. */
        fvm_jit_destroy(code); /* No function remains that can enter this code. */
    } /* End conditional branch: A failed path may still own detached or formerly attached native code. */
    return 1; /* Signal ordinary failure without assertions. */
} /* Finish explicit C89 embedding ownership. */
