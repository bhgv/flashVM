/* Nested legacy frame calls share one limit and preserve completed inner effects on failure. */
#include <stdio.h>
#include <stdlib.h>
#include "frame_blocks_fixture.hpp"


int main()
{
    FVM_Config cfg;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < 2; ++mode)
    {
        for (unsigned long limit = 10; limit <= 11; ++limit)
        {
            if (mode && !fvm_jit_available())
            {
                continue;
            }
            FVM_Runtime *rt = 0;
            cfg.instruction_limit = limit;
            if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Context *c = tick_context(rt);
            FVM_Object *clip = tick_root(c, 2);
            Bytes outer = frame_call(2), inner = action(2);
            if (!(fvm_movieclip_define_frame(c, clip, 1, 0, outer.begin(), outer.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_movieclip_define_frame(c, clip, 2, 0, inner.begin(), inner.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_execution_set_mode(c, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(run_frame(c, clip, 0) == (limit == 10 ? FVM_ERROR : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->avm1_steps == 11))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, clip, "seen", 2);
            tick_clean(c);
            if (mode)
            {
                if (!(c->execution_mode.stats.native_entries > 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_context_destroy(c);
            fvm_runtime_destroy(rt);
        }
    }
}
