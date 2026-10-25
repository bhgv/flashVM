#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_SCRIPT_STROKE_FIXTURE_HPP
#define FVM_SCRIPT_STROKE_FIXTURE_HPP
#include "gradient_spread_fixture.hpp"
#include "path_style_bounds_fixture.hpp"
#include "flashvm/stage.h"


static void ss_context(FVM_Runtime *rt, FVM_Context **c, unsigned version = 0)
{
    if (!(fvm_context_create(rt, c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (version)
    {
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(*c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_standard_install(*c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void ss_args(FVM_Value *a, unsigned cap, unsigned join, double width = 4)
{
    for (unsigned i = 0; i < 8; ++i)
    {
        fvm_value_undefined(a + i);
    }
    fvm_value_number(a, width);
    fvm_value_number(a + 1, 0xff0000);
    fvm_value_number(a + 2, 100);
    fvm_value_boolean(a + 3, 0);
    if (!(fvm_value_string(a + 4, "normal") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string(a + 5, cap == 1 ? "none" : cap == 2 ? "square" : "round") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string(a + 6, join ? "bevel" : "round") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void ss_style(FVM_Context *c, FVM_Object *clip, unsigned cap, unsigned join, double width = 4)
{
    FVM_Value a[8], out;
    ss_args(a, cap, join, width);
    if (!(fvm_object_call(c, clip, "lineStyle", 8, a, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void ss_xy(FVM_Context *c, FVM_Object *clip, const char *method, double x, double y)
{
    FVM_Value a[2], out;
    fvm_value_number(a, x);
    fvm_value_number(a + 1, y);
    if (!(fvm_object_call(c, clip, method, 2, a, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void ss_geometry(FVM_Context *c, FVM_Object *clip, unsigned geometry = 0)
{
    ss_xy(c, clip, "moveTo", 2, 8);
    if (geometry == 1)
    {
        FVM_Value a[4], out;
        fvm_value_number(a, 8);
        fvm_value_number(a + 1, 0);
        fvm_value_number(a + 2, 14);
        fvm_value_number(a + 3, 8);
        if (!(fvm_object_call(c, clip, "curveTo", 4, a, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        ss_xy(c, clip, "lineTo", geometry == 2 ? 8 : 14, 8);
        if (geometry == 2)
        {
            ss_xy(c, clip, "lineTo", 8, 14);
        }
    }
}


static FVM_Object *ss_clip(FVM_Context *c, unsigned cap, unsigned join, unsigned kind = 0, unsigned geometry = 0)
{
    FVM_Object *clip = 0;
    if (!(fvm_movieclip_create(c, "stroke", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ss_style(c, clip, cap, join);
    if (kind)
    {
        FVM_Value a[8], out;
        spread_arguments(c, a, 1, kind > 1);
        if (kind == 3)
        {
            fvm_value_number(a + 7, .5);
        }
        if (!(fvm_object_call(c, clip, "lineGradientStyle", 8, a, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    ss_geometry(c, clip, geometry);
    return clip;
}


static void ss_metadata(FVM_Object *clip, unsigned cap, unsigned join)
{
    FVM_Object *part = clip->resource_result.object->properties["paths"].object->indexed[0].object;
    const char *names[] = {"lineCap", "lineJoin"};
    const unsigned values[] = {cap, join};
    for (unsigned i = 0; i < 2; ++i)
    {
        if (values[i])
        {
            if (!(part->properties[names[i]].type == FVM_VALUE_NUMBER &&
                  part->properties[names[i]].number == values[i]))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(part->properties.find(names[i]) == part->properties.end()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
}


static void
ss_pixels(FVM_Context *c, FVM_Object *clip, unsigned cap, unsigned join, unsigned kind = 0, unsigned geometry = 0)
{
    unsigned char a[1024] = {0}, b[1024] = {0};
    FVM_RasterSurface dst = {a, 16, 16, 64}, oracle = {b, 16, 16, 64};
    FVM_Result result = fvm_movieclip_render(c, clip, &dst);
    if (result == FVM_UNSUPPORTED)
    {
        return;
    }
    if (!(result == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Path *p = 0;
    if (!(fvm_path_create(&p) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(hit_add(p, FVM_PATH_MOVE, 2, 8) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (geometry == 1)
    {
        if (!(hit_add(p, FVM_PATH_QUADRATIC, 8, 0, 14, 8) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(hit_add(p, FVM_PATH_LINE, geometry == 2 ? 8 : 14, 8) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (geometry == 2)
        {
            if (!(hit_add(p, FVM_PATH_LINE, 8, 14) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    bool aa = c->stage_quality != FVM_STAGE_QUALITY_LOW;
    if (!kind)
    {
        FVM_RasterColor red = {255, 0, 0, 255};
        if (!(fvm_path_draw_quality(p, &oracle, red, 4, 0, aa, cap, join) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        FVM_Gradient g;
        if (!(fvm_gradient_init(&g) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_GradientStop stops[2];
        std::memset(stops, 0, sizeof(stops));
        stops[0].color.r = stops[0].color.a = 255;
        stops[1].ratio = 1;
        stops[1].color.b = stops[1].color.a = 255;
        g.stops = stops;
        g.count = 2;
        g.type = kind > 1 ? FVM_GRADIENT_RADIAL : FVM_GRADIENT_LINEAR;
        g.matrix.xx = g.matrix.yy = 2;
        g.matrix.x0 = 4.5;
        g.matrix.y0 = 8.5;
        if (!(fvm_path_gradient_focal_quality(p,
                                              &oracle,
                                              &g,
                                              4,
                                              0,
                                              FVM_GRADIENT_SPREAD_REFLECT,
                                              FVM_GRADIENT_INTERPOLATION_RGB,
                                              kind == 3 ? .5 : 0,
                                              aa,
                                              cap,
                                              join) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_path_destroy(p);
    if (!(std::memcmp(a, b, sizeof(a)) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#endif
