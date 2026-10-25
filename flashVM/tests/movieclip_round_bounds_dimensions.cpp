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
    FVM_Value v;
    if (!(fvm_object_get(c, clip, "_width", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bound_near(v.number, 13);
    if (!(fvm_object_get(c, clip, "_height", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bound_near(v.number, 1);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
