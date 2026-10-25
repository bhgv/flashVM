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
    for (unsigned kind = 0; kind < 4; ++kind)
    {
        FVM_Object *clip = rc_clip(c, 4, kind);
        ss_xy(c, clip, "moveTo", 100, 100);
        bounds_equal(rb_bounds(c, clip), 0, 2, 16, 10);
        bounds_equal(rb_bounds(c, clip, "getRect"), 2, 4, 100, 100);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
