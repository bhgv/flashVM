/* Head-function options run native root actions and diagnostic replay keeps the original exception/budget. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution_mode.h"
#include "flashvm/standard.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"

static unsigned calls;


static FVM_Result getter(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++calls;
    fvm_value_number(out, calls == 1 ? 111 : 42);
    return calls == 1 ? fvm_exception_raise(c, out) : FVM_OK;
}


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    const unsigned char body[] = {0x96, 3, 0, 0, 'x', 0, 0x1c, 0};
    FVM_ExecutionOptions options;
    FVM_ExecutionStats stats;
    FVM_Value exception;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
          fvm_object_define_accessor_rooted(c, c->global, "x", getter, 0, 0, 0) == FVM_OK &&
          fvm_execution_options_init(&options) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.mode = FVM_EXECUTION_JIT;
    options.flags = FVM_EXEC_REPLAY_ON_EXCEPTION;
    if (!(fvm_avm1_execute_options(c, body, sizeof(body), &options) == FVM_EXCEPTION && calls == 2 &&
          !c->avm1_execution_depth && !c->execution_mode.replay_depth && c->avm1_steps == 2 &&
          fvm_exception_get(c, &exception) == FVM_OK && exception.number == 111))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    stats.struct_size = sizeof(stats);
    if (!(fvm_execution_get_stats(c, &stats) == FVM_OK && stats.native_entries == 1 && stats.interpreter_entries == 1 &&
          stats.replays == 1 && stats.replay_steps == 3 && stats.last_replay_status == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exception_clear(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
