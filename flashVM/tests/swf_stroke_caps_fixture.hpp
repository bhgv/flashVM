/* Raw symmetric LINESTYLE2 caps/joins across solid, gradient and bitmap paints. */
#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_STROKE_CAPS_FIXTURE_HPP
#define FVM_STROKE_CAPS_FIXTURE_HPP
#include "swf_bitmap_stroke_fixture.hpp"


static Bytes caps_shape(unsigned type,
                        unsigned cap,
                        unsigned join,
                        unsigned width = 80,
                        bool closed = false,
                        bool no_close = false,
                        size_t *style_end = 0)
{
    size_t style = 0, end = 0;
    Bytes base = bs_shape(0x42, width, no_close, closed, 0, 7, &style, &end);
    if (type == 3)
    {
        if (style_end)
        {
            *style_end = end;
        }
        base[style + 2] = (unsigned char)((cap << 6) | (join << 4) | 8);
        base[style + 3] = (unsigned char)(cap | (no_close ? 4 : 0));
        return base;
    }
    Bytes b;
    for (size_t i = 0; i < style; ++i)
    {
        b.push_back(base[i]);
    }
    u16(b, width);
    b.push_back((unsigned char)((cap << 6) | (join << 4) | 8));
    b.push_back((unsigned char)(cap | (no_close ? 4 : 0)));
    if (type == 0)
    {
        b.push_back(0);
        b.push_back(255);
        b.push_back(0);
        b.push_back(0);
        b.push_back(128);
    }
    else
    {
        b.push_back(type == 1 ? 0x10 : 0x13);
        Bits matrix;
        matrix.put(1, 1);
        matrix.put(11, 5);
        matrix.put(320, 11);
        matrix.put(320, 11);
        matrix.put(0, 1);
        matrix.put(16, 5);
        matrix.put(160, 16);
        matrix.put(160, 16);
        append(b, matrix.b);
        b.push_back(0x52);
        b.push_back(0);
        b.push_back(255);
        b.push_back(0);
        b.push_back(0);
        b.push_back(255);
        b.push_back(255);
        b.push_back(0);
        b.push_back(0);
        b.push_back(255);
        b.push_back(255);
        if (type == 2)
        {
            u16(b, 128);
        }
    }
    if (style_end)
    {
        *style_end = b.size();
    }
    for (size_t i = end; i < base.size(); ++i)
    {
        b.push_back(base[i]);
    }
    return b;
}


static void caps_expected(unsigned char *out,
                          unsigned type,
                          unsigned cap,
                          unsigned join,
                          unsigned width = 80,
                          bool closed = false,
                          bool no_close = false,
                          bool antialias = true)
{
    if (type == 3)
    {
        bs_expected(out, 0x42, width, no_close, closed, 0, antialias, cap, join);
        return;
    }
    std::memset(out, 0, 1024);
    FVM_RasterSurface s = {out, 16, 16, 64};
    FVM_Path *path = bs_path(closed, no_close);
    if (type == 0)
    {
        FVM_RasterColor red = {255, 0, 0, 128};
        if (!(fvm_path_draw_quality(path, &s, red, width ? width / 20.0 : 1, 0, antialias, cap, join) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        FVM_Gradient g;
        FVM_GradientStop stops[2];
        if (!(fvm_gradient_init(&g) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        std::memset(stops, 0, sizeof(stops));
        stops[0].color.r = stops[0].color.a = 255;
        stops[1].ratio = 1;
        stops[1].color.b = stops[1].color.a = 255;
        g.type = type == 1 ? FVM_GRADIENT_LINEAR : FVM_GRADIENT_RADIAL;
        g.stops = stops;
        g.count = 2;
        g.matrix.xx = g.matrix.yy = 4;
        g.matrix.x0 = g.matrix.y0 = 8;
        if (!(fvm_path_gradient_focal_quality(
                  path, &s, &g, width ? width / 20.0 : 1, 0, 1, 1, type == 2 ? .5 : 0, antialias, cap, join) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_path_destroy(path);
}


static void caps_pixels(FVM_Context *c,
                        FVM_Object *root,
                        unsigned type,
                        unsigned cap,
                        unsigned join,
                        unsigned width = 80,
                        bool closed = false,
                        bool no_close = false,
                        bool antialias = true)
{
    unsigned char actual[1024], expected[1024];
    std::memset(actual, 0, sizeof(actual));
    FVM_RasterSurface s = {actual, 16, 16, 64};
    FVM_Result r = fvm_movieclip_render(c, root, &s);
    if (r == FVM_UNSUPPORTED)
    {
        return;
    }
    if (!(r == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    caps_expected(expected, type, cap, join, width, closed, no_close, antialias);
    if (!(std::memcmp(actual, expected, sizeof(actual)) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#endif
