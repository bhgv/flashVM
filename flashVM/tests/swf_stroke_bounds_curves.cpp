#include <stdio.h>
#include <stdlib.h>
#include "swf_stroke_bounds_fixture.hpp"


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
    for (unsigned type = 0; type < 4; ++type)
    {
        for (unsigned cap = 0; cap < 3; ++cap)
        {
            FVM_Object *root = load(c, bs_tags(caps_shape(type, cap, 1)), 1);
            tick(c, root);
            FVM_Object *p = bounds_part(root);
            FVM_Object *cmd = p->properties["commands"].object->indexed[1].object;
            bounds_number(cmd, "verb", FVM_PATH_QUADRATIC);
            bounds_number(cmd, "x0", 8);
            bounds_number(cmd, "y0", 16);
            bounds_number(cmd, "x1", 14);
            bounds_number(cmd, "y1", 8);
            FVM_PathBounds b;
            if (!(clip_bounds(c, root, &b) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            double r = cap == 2 ? 2 * std::sqrt(2.0) : 2;
            bounds_equal(b, 2 - r, 8 - r, 14 + r, 12 + r);
            if (!(clip_bounds(c, root, &b, "getRect") == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            bounds_equal(b, 2, 8, 14, 12);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
