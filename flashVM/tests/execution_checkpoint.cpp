#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/debug.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cstring>
#include <limits>


struct State
{
    FVM_Object *nested;
    unsigned calls;
    int overflow;
};


static FVM_Result generated(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    State *state = (State *)raw;
    ++state->calls;
    if (state->overflow)
    {
        if (!(fvm_exec_checkpoint(c, std::numeric_limits<unsigned long>::max(), 50, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_checkpoint(c, 1, 51, 0) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->avm1_steps == std::numeric_limits<unsigned long>::max()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return FVM_ERROR;
    }
    if (!(fvm_exec_checkpoint(c, 2, 50, FVM_EXEC_POLL) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->debug_line == 7 && c->debug_source == fvm_detail::string("generated.as") && c->debug_offset == 50))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *temporary = 0;
    if (!(fvm_object_create(c, &temporary) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&frame->registers[0]);
    frame->registers[0].type = FVM_VALUE_OBJECT;
    frame->registers[0].object = temporary;
    c->gc_allocations = std::numeric_limits<unsigned long>::max();
    if (!(fvm_exec_checkpoint(c, 0, 50, FVM_EXEC_POLL) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value alive;
    fvm_value_number(&alive, 17);
    if (!(fvm_object_set(c, temporary, "alive", &alive) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_checkpoint(c, 0, 50, 2) == FVM_INVALID_ARGUMENT && c->avm1_steps == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_invoke(c, state->nested, 0, 0, 0, out) == FVM_OK && out->number == 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->avm1_steps == 4 && c->debug_line == 7 && c->debug_offset == 50))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fvm_exec_checkpoint(c, 1, 51, 0);
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cfg.instruction_limit = 4;
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Context *c = 0;
    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned char code[] = {0x96, 5, 0, 7, 7, 0, 0, 0, 0x3e, 0};
    FVM_Object *nested = 0, *fn = 0;
    if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &nested) == FVM_OK &&
          fvm_gc_pin(c, nested) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    State state = {nested, 0, 0};
    if (!(fvm_function_set_body(c, fn, generated, &state, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_map_action(c, "<function>", 50, "generated.as", 7) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_GCRootFrame *roots = c->gc_frames;
    FVM_Value out;
    if (!(fvm_exec_checkpoint(c, 1, 0, FVM_EXEC_POLL) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_ERROR && c->avm1_steps == 5))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->avm1_execution_depth && !c->script_depth && c->gc_frames == roots && !c->avm1_target_active))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_invoke(c, nested, 0, 0, 0, &out) == FVM_OK && c->avm1_steps == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    rt->config.instruction_limit = 0;
    state.overflow = 1;
    if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_ERROR &&
          c->avm1_steps == std::numeric_limits<unsigned long>::max()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->avm1_execution_depth && c->gc_frames == roots))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_invoke(c, nested, 0, 0, 0, &out) == FVM_OK && c->avm1_steps == 2 && state.calls == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
