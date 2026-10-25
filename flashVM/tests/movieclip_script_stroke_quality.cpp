#include <stdio.h>
#include <stdlib.h>
#include "script_stroke_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ss_context(rt, &c);
    for (unsigned q = 0; q < 4; ++q)
    {
        for (unsigned kind = 0; kind < 4; ++kind)
        {
            for (unsigned cap = 0; cap < 3; ++cap)
            {
                for (unsigned join = 0; join < 2; ++join)
                {
                    if (!(fvm_stage_set_quality(c, (FVM_StageQuality)q) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *clip = ss_clip(c, cap, join, kind);
                    ss_pixels(c, clip, cap, join, kind);
                }
            }
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
