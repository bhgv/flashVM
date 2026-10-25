/* Reject malformed optional stroke spread metadata before a single paint touches destination pixels. */
#include <stdio.h>
#include <stdlib.h>
#include "line_gradient_fixture.hpp"
#include <limits>


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *clip = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
          fvm_movieclip_create(c, "line", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    line_gradient_style(c, clip, 1);
    line_gradient_geometry(c, clip);
    FVM_Object *record = clip->resource_result.object->properties["lineGradient"].object;
    const double bad[] = {
        -1, 0.5, 3, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()};
    for (unsigned i = 0; i < 6; ++i)
    {
        FVM_Value value;
        if (i < 5)
        {
            fvm_value_number(&value, bad[i]);
        }
        else
        {
            if (!(fvm_value_string(&value, "repeat") == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        record->properties["spread"] = value;
        unsigned char pixels[1024], saved[1024];
        std::memset(pixels, 37, sizeof(pixels));
        std::memcpy(saved, pixels, sizeof(pixels));
        FVM_RasterSurface surface = {pixels, 16, 16, 64};
        FVM_Result result = fvm_movieclip_render(c, clip, &surface);
        if (!(result == FVM_INVALID_ARGUMENT && !std::memcmp(pixels, saved, sizeof(pixels))))
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
    record->properties.erase("spread");
    line_gradient_pixels(c, clip, 0);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
