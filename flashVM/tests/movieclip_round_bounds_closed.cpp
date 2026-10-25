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
    FVM_Object *clip = rb_clip(c, 4);
    ss_xy(c, clip, "moveTo", 2, 2);
    ss_xy(c, clip, "lineTo", 10, 2);
    ss_xy(c, clip, "lineTo", 10, 10);
    ss_xy(c, clip, "lineTo", 2, 2);
    bounds_equal(rb_bounds(c, clip), 0, 0, 12, 12);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
