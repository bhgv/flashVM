/* Shared stroke construction and independently sampled C89 stroke comparison. */
#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_TEST_LINE_GRADIENT_FIXTURE_HPP
#define FVM_TEST_LINE_GRADIENT_FIXTURE_HPP
#include "gradient_spread_fixture.hpp"


static void line_gradient_style(FVM_Context *c, FVM_Object *clip, unsigned mode, bool radial = false)
{
    FVM_Value line[3], args[8], out;
    fvm_value_number(line, 4);
    fvm_value_number(line + 1, 0x00ff00);
    fvm_value_number(line + 2, 12);
    if (!(fvm_object_call(c, clip, "lineStyle", 3, line, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    spread_arguments(c, args, mode, radial);
    if (!(fvm_object_call(c, clip, "lineGradientStyle", 8, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void line_gradient_geometry(FVM_Context *c, FVM_Object *clip)
{
    FVM_Value args[2], out;
    fvm_value_number(args, 1);
    fvm_value_number(args + 1, 8);
    if (!(fvm_object_call(c, clip, "moveTo", 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 15);
    if (!(fvm_object_call(c, clip, "lineTo", 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Result line_gradient_pixels(FVM_Context *c, FVM_Object *clip, unsigned mode, bool radial = false)
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
    if (!(fvm_path_stroke_gradient(path, &oracle, &gradient, 4, mode) == FVM_OK))
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
