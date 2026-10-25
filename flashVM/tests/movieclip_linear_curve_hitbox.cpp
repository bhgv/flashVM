#include <stdio.h>
#include <stdlib.h>
#include "linear_curve_fixture.hpp"


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
            FVM_Object *clip = lc_clip(c, cap, join);
            ac_move(c, clip);
            FVM_Value a[3], out;
            fvm_value_number(a, 50);
            fvm_value_number(a + 1, 50);
            fvm_value_boolean(a + 2, 0);
            if (!(fvm_object_call(c, clip, "hitTest", 3, a, &out) == FVM_OK && !out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(a, 8);
            fvm_value_number(a + 1, 7);
            if (!(fvm_object_call(c, clip, "hitTest", 3, a, &out) == FVM_OK && out.boolean))
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
