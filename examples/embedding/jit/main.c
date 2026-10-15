/* Tutorial: use the interpreter, explicitly compile a supported body, then restore interpretation.
 */
#include "flashvm/jit.h"      /* Compile and attach a host-owned native code handle. */
#include "flashvm/standard.h" /* Install script standard objects before creating functions. */
#include "body.h"  /* Keep encoded instructions and tutorial values named in a private header. */
#include <stdio.h> /* Show actual interpreter and native results. */
/* Run a complete C89 embedding example with a usable interpreter fallback. */
int main(void) /* The host owns every runtime, context and compiled handle below. */
{              /* Begin the host lifetime. */
    const unsigned char body[] = FVM_JIT_EXAMPLE_BODY; /* Same source for both execution paths. */
    const char *parameters[] = {"x"}; /* Bind the actual argument to a named local. */
    FVM_Config config;        /* Initialize this ABI-sized embedding configuration before use. */
    FVM_Runtime *runtime = 0; /* Own the VM runtime until the last context is destroyed. */
    FVM_Context *context = 0; /* Own this function's object graph and activation scopes. */
    FVM_Object *function;     /* Retain the function through the ordinary invocation API. */
    FVM_JitCode *code = 0; /* Keep native code alive until every function attachment is removed. */
    FVM_Value argument, result; /* Pass and receive ordinary managed ABI values. */
    FVM_Result status;          /* An unsupported compilation leaves the interpreter available. */
    if (!(fvm_config_init(&config) == FVM_OK))
        goto failed; /* Select the normal interpreter configuration. */
    if (!(fvm_runtime_create(&config, &runtime) == FVM_OK))
        goto failed; /* Create the embedding runtime. */
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
        goto failed; /* Allocate one independent context. */
    if (!(fvm_standard_install(context) == FVM_OK))
        goto failed; /* Install the script standard library. */
    if (!(fvm_function_create_avm1(context, body, sizeof(body), 1, parameters, 0, &function) ==
          FVM_OK))
        goto failed; /* Keep original bytecode for fallback. */
    fvm_value_number(&argument,
                     FVM_JIT_EXAMPLE_INPUT); /* Supply x through normal call preparation. */
    if (!(fvm_function_invoke(context, function, 0, 1, &argument, &result) == FVM_OK))
        goto failed; /* Execute with the interpreter first. */
    if (!(result.number == FVM_JIT_EXAMPLE_EXPECTED))
        goto failed;                              /* Verify the independent expected answer. */
    printf("interpreter: %.0f\n", result.number); /* Display the interpreted result. */
    status = fvm_jit_compile(body, sizeof(body),
                             &code); /* Decode and emit the supported native subset. */
    if (status == FVM_OK) /* Attach only after compilation and executable-memory sealing succeed. */
    {                     /* Begin the optional native lifetime. */
        if (!(fvm_function_set_body(context, function, fvm_jit_body, code, 0) == FVM_OK))
            goto failed; /* Reuse the prepared script activation ABI. */
        if (!(fvm_function_invoke(context, function, 0, 1, &argument, &result) == FVM_OK))
            goto failed; /* Enter emitted machine code through rooted helpers. */
        if (!(result.number == FVM_JIT_EXAMPLE_EXPECTED))
            goto failed; /* Check parity with the interpreter. */
        printf("native: %.0f (%lu code bytes)\n", result.number,
               (unsigned long)fvm_jit_code_size(code)); /* Report actual emitted code. */
        if (!(fvm_function_set_body(context, function, 0, 0, 0) == FVM_OK))
            goto failed; /* Restore bytecode execution before freeing code. */
        if (!(fvm_jit_destroy(code) == FVM_OK))
            goto failed; /* Release the inactive executable mapping. */
        code = 0;        /* Mark the native handle as released before error cleanup. */
    } /* Finish the optional native lifetime. */
    else /* Unsupported targets and bodies can continue with their original bytecode. */
    {    /* Handle the documented fallback without silently accepting unrelated errors. */
        if (!(status == FVM_UNSUPPORTED))
            goto failed; /* Other compiler errors require host attention. */
        puts("native backend unavailable; interpreter retained"); /* Explain the selected path. */
    } /* Finish compiler fallback handling. */
    fvm_context_destroy(context); /* Release functions, activations and managed values. */
    fvm_runtime_destroy(runtime); /* Release the runtime after all contexts are gone. */
    return 0;                     /* Signal a successful interpreter/native demonstration. */
failed: /* Report errors even when assertions are disabled in a release host. */
    fputs("JIT tutorial: operation failed\n", stderr); /* Identify the failing example. */
    if (context)
        fvm_context_destroy(context); /* Drop all attachments before releasing code. */
    if (runtime)
        fvm_runtime_destroy(runtime); /* Release a successfully created runtime. */
    if (code)
        fvm_jit_destroy(code); /* No function can reference this inactive code now. */
    return 1;                  /* Signal failure to the host or test runner. */
} /* Finish the host lifetime. */
