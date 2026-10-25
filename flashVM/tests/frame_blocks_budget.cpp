/* A direct frame call no longer resets the instruction budget between its independent blocks. */
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
    for (unsigned route = 0; route <= 4; ++route)
    {
        for (unsigned long limit = 1; limit <= 25; ++limit)
        {
            if (route == 3 && !fvm_jit_available())
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
            FVM_Object *clip = tick_root(c), *segments = frame_segments(c, clip);
            segments->indexed[0] = frame_hex(c, action(1));
            segments->indexed[1] = frame_hex(c, action(2));
            unsigned long overhead = route == 2 || route == 3 ? 2UL : route == 4 ? 1UL : 0UL;
            /* action(n) has six executed instructions, including End. Two captures + two entries. */
            unsigned long required = 16 + overhead + (route >= 2 ? 1UL : 0UL);
            FVM_Result result = run_frame(c, clip, route);
            if (!(result == (limit < required ? FVM_ERROR : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(result == FVM_OK ? c->avm1_steps == required
                                   : (c->avm1_steps >= limit && c->avm1_steps <= limit + 1)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            tick_clean(c);
            FVM_Value seen;
            if (!(fvm_object_get(c, clip, "seen", &seen) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (limit < overhead + 8)
            {
                if (!(seen.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (result == FVM_OK)
            {
                if (!(seen.number == 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (route < 2 && limit == 10)
            {
                if (!(seen.type == FVM_VALUE_NUMBER && seen.number == 1))
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
