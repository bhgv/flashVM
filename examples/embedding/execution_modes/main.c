/* Tutorial: entry options, runtime switching and opt-in interpreter replay with default logs. */
#include "flashvm/execution_mode.h" /* Select policy and inspect replay counters. */
#include "flashvm/standard.h"       /* Install script objects and valueOf methods. */
#include "flashvm/exception.h"      /* Raise and clear the original diagnostic exception. */
#include "../jit/body.h"            /* Reuse the documented bytecode function x*2+2. */
#include "constants.h"              /* Keep the demonstration's thrown value named. */
#include <stdio.h>                  /* Show results independently from diagnostic stderr logging. */
static unsigned conversions; /* Count observable side effects across original/replayed calls. */
/* Fail only the first valueOf call, demonstrating why replay must be explicitly enabled. */
static FVM_Result number(FVM_Context *context, const FVM_ScriptFrame *frame, FVM_Value *out,
                         void *data) /* Match the prepared native-body ABI. */
{                                    /* Begin the trusted script-body bridge. */
    (void)frame;
    (void)data;    /* This example does not inspect activation fields or host state. */
    ++conversions; /* Replay repeats this side effect; it does not restore application state. */
    fvm_value_number(out,
                     conversions == 1
                         ? FVM_EXECUTION_EXAMPLE_THROW
                         : FVM_JIT_EXAMPLE_INPUT); /* Return an error marker first, then x=20. */
    return conversions == 1 ? fvm_exception_raise(context, out)
                            : FVM_OK; /* Preserve normal exception/status conventions. */
} /* End the bridge. */
/* Run both modes through the ordinary entry API, then request diagnostic replay. */
int main(
    void) /* The host owns the runtime and context, while automatic code belongs to the context. */
{         /* Begin the embedding lifetime. */
    const unsigned char body[] = FVM_JIT_EXAMPLE_BODY; /* Keep one source body for both modes. */
    const unsigned char empty[] = {0}; /* A bound script bridge retains valid fallback bytecode. */
    const char *parameters[] = {
        "x"}; /* Prepare one named argument through the common activation builder. */
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;               /* Own embedding resources. */
    FVM_Object *function, *object, *method; /* Retain callable and coercible script objects. */
    FVM_Value argument, result, value,
        exception; /* Pass ordinary ABI values and preserve the throw. */
    FVM_ExecutionOptions options;
    FVM_ExecutionStats stats; /* Keep ABI-sized mode/statistics records. */
    if (!(fvm_config_init(&config) == FVM_OK))
        goto failed; /* Start from the default embedding configuration. */
    if (!(fvm_runtime_create(&config, &runtime) == FVM_OK))
        goto failed; /* Allocate the runtime. */
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
        goto failed; /* Allocate one independent execution policy/cache. */
    if (!(fvm_standard_install(context) == FVM_OK))
        goto failed; /* Install standard script objects. */
    if (!(fvm_function_create_avm1(context, body, sizeof(body), 1, parameters, 0, &function) ==
          FVM_OK))
        goto failed; /* Preserve bytecode for interpreter execution/replay. */
    if (!(fvm_execution_options_init(&options) == FVM_OK))
        goto failed; /* Initialize interpreter mode and disable replay. */
    fvm_value_number(&argument, FVM_JIT_EXAMPLE_INPUT); /* Set x=20. */
    if (!(fvm_function_invoke_options(context, function, 0, 1, &argument, &result, &options) ==
          FVM_OK))
        goto failed;                              /* Run the interpreter. */
    printf("interpreter: %.0f\n", result.number); /* Display 42. */
    printf("backend: %s\n",
           fvm_jit_backend_name());   /* Report the build architecture without host guesses. */
    options.mode = FVM_EXECUTION_JIT; /* Select automatic compilation with interpreter fallback. */
    if (!(fvm_function_invoke_options(context, function, 0, 1, &argument, &result, &options) ==
          FVM_OK))
        goto failed; /* Reuse the same activation contract. */
    printf("JIT/fallback: %.0f\n",
           result.number); /* Display the same answer on every supported host. */
    if (!(fvm_execution_set_mode(context, FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
        goto failed;         /* This setter is also callable while another frame is running. */
    if (fvm_jit_available()) /* Exercise native-exception replay only on the implemented backend. */
    {                        /* Begin the optional diagnostic scenario. */
        if (!(fvm_object_create(context, &object) == FVM_OK))
            goto failed; /* Create an argument whose numeric conversion has effects. */
        if (!(fvm_function_create_avm1(context, empty, sizeof(empty), 0, 0, 0, &method) == FVM_OK))
            goto failed; /* Allocate a prepared script callback. */
        if (!(fvm_function_set_body(context, method, number, 0, 0) == FVM_OK))
            goto failed; /* Supply a trusted valueOf implementation. */
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = method; /* Form the method value. */
        if (!(fvm_object_set(context, object, "valueOf", &value) == FVM_OK))
            goto failed; /* Publish the callback on the argument. */
        argument = value;
        argument.object = object; /* Pass the coercible argument. */
        options.mode = FVM_EXECUTION_JIT;
        options.flags = FVM_EXEC_REPLAY_ON_EXCEPTION; /* Explicitly authorize repeated application
                                                         effects for diagnostics. */
        if (!(fvm_function_invoke_options(context, function, 0, 1, &argument, &result, &options) ==
              FVM_EXCEPTION))
            goto failed; /* Retry interprets successfully but returns the original native exception.
                          */
        if (!(fvm_exception_get(context, &exception) == FVM_OK &&
              exception.number == FVM_EXECUTION_EXAMPLE_THROW))
            goto failed;                   /* Verify the original throw was not hidden. */
        stats.struct_size = sizeof(stats); /* Initialize the statistics ABI size. */
        if (!(fvm_execution_get_stats(context, &stats) == FVM_OK))
            goto failed; /* Read replay counts and result status. */
        printf("original exception: %.0f; replays=%lu; steps=%lu; replay status=%d\n",
               exception.number, stats.replays, stats.replay_steps,
               (int)stats.last_replay_status); /* Show diagnostic evidence. */
        if (!(fvm_exception_clear(context) == FVM_OK))
            goto failed; /* Clear the preserved exception before normal host continuation. */
    } /* Finish optional diagnostics. */
    fvm_context_destroy(
        context); /* Release managed objects and every automatic cached code mapping. */
    fvm_runtime_destroy(runtime); /* Release the runtime after context shutdown. */
    return 0;                     /* Report successful execution and diagnostics. */
failed: /* Release partially initialized resources on every API failure. */
    fputs("Execution mode tutorial: operation failed\n", stderr); /* Keep errors visible. */
    if (context)
        fvm_context_destroy(context); /* Release context-owned code and objects. */
    if (runtime)
        fvm_runtime_destroy(runtime); /* Finish the enclosing runtime lifetime. */
    return 1;                         /* Report an unsuccessful demonstration. */
} /* Finish the embedding lifetime. */
