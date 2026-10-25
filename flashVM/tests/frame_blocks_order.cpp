/* Reinserted ordinal tables execute in authored index order on direct/service/tick/interpreter/JIT routes. */
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
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (!version || version >= 5)
        {
            for (unsigned route = 0; route <= 4; ++route)
            {
                if (route == 3 && !fvm_jit_available())
                {
                    continue;
                }
                FVM_Context *c = tick_context(rt, version);
                FVM_Object *clip = tick_root(c), *segments = frame_segments(c, clip);
                segments->indexed[2] = frame_hex(c, action(3));
                segments->indexed[0] = frame_hex(c, action(1));
                segments->indexed[1] = frame_hex(c, action(2));
                if (!(run_frame(c, clip, route) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                number(c, clip, "seen", 3);
                number(c, clip, "_currentframe", 1);
                tick_clean(c);
                if (route == 3)
                {
                    if (!(c->execution_mode.stats.native_entries > 0))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
