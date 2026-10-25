#include <stdio.h>
#include <stdlib.h>
#include "round_curve_fixture.hpp"


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
    FVM_Object *clip = rc_clip(c);
    ss_xy(c, clip, "moveTo", 100, 100);
    bounds_equal(rb_bounds(c, clip), 0, 2, 16, 10);
    ss_style(c, clip, 2, 1, 8);
    bounds_equal(rb_bounds(c, clip), 0, 2, 16, 10);
    FVM_Value out;
    if (!(fvm_object_call(c, clip, "clear", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bounds_equal(rb_bounds(c, clip), 0, 0, 0, 0);
    ss_style(c, clip, 0, 0, 0);
    ss_geometry(c, clip, 1);
    bounds_equal(rb_bounds(c, clip), 1.5, 3.5, 14.5, 8.5);
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bounds_equal(rb_bounds(c, clip), 1.5, 3.5, 14.5, 8.5);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
