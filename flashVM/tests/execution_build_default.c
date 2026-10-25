/* Build-selected context defaults coexist with explicit options and fallback. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution_mode.h"
#include "flashvm/standard.h"
#include "flashvm/serialization.h"


static void check_mode(FVM_Context *context, FVM_ExecutionMode expected)
{
    FVM_ExecutionOptions options;
    options.struct_size = sizeof(options);
    if (!(fvm_execution_get_options(context, &options) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(options.mode == expected && options.flags == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    const unsigned char body[] = {0x96, 5, 0, 7, 42, 0, 0, 0, 0x3e, 0};
    /* Unsupported, unreachable reserved opcode 0x16 forces whole-body fallback. */
    const unsigned char fallback[] = {0x99, 2, 0, 1, 0, 0x16, 0x96, 5, 0, 7, 42, 0, 0, 0, 0x3e, 0};
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
    if (!(fvm_config_init(&config) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &first) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &second) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    check_mode(first, initial);
    check_mode(second, initial);
    if (!(fvm_standard_install(first) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(first, body, sizeof(body), 0, 0, 0, &function) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_invoke(first, function, 0, 0, 0, &result) == FVM_OK && result.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_invoke_options(first, function, 0, 0, 0, &result, 0) == FVM_OK && result.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    stats.struct_size = sizeof(stats);
    if (!(fvm_execution_get_stats(first, &stats) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    native = initial == FVM_EXECUTION_JIT && fvm_jit_available() ? 2 : 0;
    if (!(stats.native_entries == native))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (native)
    {
        if (!(stats.compiled_bodies == 1 && stats.cache_hits == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_function_create_avm1(first, fallback, sizeof(fallback), 0, 0, 0, &unsupported) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_invoke(first, unsupported, 0, 0, 0, &result) == FVM_OK && result.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_execution_get_stats(first, &stats) == FVM_OK && stats.native_entries == native))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (initial == FVM_EXECUTION_JIT)
    {
        if (!(stats.fallbacks != 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Explicit initialization remains interpreter-first in every build profile. */
    if (!(fvm_execution_options_init(&options) == FVM_OK && options.mode == FVM_EXECUTION_INTERPRETER))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_invoke_options(first, function, 0, 0, 0, &result, &options) == FVM_OK && result.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    check_mode(first, FVM_EXECUTION_INTERPRETER);
    if (!(fvm_execution_get_stats(first, &stats) == FVM_OK && stats.native_entries == native))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    check_mode(second, initial);
    if (!(fvm_execution_set_mode(first, FVM_EXECUTION_MANUAL, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    check_mode(first, FVM_EXECUTION_MANUAL);
    if (!(fvm_execution_set_mode(first, FVM_EXECUTION_JIT, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_invoke(first, function, 0, 0, 0, &result) == FVM_OK && result.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    check_mode(second, initial);
    /* Transfer source graphs, never the sender's context execution policy/cache. */
    {
        unsigned char *bytes;
        size_t size;
        FVM_Value source, restored;
        if (!(fvm_execution_set_mode(
                  first, initial == FVM_EXECUTION_JIT ? FVM_EXECUTION_INTERPRETER : FVM_EXECUTION_JIT, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&source);
        source.type = FVM_VALUE_OBJECT;
        source.object = function;
        if (!(fvm_value_serialize(first, &source, &bytes, &size) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(second) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_deserialize(second, bytes, size, &restored) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        check_mode(second, initial);
        if (!(fvm_function_invoke(second, restored.object, 0, 0, 0, &result) == FVM_OK && result.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        check_mode(second, initial);
        if (!(fvm_execution_get_stats(second, &stats) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(stats.native_entries == (initial == FVM_EXECUTION_JIT && fvm_jit_available() ? 1UL : 0UL)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(bytes);
    }
    fvm_context_destroy(second);
    fvm_context_destroy(first);
    fvm_runtime_destroy(runtime);
    return 0;
}
