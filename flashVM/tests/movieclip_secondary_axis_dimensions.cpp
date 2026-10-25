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
                FVM_PathBounds b = sa_bounds(c, clip, target, e);
                FVM_Value w, h;
                if (!(fvm_object_get(c, clip, "_width", &w) == FVM_OK &&
                      fvm_object_get(c, clip, "_height", &h) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                bound_near(w.number / sa_output(e), b.x_max - b.x_min);
                bound_near(h.number / sa_output(e), b.y_max - b.y_min);
            }
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
