/* Named record calls without a first-class callee still use function-local this casing in SWF5. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution_mode.h"
#include "flashvm/standard.h"
#include "../src/core/runtime.hpp"


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Value first, second;
    const unsigned char body[] = {0x96, 6, 0, 0, 'T', 'H', 'I', 'S', 0, 0x1c, 0x3e, 0};
    unsigned char swf[] = {'F', 'W', 'S', 5, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
          fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_ScriptFunction fn;
    fn.name = "legacy";
    for (size_t i = 0; i < sizeof(body); ++i)
    {
        fn.body.push_back(body[i]);
    }
    c->script_functions["legacy"] = fn;
    if (!(fvm_execution_set_mode(c, FVM_EXECUTION_INTERPRETER, 0) == FVM_OK &&
          fvm_avm1_call(c, "legacy", 0, 0, 0, &first) == FVM_OK && first.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_execution_set_mode(c, FVM_EXECUTION_JIT, 0) == FVM_OK &&
          fvm_avm1_call(c, "legacy", 0, 0, 0, &second) == FVM_OK && second.type == first.type))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_ExecutionStats stats;
    stats.struct_size = sizeof(stats);
    if (!(fvm_execution_get_stats(c, &stats) == FVM_OK && stats.native_entries == 1 && stats.interpreter_entries == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
