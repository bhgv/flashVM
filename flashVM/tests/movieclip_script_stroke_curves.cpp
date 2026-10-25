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
    for (unsigned geometry = 1; geometry < 3; ++geometry)
    {
        for (unsigned kind = 0; kind < 4; ++kind)
        {
            for (unsigned cap = 0; cap < 3; ++cap)
            {
                for (unsigned join = 0; join < 2; ++join)
                {
                    FVM_Object *clip = ss_clip(c, cap, join, kind, geometry);
                    ss_pixels(c, clip, cap, join, kind, geometry);
                }
            }
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
