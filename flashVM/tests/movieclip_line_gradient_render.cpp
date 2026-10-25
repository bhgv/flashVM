/* Script stroke ramps use their own colors, survive endFill and reset only with lineStyle/clear. */
#include <stdio.h>
#include <stdlib.h>
#include "line_gradient_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < 3; ++mode)
    {
        for (unsigned radial = 0; radial < 2; ++radial)
        {
            FVM_Object *clip = 0;
            if (!(fvm_movieclip_create(c, "line", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            line_gradient_style(c, clip, mode, radial != 0);
            line_gradient_geometry(c, clip);
            FVM_Object *data = clip->resource_result.object, *record = data->properties["lineGradient"].object,
                       *active = data->properties["active"].object;
            if (!(active->properties["lineGradient"].object == record))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            line_gradient_pixels(c, clip, mode, radial != 0);
            FVM_Value out;
            if (!(fvm_object_call(c, clip, "endFill", 0, 0, &out) == FVM_OK &&
                  data->properties["lineGradient"].object == record))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            line_gradient_pixels(c, clip, mode, radial != 0);
            FVM_Value width;
            fvm_value_number(&width, 2);
            if (!(fvm_object_call(c, clip, "lineStyle", 1, &width, &out) == FVM_OK &&
                  data->properties.find("lineGradient") == data->properties.end() &&
                  active->properties["lineGradient"].object == record))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            line_gradient_pixels(c, clip, mode, radial != 0);
            if (!(fvm_object_call(c, clip, "clear", 0, 0, &out) == FVM_OK &&
                  data->properties.find("lineGradient") == data->properties.end()))
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
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
