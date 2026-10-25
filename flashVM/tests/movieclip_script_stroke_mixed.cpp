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
    for (unsigned cap = 0; cap < 3; ++cap)
    {
        for (unsigned join = 0; join < 2; ++join)
        {
            FVM_Object *clip = 0;
            if (!(fvm_movieclip_create(c, "mixed", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value a[8], out;
            spread_arguments(c, a, 1);
            if (!(fvm_object_call(c, clip, "beginGradientFill", 8, a, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *fill = clip->resource_result.object->properties["gradient"].object;
            ss_style(c, clip, cap, join);
            if (!(clip->resource_result.object->properties["gradient"].object == fill))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ss_geometry(c, clip);
            FVM_Object *part = clip->resource_result.object->properties["active"].object;
            if (!(part->properties["gradient"].object == fill && part->properties["fill"].number == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ss_metadata(clip, cap, join);
            if (!(fvm_object_call(c, clip, "endFill", 0, 0, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ss_xy(c, clip, "moveTo", 2, 12);
            ss_xy(c, clip, "lineTo", 14, 12);
            part = clip->resource_result.object->properties["active"].object;
            if (cap)
            {
                if (!(part->properties["lineCap"].number == cap))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (join)
            {
                if (!(part->properties["lineJoin"].number == join))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(part->properties["fill"].number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
