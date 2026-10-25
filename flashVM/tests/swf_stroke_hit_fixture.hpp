#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_SWF_STROKE_HIT_FIXTURE_HPP
#define FVM_SWF_STROKE_HIT_FIXTURE_HPP
#include "swf_stroke_caps_fixture.hpp"


static void stroke_hit(FVM_Context *c, FVM_Object *root, double x, double y, int expected)
{
    FVM_Value a[3], out;
    fvm_value_number(a, x);
    fvm_value_number(a + 1, y);
    fvm_value_boolean(a + 2, 1);
    if (!(fvm_object_call(c, root, "hitTest", 3, a, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(out.type == FVM_VALUE_BOOLEAN && !!out.boolean == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void stroke_hits(FVM_Context *c, FVM_Object *root, unsigned cap)
{
    stroke_hit(c, root, 8, 8, 1);
    stroke_hit(c, root, 8, 10.1, 0);
    stroke_hit(c, root, 1, 8, cap != 1);
    stroke_hit(c, root, .1, 6.1, cap == 2);
}
#endif
