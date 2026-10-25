#include <stdio.h>
#include <stdlib.h>
#include "axis_curve_fixture.hpp"


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
            FVM_Object *clip = ac_clip(c, cap, join);
            ac_move(c, clip);
            rc_number(rc_part(clip), "fill", 1);
            bounds_equal(rb_bounds(c, clip), cap == 1 ? 2 : 0, 6, 100, 100);
            rc_number(rc_part(clip), "fill", 0);
            ac_expected(rb_bounds(c, clip), cap);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
