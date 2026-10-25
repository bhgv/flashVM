#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_ROUND_BOUNDS_FIXTURE_HPP
#define FVM_ROUND_BOUNDS_FIXTURE_HPP
#include "script_stroke_fixture.hpp"


static FVM_PathBounds rb_bounds(FVM_Context *c, FVM_Object *clip, const char *method = "getBounds")
{
    FVM_Value a, out;
    ref(&a, clip);
    if (!(fvm_object_call(c, clip, method, 1, &a, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *b = out.object;
    FVM_PathBounds r = {b->properties["xMin"].number,
                        b->properties["yMin"].number,
                        b->properties["xMax"].number,
                        b->properties["yMax"].number};
    return r;
}


static FVM_Object *rb_clip(FVM_Context *c, double width)
{
    FVM_Object *clip = 0;
    if (!(fvm_movieclip_create(c, "round", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ss_style(c, clip, 0, 0, width);
    return clip;
}
#endif
