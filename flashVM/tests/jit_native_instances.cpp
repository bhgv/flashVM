/* Native backend acceptance: independent VM owners, source lifetime and actual entry counters. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/jit.h"
#include "flashvm/standard.h"
#include "flashvm/execution_mode.h"
#include <cstring>


static void execute(FVM_Context *context, FVM_Object *function)
{
    FVM_Value value;
    FVM_ExecutionStats stats;
    stats.struct_size = sizeof(stats);
    if (!(fvm_function_invoke(context, function, 0, 0, 0, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(value.type == FVM_VALUE_NUMBER && value.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_execution_get_stats(context, &stats) == FVM_OK && stats.native_entries))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    if (!fvm_jit_available())
    {
        return 77;
    }
    const unsigned char bytes[] = {0x96, 5, 0, 7, 42, 0, 0, 0, 0x3e, 0};
    FVM_JitCode *code[2] = {0, 0};
    FVM_Result status = fvm_jit_compile(bytes, sizeof(bytes), &code[0]);
    /* The first mapping can legitimately be denied by host signing policy. */
    if (status == FVM_UNSUPPORTED)
    {
        if (!(!code[0]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return 77;
    }
    if (!(status == FVM_OK && code[0]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Two simultaneous bodies must not require a second MAP_JIT region. */
    if (!(fvm_jit_compile(bytes, sizeof(bytes), &code[1]) == FVM_OK && code[1]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Config config;
    if (!(fvm_config_init(&config) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Runtime *runtime[2] = {0, 0};
    FVM_Context *context[2] = {0, 0};
    FVM_Object *function[2] = {0, 0};
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(fvm_runtime_create(&config, &runtime[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_context_create(runtime[i], &context[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(context[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_execution_set_mode(context[i], FVM_EXECUTION_MANUAL, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_avm1(context[i], bytes, sizeof(bytes), 0, 0, 0, &function[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(context[i], function[i], fvm_jit_body, code[i], 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        execute(context[i], function[i]);
    }
    if (!(fvm_function_set_body(context[0], function[0], 0, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_destroy(code[0]) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    code[0] = 0;
    fvm_context_destroy(context[0]);
    fvm_runtime_destroy(runtime[0]);
    /* Teardown of one VM must not invalidate another VM's live native slot or source copy. */
    execute(context[1], function[1]);
    const void *source = 0;
    size_t size = 0;
    if (!(fvm_jit_source(code[1], &source, &size) == FVM_OK && size == sizeof(bytes)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::memcmp(source, bytes, size)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_JitCode *replacement = 0;
    if (!(fvm_jit_compile(bytes, sizeof(bytes), &replacement) == FVM_OK && replacement))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_set_body(context[1], function[1], fvm_jit_body, replacement, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_destroy(code[1]) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    code[1] = 0;
    execute(context[1], function[1]);
    if (!(fvm_function_set_body(context[1], function[1], 0, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_destroy(replacement) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context[1]);
    fvm_runtime_destroy(runtime[1]);
    return 0;
}
