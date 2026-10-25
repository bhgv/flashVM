/* Optional interpolation metadata is exact owned scalar data, with RGB fallback and no raster-time getter. */
#include <stdio.h>
#include <stdlib.h>
#include "gradient_linear_fixture.hpp"
#include <limits>


static FVM_Result
forbidden_color_getter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    ++*(unsigned *)raw;
    fvm_value_number(out, 1);
    return FVM_OK;
}


int main()
{
    for (unsigned stroke = 0; stroke < 2; ++stroke)
    {
        FVM_Config cfg;
        FVM_Runtime *rt = 0;
        FVM_Context *c = 0;
        FVM_Object *clip = 0;
        if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
              fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
              fvm_movieclip_create(c, "linear", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        linear_style(c, clip, 1, false, stroke != 0);
        if (stroke)
        {
            line_gradient_geometry(c, clip);
        }
        else
        {
            geometry(c, clip, false);
        }
        FVM_Object *record = clip->resource_result.object->properties[stroke ? "lineGradient" : "gradient"].object;
        const double bad[] = {
            -1, 0.5, 2, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()};
        for (unsigned i = 0; i < 6; ++i)
        {
            FVM_Value value;
            if (i < 5)
            {
                fvm_value_number(&value, bad[i]);
            }
            else
            {
                if (!(fvm_value_string(&value, "linearRGB") == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            record->properties["interpolation"] = value;
            unsigned char pixels[1024], saved[1024];
            std::memset(pixels, 37, sizeof(pixels));
            std::memcpy(saved, pixels, sizeof(pixels));
            FVM_RasterSurface surface = {pixels, 16, 16, 64};
            if (!(fvm_movieclip_render(c, clip, &surface) == FVM_INVALID_ARGUMENT &&
                  !std::memcmp(pixels, saved, sizeof(pixels))))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        record->properties.erase("interpolation");
        unsigned calls = 0;
        if (!(fvm_object_define_accessor_rooted(c, record, "interpolation", forbidden_color_getter, 0, &calls, clip) ==
              FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        linear_pixels(c, clip, 1, false, stroke != 0, 0);
        if (!(!calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
    }
    return 0;
}
