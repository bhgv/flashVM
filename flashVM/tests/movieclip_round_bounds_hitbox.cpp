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
    FVM_Object *clip = rb_clip(c, 0);
    ss_geometry(c, clip);
    FVM_Value a[3], v;
    fvm_value_number(a, 1.6);
    fvm_value_number(a + 1, 8);
    fvm_value_boolean(a + 2, 0);
    if (!(fvm_object_call(c, clip, "hitTest", 3, a, &v) == FVM_OK && v.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(a, 1.4);
    if (!(fvm_object_call(c, clip, "hitTest", 3, a, &v) == FVM_OK && !v.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
