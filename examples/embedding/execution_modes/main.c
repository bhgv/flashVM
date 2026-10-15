/* Tutorial: entry options, runtime switching and opt-in interpreter replay with default logs. */
#include "flashvm/execution_mode.h" /* Select policy and inspect replay counters. */
#include "flashvm/standard.h" /* Install script objects and valueOf methods. */
#include "flashvm/exception.h" /* Raise and clear the original diagnostic exception. */
#include "../jit/body.h" /* Reuse the documented bytecode function x*2+2. */
#include "constants.h" /* Keep the demonstration's thrown value named. */
#include <assert.h> /* Keep API failures visible in this CMake-built tutorial. */
#include <stdio.h> /* Show results independently from diagnostic stderr logging. */
static unsigned conversions; /* Count observable side effects across original/replayed calls. */
/* Fail only the first valueOf call, demonstrating why replay must be explicitly enabled. */
static FVM_Result number(FVM_Context *context, const FVM_ScriptFrame *frame, FVM_Value *out, void *data) /* Match the prepared native-body ABI. */
{ /* Begin the trusted script-body bridge. */
    (void)frame; (void)data; /* This example does not inspect activation fields or host state. */
    ++conversions; /* Replay repeats this side effect; it does not restore application state. */
    fvm_value_number(out, conversions == 1 ? FVM_EXECUTION_EXAMPLE_THROW : FVM_JIT_EXAMPLE_INPUT); /* Return an error marker first, then x=20. */
    return conversions == 1 ? fvm_exception_raise(context, out) : FVM_OK; /* Preserve normal exception/status conventions. */
} /* End the bridge. */
/* Run both modes through the ordinary entry API, then request diagnostic replay. */
int main(void) /* The host owns the runtime and context, while automatic code belongs to the context. */
{ /* Begin the embedding lifetime. */
    const unsigned char body[] = FVM_JIT_EXAMPLE_BODY; /* Keep one source body for both modes. */
    const unsigned char empty[] = {0}; /* A bound script bridge retains valid fallback bytecode. */
    const char *parameters[] = {"x"}; /* Prepare one named argument through the common activation builder. */
    FVM_Config config; FVM_Runtime *runtime; FVM_Context *context; /* Own embedding resources. */
    FVM_Object *function, *object, *method; /* Retain callable and coercible script objects. */
    FVM_Value argument, result, value, exception; /* Pass ordinary ABI values and preserve the throw. */
    FVM_ExecutionOptions options; FVM_ExecutionStats stats; /* Keep ABI-sized mode/statistics records. */
    assert(fvm_config_init(&config) == FVM_OK); /* Start from the default embedding configuration. */
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK); /* Allocate the runtime. */
    assert(fvm_context_create(runtime, &context) == FVM_OK); /* Allocate one independent execution policy/cache. */
    assert(fvm_standard_install(context) == FVM_OK); /* Install standard script objects. */
    assert(fvm_function_create_avm1(context, body, sizeof(body), 1, parameters, 0, &function) == FVM_OK); /* Preserve bytecode for interpreter execution/replay. */
    assert(fvm_execution_options_init(&options) == FVM_OK); /* Initialize interpreter mode and disable replay. */
    fvm_value_number(&argument, FVM_JIT_EXAMPLE_INPUT); /* Set x=20. */
    assert(fvm_function_invoke_options(context, function, 0, 1, &argument, &result, &options) == FVM_OK); /* Run the interpreter. */
    printf("interpreter: %.0f\n", result.number); /* Display 42. */
    options.mode = FVM_EXECUTION_JIT; /* Select automatic compilation with interpreter fallback. */
    assert(fvm_function_invoke_options(context, function, 0, 1, &argument, &result, &options) == FVM_OK); /* Reuse the same activation contract. */
    printf("JIT/fallback: %.0f\n", result.number); /* Display the same answer on every supported host. */
    assert(fvm_execution_set_mode(context, FVM_EXECUTION_INTERPRETER, 0) == FVM_OK); /* This setter is also callable while another frame is running. */
    if (fvm_jit_available()) /* Exercise native-exception replay only on the implemented backend. */
    { /* Begin the optional diagnostic scenario. */
        assert(fvm_object_create(context, &object) == FVM_OK); /* Create an argument whose numeric conversion has effects. */
        assert(fvm_function_create_avm1(context, empty, sizeof(empty), 0, 0, 0, &method) == FVM_OK); /* Allocate a prepared script callback. */
        assert(fvm_function_set_body(context, method, number, 0, 0) == FVM_OK); /* Supply a trusted valueOf implementation. */
        fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = method; /* Form the method value. */
        assert(fvm_object_set(context, object, "valueOf", &value) == FVM_OK); /* Publish the callback on the argument. */
        argument = value; argument.object = object; /* Pass the coercible argument. */
        options.mode = FVM_EXECUTION_JIT; options.flags = FVM_EXEC_REPLAY_ON_EXCEPTION; /* Explicitly authorize repeated application effects for diagnostics. */
        assert(fvm_function_invoke_options(context, function, 0, 1, &argument, &result, &options) == FVM_EXCEPTION); /* Retry interprets successfully but returns the original native exception. */
        assert(fvm_exception_get(context, &exception) == FVM_OK && exception.number == FVM_EXECUTION_EXAMPLE_THROW); /* Verify the original throw was not hidden. */
        stats.struct_size = sizeof(stats); /* Initialize the statistics ABI size. */
        assert(fvm_execution_get_stats(context, &stats) == FVM_OK); /* Read replay counts and result status. */
        printf("original exception: %.0f; replays=%lu; steps=%lu; replay status=%d\n", exception.number, stats.replays, stats.replay_steps, (int)stats.last_replay_status); /* Show diagnostic evidence. */
        assert(fvm_exception_clear(context) == FVM_OK); /* Clear the preserved exception before normal host continuation. */
    } /* Finish optional diagnostics. */
    fvm_context_destroy(context); /* Release managed objects and every automatic cached code mapping. */
    fvm_runtime_destroy(runtime); /* Release the runtime after context shutdown. */
    return 0; /* Report successful execution and diagnostics. */
} /* Finish the embedding lifetime. */
