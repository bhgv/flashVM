/* Independent RGB/linearRGB fill/stroke ramps compose in order and retain both colors after binary transfer. */
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
    for (unsigned interpolation = 0; interpolation < 2; ++interpolation)
    {
        for (unsigned mode = 0; mode < 3; ++mode)
        {
            for (unsigned radial = 0; radial < 2; ++radial)
            {
                FVM_Context *c = 0, *d = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &d) == FVM_OK &&
                      fvm_standard_install(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *clip = 0;
                if (!(fvm_movieclip_create(c, "mixed", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value args[8], out, width;
                linear_arguments(c, args, mode, radial != 0);
                if (!(fvm_value_string(args + 6, interpolation ? "linearRGB" : "RGB") == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_call(c, clip, "beginGradientFill", 8, args, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_number(&width, 4);
                if (!(fvm_object_call(c, clip, "lineStyle", 1, &width, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value value;
                fvm_value_number(&value, 0x00ff00);
                if (!(fvm_table_set_index(c, args[1].object, 0, &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_number(&value, 0xffffff);
                if (!(fvm_table_set_index(c, args[1].object, 1, &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_value_string(args + 6, interpolation ? "RGB" : "linearRGB") == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_value_string(args + 5,
                                       mode == 0   ? "repeat"
                                       : mode == 1 ? "pad"
                                                   : "reflect") == FVM_OK &&
                      fvm_object_call(c, clip, "lineGradientStyle", 8, args, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                geometry(c, clip);
                FVM_Object *paint = clip->resource_result.object->properties["paths"].object->indexed[0].object;
                if (!(paint->properties["gradient"].object != paint->properties["lineGradient"].object))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                unsigned char actual[1024] = {0}, expected[1024] = {0}, received[1024] = {0};
                FVM_RasterSurface dst = {actual, 16, 16, 64}, oracle = {expected, 16, 16, 64};
                FVM_Result result = fvm_movieclip_render(c, clip, &dst);
                if (!(result == FVM_OK || result == FVM_UNSUPPORTED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (result == FVM_OK)
                {
                    FVM_Path *path = 0;
                    FVM_PathCommand command;
                    if (!(fvm_path_create(&path) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    std::memset(&command, 0, sizeof(command));
                    const double xy[] = {0, 0, 16, 0, 16, 16, 0, 16, 0, 0};
                    for (unsigned i = 0; i < 10; i += 2)
                    {
                        command.verb = i ? FVM_PATH_LINE : FVM_PATH_MOVE;
                        command.x[0] = xy[i];
                        command.y[0] = xy[i + 1];
                        if (!(fvm_path_add(path, &command) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    FVM_Gradient gradient;
                    FVM_GradientStop stops[2];
                    if (!(fvm_gradient_init(&gradient) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    stops[0].ratio = 0;
                    stops[0].color.r = stops[0].color.a = 255;
                    stops[0].color.g = stops[0].color.b = 0;
                    stops[1].ratio = 1;
                    stops[1].color.r = stops[1].color.g = 0;
                    stops[1].color.b = stops[1].color.a = 255;
                    gradient.stops = stops;
                    gradient.count = 2;
                    gradient.type = radial ? FVM_GRADIENT_RADIAL : FVM_GRADIENT_LINEAR;
                    gradient.matrix.xx = gradient.matrix.yy = 2;
                    gradient.matrix.x0 = 4.5;
                    gradient.matrix.y0 = 8.5;
                    if (!(fvm_path_draw_gradient_interpolated(path, &oracle, &gradient, 0, mode, interpolation) ==
                          FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    stops[0].color.r = stops[0].color.b = 0;
                    stops[0].color.g = 255;
                    stops[1].color.r = stops[1].color.g = 255;
                    if (!(fvm_path_stroke_gradient_interpolated(
                              path, &oracle, &gradient, 4, (mode + 2) % 3, 1 - interpolation) == FVM_OK &&
                          !std::memcmp(actual, expected, sizeof(actual))))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_path_destroy(path);
                }
                unsigned char *wire = 0;
                size_t size = 0;
                ref(&value, clip);
                if (!(fvm_value_serialize(c, &value, &wire, &size) == FVM_OK &&
                      fvm_value_deserialize(d, wire, size, &out) == FVM_OK && fvm_gc_pin(d, out.object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_serialized_free(wire);
                fvm_context_destroy(c);
                if (!(fvm_gc_collect(d, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                dst.pixels = received;
                if (!(fvm_movieclip_render(d, out.object, &dst) == result &&
                      !std::memcmp(actual, received, sizeof(actual))))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!d->gc_frames && !d->gc_native_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(d);
            }
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
