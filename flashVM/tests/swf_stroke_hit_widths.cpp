#include <stdio.h>
#include <stdlib.h>
#include "swf_stroke_hit_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned widths[] = {0, 1, 20, 80, 65535};
    for (unsigned type = 0; type < 4; ++type)
    {
        for (unsigned cap = 0; cap < 3; ++cap)
        {
            for (unsigned join = 0; join < 2; ++join)
            {
                for (unsigned w = 0; w < 5; ++w)
                {
                    FVM_Object *root = load(c, bs_tags(caps_shape(type, cap, join, widths[w])), 1);
                    tick(c, root);
                    double radius = (widths[w] ? widths[w] / 20.0 : 1) / 2;
                    stroke_hit(c, root, 8, 8, 1);
                    stroke_hit(c, root, 8, 8 + radius + .01, 0);
                    stroke_hit(c, root, 2 - radius * .5, 8, cap != 1);
                    stroke_hit(c, root, 2 - radius * .95, 8 - radius * .95, cap == 2);
                    if (!(fvm_gc_unpin(c, root) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
