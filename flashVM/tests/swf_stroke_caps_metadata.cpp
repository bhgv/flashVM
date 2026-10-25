#include <stdio.h>
#include <stdlib.h>
#include "swf_stroke_caps_fixture.hpp"


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
        FVM_Object *defs = root->resource_result.object->properties["library"].object->properties["sprites"].object;
        FVM_Object *part = defs->indexed[9].object->properties["paths"].object->indexed[0].object;
        const char *names[] = {"lineCap", "lineJoin"};
        for (unsigned style = 0; style < 2; ++style)
        {
            FVM_Value prior = part->properties[names[style]], bad;
            const double values[] = {-1, .5, style ? 2.0 : 3.0, HUGE_VAL, HUGE_VAL - HUGE_VAL};
            for (unsigned i = 0; i < 5; ++i)
            {
                fvm_value_number(&bad, values[i]);
                part->properties[names[style]] = bad;
                unsigned char pixels[1024], before[1024];
                std::memset(pixels, 37, sizeof(pixels));
                std::memcpy(before, pixels, sizeof(pixels));
                FVM_RasterSurface s = {pixels, 16, 16, 64};
                if (!(fvm_movieclip_render(c, root, &s) == FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(std::memcmp(pixels, before, sizeof(pixels)) == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            part->properties[names[style]] = prior;
        }
        caps_pixels(c, root, type, 2, 1);
        part->properties.erase("lineCap");
        part->properties.erase("lineJoin");
        caps_pixels(c, root, type, 0, 0);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
