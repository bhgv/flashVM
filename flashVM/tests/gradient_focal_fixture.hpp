/* Shared focal arguments and separately constructed C89 paint comparisons. */
#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_TEST_GRADIENT_FOCAL_FIXTURE_HPP
#define FVM_TEST_GRADIENT_FOCAL_FIXTURE_HPP
#include "gradient_linear_fixture.hpp"


static void focal_arguments(FVM_Context *c, FVM_Value *args, unsigned mode, double focal, unsigned interpolation = 0)
{
    spread_arguments(c, args, mode, true);
    if (!(fvm_value_string(args + 6, interpolation ? "linearRGB" : "RGB") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 7, focal);
}


static void focal_style(
    FVM_Context *c, FVM_Object *clip, unsigned mode, double focal, bool stroke = false, unsigned interpolation = 0)
{
    FVM_Value args[8], out;
    focal_arguments(c, args, mode, focal, interpolation);
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
    if (!(fvm_object_call(c, clip, stroke ? "lineGradientStyle" : "beginGradientFill", 8, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Result focal_pixels(
    FVM_Context *c, FVM_Object *clip, unsigned mode, double focal, bool stroke = false, unsigned interpolation = 0)
{
    unsigned char actual[1024] = {0}, expected[1024] = {0};
    FVM_RasterSurface dst = {actual, 16, 16, 64}, oracle = {expected, 16, 16, 64};
    FVM_Result result = fvm_movieclip_render(c, clip, &dst);
    if (result == FVM_UNSUPPORTED)
    {
        return result;
    }
    if (!(result == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Path *path = 0;
    FVM_PathCommand command;
    if (!(fvm_path_create(&path) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::memset(&command, 0, sizeof(command));
    const double xy[] = {0, 0, 16, 0, 16, 16, 0, 16};
    if (stroke)
    {
        command.verb = FVM_PATH_MOVE;
        command.x[0] = 1;
        command.y[0] = 8;
        if (!(fvm_path_add(path, &command) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        command.verb = FVM_PATH_LINE;
        command.x[0] = 15;
        if (!(fvm_path_add(path, &command) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
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
    gradient.type = FVM_GRADIENT_RADIAL;
    gradient.matrix.xx = gradient.matrix.yy = 2;
    gradient.matrix.x0 = 4.5;
    gradient.matrix.y0 = 8.5;
    if (!((stroke ? fvm_path_stroke_gradient_focal(path, &oracle, &gradient, 4, mode, interpolation, focal)
                  : fvm_path_draw_gradient_focal(path, &oracle, &gradient, 0, mode, interpolation, focal)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    if (!(!std::memcmp(actual, expected, sizeof(actual))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return result;
}
#endif
