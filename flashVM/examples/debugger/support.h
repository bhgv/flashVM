/* Shared C89 setup for the debugger tutorials; every API failure is handled explicitly. */
#ifndef FVM_DEBUGGER_EXAMPLE_SUPPORT_H /* Keep the helper definitions unique in each translation unit. */
#define FVM_DEBUGGER_EXAMPLE_SUPPORT_H /* Mark this tutorial helper header as included. */
#include "flashvm/standard.h"          /* Declare runtime, context and standard-library installation APIs. */
#include <stdio.h>                     /* Provide the host diagnostic stream used by all examples. */


/* Report an ordinary API result and let the caller choose its cleanup path. */
static int debugger_ok(FVM_Result result, const char *operation) /* Borrow the operation label for this call. */
{                                                                /* Begin the result checker without changing VM state. */
    if (result == FVM_OK)                                        /* Successful operations require no error output. */
    {                                                            /* Begin conditional branch: Successful operations require no error output. */
        return 1;                                                /* Tell the caller that it may continue. */
    } /* End conditional branch: Successful operations require no error output. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Preserve the actual numeric result in the diagnostic. */
    return 0;                                                   /* Let the caller leave through its explicit cleanup code. */
} /* Finish the checker; it never terminates the host process itself. */


/* Construct an isolated runtime/context and install the standard object classes. */
static int debugger_open(FVM_Runtime **runtime, FVM_Context **context) /* Return ownership through the two output slots. */
{                                                                      /* Begin the setup helper. */
    FVM_Config config;                                                 /* Keep the versioned runtime configuration on the host stack. */
    *runtime = 0;                                                      /* Make cleanup safe if configuration or allocation fails. */
    *context = 0;                                                      /* No context is owned until its creation succeeds. */
    if (!debugger_ok(fvm_config_init(&config), "config_init"))         /* Initialize ABI size and default limits. */
    {                                                                  /* Begin conditional branch: Initialize ABI size and default limits. */
        return 0;                                                      /* Leave both ownership outputs empty on configuration failure. */
    } /* End conditional branch: Initialize ABI size and default limits. */
    if (!debugger_ok(fvm_runtime_create(&config, runtime), "runtime_create")) /* Acquire the runtime before any context. */
    {                                                                         /* Begin conditional branch: Acquire the runtime before any context. */
        return 0;                                                             /* The caller still owns no runtime after creation failure. */
    } /* End conditional branch: Acquire the runtime before any context. */
    if (!debugger_ok(fvm_context_create(*runtime, context), "context_create")) /* Create one VM on the current host thread. */
    {                                                                          /* Begin conditional branch: Create one VM on the current host thread. */
        return 0;                                                              /* The caller releases the already-created runtime. */
    } /* End conditional branch: Create one VM on the current host thread. */
    if (!debugger_ok(fvm_standard_install(*context), "standard_install")) /* Make Object, Array and other builtin classes inspectable. */
    {                                                                     /* Begin conditional branch: Make Object, Array and other builtin classes inspectable. */
        return 0;                                                         /* The caller releases the partially initialized context and runtime. */
    } /* End conditional branch: Make Object, Array and other builtin classes inspectable. */
    return 1; /* Transfer both successful handles to the example. */
} /* Finish setup without hiding partial ownership from the caller. */


/* Release the context before its runtime after all synchronous calls have returned. */
static void debugger_close(FVM_Runtime *runtime, FVM_Context *context) /* Accept the handles owned by debugger_open. */
{                                                                      /* Begin orderly host cleanup. */
    if (context)                                                       /* A failed setup may not have created a context. */
    {                                                                  /* Begin conditional branch: A failed setup may not have created a context. */
        fvm_context_destroy(context);                                  /* Release managed values and copied diagnostic reports first. */
    } /* End conditional branch: A failed setup may not have created a context. */
    if (runtime)                      /* The runtime can also be absent after an early failure. */
    {                                 /* Begin conditional branch: The runtime can also be absent after an early failure. */
        fvm_runtime_destroy(runtime); /* Release runtime ownership after its context is gone. */
    } /* End conditional branch: The runtime can also be absent after an early failure. */
} /* Finish the shared tutorial cleanup helper. */
#endif /* End the tutorial helper include guard. */
