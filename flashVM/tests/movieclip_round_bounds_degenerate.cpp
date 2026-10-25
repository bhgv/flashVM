#include <stdio.h>
#include <stdlib.h>
#include "round_bounds_fixture.hpp"


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
        ss_xy(c, clip, "moveTo", 20, 20);
        ss_xy(c, clip, "lineTo", 20, 20);
        bounds_equal(rb_bounds(c, clip), 0, 0, 0, 0);
        bounds_equal(rb_bounds(c, clip, "getRect"), 20, 20, 20, 20);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
