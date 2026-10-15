/* Tutorial: use the interpreter, explicitly compile a supported body, then restore interpretation. */
#include "flashvm/jit.h" /* Compile and attach a host-owned native code handle. */
#include "flashvm/standard.h" /* Install script standard objects before creating functions. */
#include "body.h" /* Keep encoded instructions and tutorial values named in a private header. */
#include <assert.h> /* Make every ownership and execution failure visible in this example. */
#include <stdio.h> /* Show actual interpreter and native results. */
/* Run a complete C89 embedding example with a usable interpreter fallback. */
int main(void) /* The host owns every runtime, context and compiled handle below. */
{ /* Begin the host lifetime. */
    const unsigned char body[] = FVM_JIT_EXAMPLE_BODY; /* Same source for both execution paths. */
    const char *parameters[] = {"x"}; /* Bind the actual argument to a named local. */
    FVM_Config config; /* Initialize this ABI-sized embedding configuration before use. */
    FVM_Runtime *runtime; /* Own the VM runtime until the last context is destroyed. */
    FVM_Context *context; /* Own this function's object graph and activation scopes. */
    FVM_Object *function; /* Retain the function through the ordinary invocation API. */
    FVM_JitCode *code = 0; /* Keep native code alive until every function attachment is removed. */
    FVM_Value argument, result; /* Pass and receive ordinary managed ABI values. */
    FVM_Result status; /* An unsupported compilation leaves the interpreter available. */
    assert(fvm_config_init(&config) == FVM_OK); /* Select the normal interpreter configuration. */
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK); /* Create the embedding runtime. */
    assert(fvm_context_create(runtime, &context) == FVM_OK); /* Allocate one independent context. */
    assert(fvm_standard_install(context) == FVM_OK); /* Install the script standard library. */
    assert(fvm_function_create_avm1(context, body, sizeof(body), 1, parameters, 0, &function) == FVM_OK); /* Keep original bytecode for fallback. */
    fvm_value_number(&argument, FVM_JIT_EXAMPLE_INPUT); /* Supply x through normal call preparation. */
    assert(fvm_function_invoke(context, function, 0, 1, &argument, &result) == FVM_OK); /* Execute with the interpreter first. */
    assert(result.number == FVM_JIT_EXAMPLE_EXPECTED); /* Verify the independent expected answer. */
    printf("interpreter: %.0f\n", result.number); /* Display the interpreted result. */
    status = fvm_jit_compile(body, sizeof(body), &code); /* Decode and emit the supported native subset. */
    if (status == FVM_OK) /* Attach only after compilation and executable-memory sealing succeed. */
    { /* Begin the optional native lifetime. */
        assert(fvm_function_set_body(context, function, fvm_jit_body, code, 0) == FVM_OK); /* Reuse the prepared script activation ABI. */
        assert(fvm_function_invoke(context, function, 0, 1, &argument, &result) == FVM_OK); /* Enter emitted machine code through rooted helpers. */
        assert(result.number == FVM_JIT_EXAMPLE_EXPECTED); /* Check parity with the interpreter. */
        printf("native: %.0f (%lu code bytes)\n", result.number, (unsigned long)fvm_jit_code_size(code)); /* Report actual emitted code. */
        assert(fvm_function_set_body(context, function, 0, 0, 0) == FVM_OK); /* Restore bytecode execution before freeing code. */
        assert(fvm_jit_destroy(code) == FVM_OK); /* Release the inactive executable mapping. */
    } /* Finish the optional native lifetime. */
    else /* Unsupported targets and bodies can continue with their original bytecode. */
    { /* Handle the documented fallback without silently accepting unrelated errors. */
        assert(status == FVM_UNSUPPORTED); /* Other compiler errors require host attention. */
        puts("native backend unavailable; interpreter retained"); /* Explain the selected path. */
    } /* Finish compiler fallback handling. */
    fvm_context_destroy(context); /* Release functions, activations and managed values. */
    fvm_runtime_destroy(runtime); /* Release the runtime after all contexts are gone. */
    return 0; /* Signal a successful interpreter/native demonstration. */
} /* Finish the host lifetime. */
