#include <stdio.h>
#include <stdlib.h>
#include "secondary_axis_fixture.hpp"


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
    FVM_Object *target = sa_target(c);
    for (int e = -600; e <= 10; e += 610)
    {
        for (unsigned cap = 0; cap < 3; ++cap)
        {
            for (unsigned join = 0; join < 2; ++join)
            {
                FVM_Object *clip = sa_clip(c, cap, join, e);
                double scale = std::ldexp(1.0, e);
                FVM_Affine axis = {0, 0, 0, 5 * scale, 0, 0};
                if (!(fvm_movieclip_set_matrix(c, clip, &axis) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *cmd = rc_part(clip)->properties["commands"].object->indexed[1].object;
                rc_number(cmd, "verb", FVM_PATH_QUADRATIC);
                rc_number(cmd, "x0", 0);
                rc_number(cmd, "y0", 5 * sa_unit(e));
                rc_number(cmd, "x1", 0);
                rc_number(cmd, "y1", 10 * sa_unit(e));
                lc_expected(sa_bounds(c, clip, target, e), cap, 20, 0, 0, 0, 50);
            }
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
