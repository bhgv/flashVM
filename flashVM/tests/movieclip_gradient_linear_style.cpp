/* linearRGB copies optional metadata only in host/SWF8 and rejects counted-name suffixes atomically. */
#include <stdio.h>
#include <stdlib.h>
#include "gradient_linear_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned versions[] = {0, 6, 7, 8};
    for (unsigned v = 0; v < 4; ++v)
    {
        for (unsigned stroke = 0; stroke < 2; ++stroke)
        {
            for (unsigned radial = 0; radial < 2; ++radial)
            {
                if (stroke && versions[v] && versions[v] < 8)
                {
                    continue;
                }
                FVM_Context *c = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (versions[v])
                {
                    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                    swf[3] = (unsigned char)versions[v];
                    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_standard_install(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *clip = 0;
                if (!(fvm_movieclip_create(c, "root", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value args[8], out;
                linear_arguments(c, args, 0, radial != 0);
                if (stroke)
                {
                    FVM_Value width;
                    fvm_value_number(&width, 4);
                    if (!(fvm_object_call(c, clip, "lineStyle", 1, &width, &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                const char *method = stroke ? "lineGradientStyle" : "beginGradientFill",
                           *key = stroke ? "lineGradient" : "gradient";
                if (!(fvm_object_call(c, clip, method, 5, args, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *data = clip->resource_result.object, *old = data->properties[key].object;
                if (!(old->properties.find("interpolation") == old->properties.end()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const bool modern = !versions[v] || versions[v] >= 8;
                if (!(fvm_object_call(c, clip, method, 8, args, &out) == (modern ? FVM_OK : FVM_UNSUPPORTED)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *record = data->properties[key].object;
                if (!(modern ? record != old && record->properties["interpolation"].number == 1 : record == old))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_value_string_bytes(args + 6, "linearRGB\0ignored", 17) == FVM_OK &&
                      fvm_object_call(c, clip, method, 7, args, &out) == FVM_UNSUPPORTED &&
                      data->properties[key].object == record))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
