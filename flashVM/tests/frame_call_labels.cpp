/* Duplicate counted labels select the lowest frame regardless of insertion; lookup obeys work limits. */
#include <stdio.h>
#include <stdlib.h>
#include "frame_blocks_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (!version || version >= 5)
        {
            FVM_Context *c = tick_context(rt, version);
            FVM_Object *clip = tick_root(c, 3);
            Bytes high = action(3), low = action(1);
            if (!(fvm_movieclip_define_frame(c, clip, 3, "duplicate", high.begin(), high.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_movieclip_define_frame(c, clip, 1, "duplicate", low.begin(), low.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value label;
            if (!(fvm_value_string(&label, "duplicate") == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            rt->config.instruction_limit = 10;
            if (!(fvm_movieclip_call_frame(c, clip, label) == FVM_ERROR && c->avm1_steps == 10))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            tick_clean(c);
            FVM_Value seen;
            if (!(fvm_object_get(c, clip, "seen", &seen) == FVM_OK && seen.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            rt->config.instruction_limit = 0;
            if (!(fvm_movieclip_call_frame(c, clip, label) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, clip, "seen", 1);
            FVM_Object *frames = clip->resource_result.object->properties["frames"].object;
            FVM_Value counted;
            if (!(fvm_value_string_bytes(&counted, "x\0label", 7) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            frames->indexed[3].object->properties["label"] = counted;
            frames->indexed[1].object->properties["label"] = counted;
            if (!(fvm_movieclip_call_frame(c, clip, counted) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, clip, "seen", 1);
            tick_clean(c);
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
