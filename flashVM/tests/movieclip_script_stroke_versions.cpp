#include <stdio.h>
#include <stdlib.h>
#include "script_stroke_fixture.hpp"


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
    fvm_context_destroy(c);
    c = 0;
    for (unsigned version = 1; version <= 8; ++version)
    {
        ss_context(rt, &c, version);
        FVM_Object *clip = ss_clip(c, 2, 1);
        ss_metadata(clip, version == 8 ? 2 : 0, version == 8 ? 1 : 0);
        ss_pixels(c, clip, version == 8 ? 2 : 0, version == 8 ? 1 : 0);
        FVM_Value a[8], out;
        ss_args(a, 2, 1);
        fvm_value_boolean(a + 3, 1);
        if (!(fvm_object_call(c, clip, "lineStyle", 8, a, &out) == (version == 8 ? FVM_UNSUPPORTED : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        c = 0;
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
