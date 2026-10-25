/* C89 analytic quarter-phase tables distinguish encoded RGB from linear-light RGB. */
#define main gradient_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "gradient.c"

#undef main
#include <limits.h>


int main(void)
{
    FVM_Path *path = box();
    FVM_Gradient g;
    FVM_GradientStop stops[2];
    FVM_RasterSurface surface;
    unsigned char pixels[1024], legacy[1024], saved[1024];
    unsigned mode, type, x, stroke;
    FVM_Result result;
    const unsigned char red[2][3][16] = {
        {{255, 255, 255, 225, 188, 137, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
         {188, 225, 255, 225, 188, 137, 0, 137, 188, 225, 255, 225, 188, 137, 0, 137},
         {188, 137, 255, 225, 188, 137, 255, 225, 188, 137, 255, 225, 188, 137, 255, 225}},
        {{0, 0, 0, 188, 255, 188, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
         {255, 188, 0, 188, 255, 188, 0, 188, 255, 188, 0, 188, 255, 188, 0, 188},
         {255, 188, 255, 188, 255, 188, 255, 188, 255, 188, 255, 188, 255, 188, 255, 188}}};
    if (!(fvm_gradient_init(&g) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    g.matrix.xx = g.matrix.yy = 2;
    g.matrix.x0 = 4.5;
    g.matrix.y0 = 8.5;
    stop(stops, 0, 255, 0, 0, 255);
    stop(stops + 1, 1, 0, 0, 255, 255);
    g.stops = stops;
    g.count = 2;
    surface.pixels = pixels;
    surface.width = surface.height = 16;
    surface.stride = 64;
    for (type = 0; type < 2; ++type)
    {
        for (mode = 0; mode < 3; ++mode)
        {
            g.type = (FVM_GradientType)type;
            memset(pixels, 0, sizeof(pixels));
            result =
                fvm_path_draw_gradient_interpolated(path, &surface, &g, 0, mode, FVM_GRADIENT_INTERPOLATION_LINEAR_RGB);
            if (!(result == FVM_OK || result == FVM_UNSUPPORTED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (result == FVM_UNSUPPORTED)
            {
                continue;
            }
            for (x = 0; x < 16; ++x)
            {
                unsigned char *p = pixels + 4 * (8 * 16 + x);
                if (!(p[0] == red[type][mode][x] && !p[1] && p[3] == 255))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            for (stroke = 0; stroke < 2; ++stroke)
            {
                memset(pixels, 0, sizeof(pixels));
                if (stroke)
                {
                    if (!(fvm_path_stroke_gradient(path, &surface, &g, 4, mode) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(fvm_path_draw_gradient_spread(path, &surface, &g, 0, mode) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                memcpy(legacy, pixels, sizeof(pixels));
                memset(pixels, 0, sizeof(pixels));
                if (stroke)
                {
                    if (!(fvm_path_stroke_gradient_interpolated(path, &surface, &g, 4, mode, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(fvm_path_draw_gradient_interpolated(path, &surface, &g, 0, mode, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(!memcmp(legacy, pixels, sizeof(pixels))))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    memset(pixels, 37, sizeof(pixels));
    memcpy(saved, pixels, sizeof(pixels));
    if (!(fvm_path_draw_gradient_interpolated(path, &surface, &g, 0, 0, UINT_MAX) == FVM_INVALID_ARGUMENT &&
          !memcmp(pixels, saved, sizeof(pixels))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_stroke_gradient_interpolated(path, &surface, &g, 4, 0, 2) == FVM_INVALID_ARGUMENT &&
          !memcmp(pixels, saved, sizeof(pixels))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_stroke_gradient_interpolated(path, &surface, &g, 0, 0, 1) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_draw_gradient_interpolated(path, &surface, &g, 0, UINT_MAX, 1) == FVM_INVALID_ARGUMENT &&
          !memcmp(pixels, saved, sizeof(pixels))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_stroke_gradient_interpolated(path, &surface, &g, HUGE_VAL, 0, 1) == FVM_INVALID_ARGUMENT &&
          !memcmp(pixels, saved, sizeof(pixels))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_stroke_gradient_interpolated(path, &surface, &g, 4097, 0, 1) == FVM_INVALID_ARGUMENT &&
          !memcmp(pixels, saved, sizeof(pixels))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    return 0;
}
