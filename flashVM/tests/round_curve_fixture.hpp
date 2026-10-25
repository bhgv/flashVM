#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_ROUND_CURVE_FIXTURE_HPP
#define FVM_ROUND_CURVE_FIXTURE_HPP
#include "round_bounds_fixture.hpp"


static FVM_Path *rc_path(unsigned kind = 0)
{
    FVM_Path *p = 0;
    if (!(fvm_path_create(&p) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(hit_add(p, FVM_PATH_MOVE, 2, 8) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (kind == 0)
    {
        if (!(hit_add(p, FVM_PATH_QUADRATIC, 8, 0, 14, 8) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (kind == 1)
    {
        if (!(hit_add(p, FVM_PATH_CUBIC, 2, 0, 14, 0, 14, 8) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(hit_add(p, FVM_PATH_CUBIC, 2, 20, 14, -4, 14, 8) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return p;
}


static FVM_Object *rc_clip(FVM_Context *c, double width = 4, unsigned kind = 0)
{
    FVM_Object *clip = ss_clip(c, 0, 0, kind, 1);
    if (width != 4)
    {
        FVM_Value out;
        if (!(fvm_object_call(c, clip, "clear", 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ss_style(c, clip, 0, 0, width);
        ss_geometry(c, clip, 1);
    }
    return clip;
}


static FVM_Object *rc_part(FVM_Object *clip)
{
    return clip->resource_result.object->properties["paths"].object->indexed[0].object;
}


static void rc_number(FVM_Object *o, const char *key, double n)
{
    FVM_Value v;
    fvm_value_number(&v, n);
    o->properties[key] = v;
}


static void rc_cubic(FVM_Object *clip)
{
    FVM_Object *p = rc_part(clip)->properties["commands"].object->indexed[1].object;
    rc_number(p, "verb", FVM_PATH_CUBIC);
    rc_number(p, "x0", 2);
    rc_number(p, "y0", 0);
    rc_number(p, "x1", 14);
    rc_number(p, "y1", 0);
    rc_number(p, "x2", 14);
    rc_number(p, "y2", 8);
}
#endif
