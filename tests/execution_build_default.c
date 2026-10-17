/* Build-selected context defaults coexist with explicit options and fallback. */
#include "flashvm/execution_mode.h"
#include "flashvm/standard.h"
#include "flashvm/serialization.h"
#include <assert.h>

static void check_mode(FVM_Context *context, FVM_ExecutionMode expected)
{
    FVM_ExecutionOptions options;
    options.struct_size = sizeof(options);
    assert(fvm_execution_get_options(context, &options) == FVM_OK);
    assert(options.mode == expected && options.flags == 0);
}

int main(void)
{
    const unsigned char body[] = {0x96,5,0,7,42,0,0,0,0x3e,0};
    /* Unsupported, unreachable ToggleQuality forces whole-body fallback. */
    const unsigned char fallback[] = {0x99,2,0,1,0,0x08,0x96,5,0,7,42,0,0,0,0x3e,0};
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *first, *second;
    FVM_Object *function, *unsupported;
    FVM_Value result;
    FVM_ExecutionOptions options;
    FVM_ExecutionStats stats;
    FVM_ExecutionMode initial;
    unsigned long native;
#if defined(JIT) && JIT
    initial = FVM_EXECUTION_JIT;
#else
    initial = FVM_EXECUTION_MANUAL;
#endif
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &first) == FVM_OK);
    assert(fvm_context_create(runtime, &second) == FVM_OK);
    check_mode(first, initial);
    check_mode(second, initial);
    assert(fvm_standard_install(first) == FVM_OK);
    assert(fvm_function_create_avm1(first, body, sizeof(body), 0, 0, 0, &function) == FVM_OK);
    assert(fvm_function_invoke(first, function, 0, 0, 0, &result) == FVM_OK && result.number == 42);
    assert(fvm_function_invoke_options(first, function, 0, 0, 0, &result, 0) == FVM_OK && result.number == 42);
    stats.struct_size = sizeof(stats);
    assert(fvm_execution_get_stats(first, &stats) == FVM_OK);
    native = initial == FVM_EXECUTION_JIT && fvm_jit_available() ? 2 : 0;
    assert(stats.native_entries == native);
    if (native)
        assert(stats.compiled_bodies == 1 && stats.cache_hits == 1);
    assert(fvm_function_create_avm1(first, fallback, sizeof(fallback), 0, 0, 0, &unsupported) == FVM_OK);
    assert(fvm_function_invoke(first, unsupported, 0, 0, 0, &result) == FVM_OK && result.number == 42);
    assert(fvm_execution_get_stats(first, &stats) == FVM_OK && stats.native_entries == native);
    if (initial == FVM_EXECUTION_JIT)
        assert(stats.fallbacks != 0);
    /* Explicit initialization remains interpreter-first in every build profile. */
    assert(fvm_execution_options_init(&options) == FVM_OK && options.mode == FVM_EXECUTION_INTERPRETER);
    assert(fvm_function_invoke_options(first, function, 0, 0, 0, &result, &options) == FVM_OK && result.number == 42);
    check_mode(first, FVM_EXECUTION_INTERPRETER);
    assert(fvm_execution_get_stats(first, &stats) == FVM_OK && stats.native_entries == native);
    check_mode(second, initial);
    assert(fvm_execution_set_mode(first, FVM_EXECUTION_MANUAL, 0) == FVM_OK);
    check_mode(first, FVM_EXECUTION_MANUAL);
    assert(fvm_execution_set_mode(first, FVM_EXECUTION_JIT, 0) == FVM_OK);
    assert(fvm_function_invoke(first, function, 0, 0, 0, &result) == FVM_OK && result.number == 42);
    check_mode(second, initial);
    /* Transfer source graphs, never the sender's context execution policy/cache. */
    {
        unsigned char *bytes;
        size_t size;
        FVM_Value source, restored;
        assert(fvm_execution_set_mode(first, initial == FVM_EXECUTION_JIT
            ? FVM_EXECUTION_INTERPRETER : FVM_EXECUTION_JIT, 0) == FVM_OK);
        fvm_value_undefined(&source);
        source.type = FVM_VALUE_OBJECT;
        source.object = function;
        assert(fvm_value_serialize(first, &source, &bytes, &size) == FVM_OK);
        assert(fvm_standard_install(second) == FVM_OK);
        assert(fvm_value_deserialize(second, bytes, size, &restored) == FVM_OK);
        check_mode(second, initial);
        assert(fvm_function_invoke(second, restored.object, 0, 0, 0, &result) == FVM_OK && result.number == 42);
        check_mode(second, initial);
        assert(fvm_execution_get_stats(second, &stats) == FVM_OK);
        assert(stats.native_entries == (initial == FVM_EXECUTION_JIT && fvm_jit_available() ? 1UL : 0UL));
        fvm_serialized_free(bytes);
    }
    fvm_context_destroy(second);
    fvm_context_destroy(first);
    fvm_runtime_destroy(runtime);
    return 0;
}
