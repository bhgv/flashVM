/* Fault sweeps retain old RGB styles/active aliases and destination pixels until linearRGB preparation succeeds. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "gradient_linear_fixture.hpp"


int main(int argc, char **)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned stroke = 0; stroke < 2; ++stroke)
    {
        for (unsigned mode = 0; mode < 3; ++mode)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
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
            linear_arguments(c, args, mode);
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
            FVM_Value point[2];
            fvm_value_number(point, 1);
            fvm_value_number(point + 1, 8);
            if (!(fvm_object_call(c, clip, "moveTo", 2, point, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *data = clip->resource_result.object, *old = data->properties[key].object,
                       *active = data->properties["active"].object;
            bool success = false;
            unsigned failures = 0;
            for (long boundary = 0; boundary < 4096 && !success; ++boundary)
            {
                allowance = argc > 1 ? -1 : boundary;
                FVM_Result result = fvm_object_call(c, clip, method, 8, args, &out);
                allowance = -1;
                if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                success = result == FVM_OK;
                if (!success)
                {
                    ++failures;
                    if (!(data->properties[key].object == old && data->properties["active"].object == active))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(data->properties[key].object->properties["interpolation"].number == 1))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(success && (argc > 1 || failures)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (stroke)
            {
                line_gradient_geometry(c, clip);
            }
            else
            {
                geometry(c, clip);
            }
            linear_pixels(c, clip, mode, false, stroke != 0);
            fvm_context_destroy(c);
        }
    }
    FVM_Path *path = 0;
    if (!(fvm_path_create(&path) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_PathCommand command;
    std::memset(&command, 0, sizeof(command));
    const double xy[] = {0, 0, 16, 0, 16, 16, 0, 16};
    for (unsigned i = 0; i < 8; i += 2)
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
    command.verb = FVM_PATH_CLOSE;
    if (!(fvm_path_add(path, &command) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Gradient gradient;
    if (!(fvm_gradient_init(&gradient) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_GradientStop stops[2];
    stops[0].ratio = 0;
    stops[0].color.r = stops[0].color.a = 255;
    stops[0].color.g = stops[0].color.b = 0;
    stops[1].ratio = 1;
    stops[1].color.r = stops[1].color.g = 0;
    stops[1].color.b = stops[1].color.a = 255;
    gradient.stops = stops;
    gradient.count = 2;
    gradient.matrix.xx = gradient.matrix.yy = 2;
    for (unsigned stroke = 0; stroke < 2; ++stroke)
    {
        for (unsigned type = 0; type < 2; ++type)
        {
            for (unsigned mode = 0; mode < 3; ++mode)
            {
                gradient.type = (FVM_GradientType)type;
                bool success = false, supported = true;
                unsigned failures = 0;
                for (long boundary = 0; boundary < 4096 && !success; ++boundary)
                {
                    unsigned char pixels[1024], saved[1024];
                    std::memset(pixels, 37, sizeof(pixels));
                    std::memcpy(saved, pixels, sizeof(pixels));
                    FVM_RasterSurface surface = {pixels, 16, 16, 64};
                    allowance = argc > 1 ? -1 : boundary;
                    FVM_Result result =
                        stroke ? fvm_path_stroke_gradient_interpolated(path, &surface, &gradient, 4, mode, 1)
                               : fvm_path_draw_gradient_interpolated(path, &surface, &gradient, 0, mode, 1);
                    allowance = -1;
                    if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY || result == FVM_UNSUPPORTED))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    success = result != FVM_OUT_OF_MEMORY;
                    supported = result != FVM_UNSUPPORTED;
                    if (!success)
                    {
                        ++failures;
                        if (!(!std::memcmp(pixels, saved, sizeof(pixels))))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                if (!(success && (!supported || argc > 1 || failures)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    fvm_path_destroy(path);
    fvm_runtime_destroy(rt);
    return 0;
}
