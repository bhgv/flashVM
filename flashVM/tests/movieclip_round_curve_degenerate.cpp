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
    for (unsigned i = 0; i < 2; ++i)
    {
        FVM_Object *clip = rb_clip(c, i ? 4 : 0);
        ss_xy(c, clip, "moveTo", 50, 60);
        FVM_Value a[4], out;
        for (unsigned n = 0; n < 4; ++n)
        {
            fvm_value_number(a + n, n % 2 ? 60 : 50);
        }
        if (!(fvm_object_call(c, clip, "curveTo", 4, a, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        bounds_equal(rb_bounds(c, clip), 0, 0, 0, 0);
        bounds_equal(rb_bounds(c, clip, "getRect"), 50, 60, 50, 60);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
