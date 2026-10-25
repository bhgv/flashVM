/* Fully consumed init tables contribute no copying units to root/child ticks, interpreted or native JIT. */
#include <stdio.h>
#include <stdlib.h>
#include "initializer_progress_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 6; version <= 8; ++version)
    {
        for (unsigned mode = 0; mode < 2; ++mode)
        {
            for (unsigned child_first = 0; child_first < 2; ++child_first)
            {
                if (mode && !fvm_jit_available())
                {
                    continue;
                }
                FVM_Context *c = tick_context(rt, version);
                FVM_Object *root = init_root(c, version, 3);
                InitProbe p;
                init_bind(c, root, p);
                if (!(fvm_execution_set_mode(c, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *target = root;
                if (child_first)
                {
                    target = tick_child(c, root, "child", 1);
                }
                rt->config.instruction_limit = 0;
                if (!(fvm_movieclip_advance(c, target, 1) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(p.calls[0] == 1 && p.calls[1] == 1 && p.calls[2] == 1 && p.library->swf_initializer_next == 3))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (mode)
                {
                    if (!(c->execution_mode.stats.native_entries > 0))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                rt->config.instruction_limit = 2;
                if (!(fvm_movieclip_advance(c, target, 1) == FVM_OK && c->avm1_steps == 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(p.calls[0] == 1 && p.calls[1] == 1 && p.calls[2] == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                init_clean(c, p);
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
