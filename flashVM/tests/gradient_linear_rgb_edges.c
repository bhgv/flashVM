/* Independent numeric tables cover dark transfer branches, duplicate stops, alpha and transform order. */
#define main gradient_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "gradient.c"

#undef main


int main(void)
{
    FVM_Path *path = box();
    FVM_Gradient g;
    FVM_GradientStop stops[4];
    FVM_RasterSurface surface;
    unsigned char pixels[1024], *p;
    FVM_Result result;
    unsigned i, j;
    const unsigned char pairs[4][2] = {{0, 20}, {8, 128}, {10, 11}, {64, 192}},
                        expected[4][5] = {
                            {0, 6, 11, 16, 20}, {8, 67, 93, 112, 128}, {10, 10, 11, 11, 11}, {64, 115, 146, 171, 192}};
    if (!(fvm_gradient_init(&g) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    g.matrix.xx = g.matrix.yy = 2;
    g.matrix.x0 = 4.5;
    g.matrix.y0 = 8.5;
    g.stops = stops;
    g.count = 2;
    surface.pixels = pixels;
    surface.width = surface.height = 16;
    surface.stride = 64;
    for (i = 0; i < 4; ++i)
    {
        stop(stops, 0, pairs[i][0], pairs[i][0], pairs[i][0], 255);
        stop(stops + 1, 1, pairs[i][1], pairs[i][1], pairs[i][1], 255);
        memset(pixels, 0, sizeof(pixels));
        result = fvm_path_draw_gradient_interpolated(path, &surface, &g, 0, 0, 1);
        if (!(result == FVM_OK || result == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (result == FVM_UNSUPPORTED)
        {
            continue;
        }
        for (j = 0; j < 5; ++j)
        {
            p = pixels + 4 * (8 * 16 + 2 + j);
            if (!(p[0] == expected[i][j] && p[1] == p[0] && p[2] == p[0] && p[3] == 255))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (result == FVM_OK)
    {
        stop(stops, 0, 0, 0, 0, 0);
        stop(stops + 1, 1, 255, 255, 255, 255);
        memset(pixels, 0, sizeof(pixels));
        if (!(fvm_path_draw_gradient_interpolated(path, &surface, &g, 0, 0, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        p = pixels + 4 * (8 * 16 + 4);
        if (!(p[0] == 188 && p[1] == 188 && p[2] == 188 && p[3] == 128))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < 256; ++i)
        {
            pixels[4 * i] = 20;
            pixels[4 * i + 1] = 40;
            pixels[4 * i + 2] = 60;
            pixels[4 * i + 3] = 255;
        }
        if (!(fvm_path_draw_gradient_interpolated(path, &surface, &g, 0, 0, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(p[0] == 104 && p[1] == 114 && p[2] == 124 && p[3] == 255))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        g.multiply[0] = 0.5;
        g.add[0] = 10;
        g.multiply[3] = 0.5;
        g.add[3] = 20;
        memset(pixels, 0, sizeof(pixels));
        if (!(fvm_path_draw_gradient_interpolated(path, &surface, &g, 0, 0, 1) == FVM_OK && p[0] == 104 && p[3] == 84))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        g.multiply[0] = g.multiply[3] = 1;
        g.add[0] = g.add[3] = 0;
        stop(stops, 0, 8, 8, 8, 255);
        stop(stops + 1, 0, 20, 20, 20, 255);
        stop(stops + 2, 0.5, 128, 128, 128, 255);
        stop(stops + 3, 1, 192, 192, 192, 255);
        g.count = 4;
        memset(pixels, 0, sizeof(pixels));
        if (!(fvm_path_draw_gradient_interpolated(path, &surface, &g, 0, 0, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(pixels[4 * (8 * 16)] == 8 && pixels[4 * (8 * 16 + 2)] == 20 && pixels[4 * (8 * 16 + 4)] == 128 &&
              pixels[4 * (8 * 16 + 6)] == 192))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        g.count = 2;
        stop(stops, 0, 128, 128, 128, 255);
        stop(stops + 1, 1, 128, 128, 128, 255);
        g.add[0] = 0.5;
        memset(pixels, 0, sizeof(pixels));
        if (!(fvm_path_draw_gradient_interpolated(path, &surface, &g, 0, 0, 1) == FVM_OK && p[0] == 129 &&
              p[1] == 128 && p[2] == 128))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_path_destroy(path);
    return 0;
}
