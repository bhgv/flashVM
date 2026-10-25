#include <stdio.h>
#include <stdlib.h>
#include "swf_stroke_hit_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned type = 0; type < 4; ++type)
    {
        FVM_Object *root = load(c, bs_tags(caps_shape(type, 2, 1)), 1);
        tick(c, root);
        FVM_Object *part = root->resource_result.object->properties["library"]
                               .object->properties["sprites"]
                               .object->indexed[9]
                               .object->properties["paths"]
                               .object->indexed[0]
                               .object;
        const char *names[] = {"lineCap", "lineJoin"};
        for (unsigned style = 0; style < 2; ++style)
        {
            FVM_Value prior = part->properties[names[style]], bad;
            const double values[] = {-1, .5, style ? 2.0 : 3.0, HUGE_VAL, HUGE_VAL - HUGE_VAL};
            for (unsigned i = 0; i < 5; ++i)
            {
                fvm_value_number(&bad, values[i]);
                part->properties[names[style]] = bad;
                FVM_Value a[3], out;
                fvm_value_number(a, 8);
                fvm_value_number(a + 1, 8);
                fvm_value_boolean(a + 2, 1);
                if (!(fvm_object_call(c, root, "hitTest", 3, a, &out) == FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            part->properties[names[style]] = prior;
        }
        stroke_hits(c, root, 2);
        part->properties.erase("lineCap");
        part->properties.erase("lineJoin");
        stroke_hits(c, root, 0);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
