/* Empty DoAction blocks still charge capture/entry; no AVM1 instruction is needed to fail copying. */
#include <stdio.h>
#include <stdlib.h>
#include "frame_blocks_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Context *c = tick_context(rt);
    FVM_Object *clip = tick_root(c), *segments = frame_segments(c, clip);
    FVM_Value empty;
    if (!(fvm_value_string(&empty, "") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 40; ++i)
    {
        segments->indexed[i] = empty;
    }
    for (unsigned long limit = 1; limit <= 40; ++limit)
    {
        rt->config.instruction_limit = limit;
        if (!(run_frame(c, clip, 0) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->avm1_steps == limit))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
    }
    rt->config.instruction_limit = 0;
    if (!(run_frame(c, clip, 0) == FVM_OK && c->avm1_steps == 120))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
