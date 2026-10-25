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
    const FVM_Affine transforms[] = {
        {2, 0, 0, 2, 20, 20}, {0, 2, -2, 0, 40, 20}, {-2, 0, 0, 2, 40, 20}, {0, -2, 2, 0, 20, 40}};
    const double points[][2] = {{8, 8}, {8, 10.1}, {1, 8}, {.1, 6.1}};
    for (unsigned type = 0; type < 4; ++type)
    {
        for (unsigned cap = 0; cap < 3; ++cap)
        {
            for (unsigned join = 0; join < 2; ++join)
            {
                FVM_Object *root = load(c, bs_tags(caps_shape(type, cap, join)), 1);
                tick(c, root);
                FVM_Object *node = display_node(root);
                for (unsigned m = 0; m < 4; ++m)
                {
                    const FVM_Affine &a = transforms[m];
                    if (!(fvm_movieclip_set_matrix(c, node, &a) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    for (unsigned i = 0; i < 4; ++i)
                    {
                        stroke_hit(c,
                                   root,
                                   a.xx * points[i][0] + a.xy * points[i][1] + a.x0,
                                   a.yx * points[i][0] + a.yy * points[i][1] + a.y0,
                                   i == 0   ? 1
                                   : i == 1 ? 0
                                   : i == 2 ? cap != 1
                                            : cap == 2);
                    }
                }
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
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
