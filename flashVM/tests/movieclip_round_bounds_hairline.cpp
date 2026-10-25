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
    for (unsigned version = 6; version <= 8; ++version)
    {
        FVM_Context *v = 0;
        ss_context(rt, &v, version);
        FVM_Object *clip = rb_clip(v, 0);
        ss_geometry(v, clip);
        bounds_equal(rb_bounds(v, clip), 1.5, 7.5, 14.5, 8.5);
        bounds_equal(rb_bounds(v, clip, "getRect"), 2, 8, 14, 8);
        fvm_context_destroy(v);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
