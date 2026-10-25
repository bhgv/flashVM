/* C89 analytic pixel tables cover signed phases, seams, radial periods and legacy PAD bytes. */
#define main gradient_pad_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "gradient.c"

#undef main


int main(void)
{
    FVM_Path *path = box();
    FVM_Gradient g;
    FVM_GradientStop stops[2];
    FVM_RasterSurface surface;
    unsigned char pixels[1024], legacy[1024], saved[1024];
    FVM_Result result;
    unsigned int mode, type, x, shift;
    const unsigned char expected[2][3][16] = {
        {{255, 255, 255, 191, 128, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
         {128, 191, 255, 191, 128, 64, 0, 64, 128, 191, 255, 191, 128, 64, 0, 64},
         {128, 64, 255, 191, 128, 64, 255, 191, 128, 64, 255, 191, 128, 64, 255, 191}},
        {{0, 0, 0, 128, 255, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
         {255, 128, 0, 128, 255, 128, 0, 128, 255, 128, 0, 128, 255, 128, 0, 128},
         {255, 128, 255, 128, 255, 128, 255, 128, 255, 128, 255, 128, 255, 128, 255, 128}}};
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
            g.matrix.x0 = 4.5;
            memset(pixels, 0, sizeof(pixels));
            result = fvm_path_draw_gradient_spread(path, &surface, &g, 0, (FVM_GradientSpread)mode);
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
                if (!(p[0] == expected[type][mode][x] && p[1] == 0 && near(p[2], 255 - expected[type][mode][x]) &&
                      p[3] == 255))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!mode)
            {
                memcpy(legacy, pixels, sizeof(pixels));
                memset(pixels, 0, sizeof(pixels));
                if (!(fvm_path_draw_gradient(path, &surface, &g, 0) == FVM_OK &&
                      !memcmp(legacy, pixels, sizeof(pixels))))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!type && mode)
            {
                memcpy(legacy, pixels, sizeof(pixels));
                for (shift = 0; shift < 2; ++shift)
                {
                    g.matrix.x0 = 4.5 + (shift ? -400000 : 400000);
                    memset(pixels, 0, sizeof(pixels));
                    if (!(fvm_path_draw_gradient_spread(path, &surface, &g, 0, (FVM_GradientSpread)mode) == FVM_OK &&
                          !memcmp(legacy, pixels, sizeof(pixels))))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
    memset(pixels, 37, sizeof(pixels));
    memcpy(saved, pixels, sizeof(pixels));
    if (!(fvm_path_draw_gradient_spread(path, &surface, &g, 0, (FVM_GradientSpread)99) == FVM_INVALID_ARGUMENT &&
          !memcmp(saved, pixels, sizeof(pixels))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    g.matrix.xx = g.matrix.xy = 0;
    if (!(fvm_path_draw_gradient_spread(path, &surface, &g, 0, FVM_GRADIENT_SPREAD_REPEAT) == FVM_INVALID_ARGUMENT &&
          !memcmp(saved, pixels, sizeof(pixels))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    return 0;
}
