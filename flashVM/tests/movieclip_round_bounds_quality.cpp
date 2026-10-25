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
    for (unsigned q = 0; q < 4; ++q)
    {
        c->stage_quality = (FVM_StageQuality)q;
        bounds_equal(rb_bounds(c, clip), 1.5, 7.5, 14.5, 8.5);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
