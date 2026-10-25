#define main vector_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "vector_paths.c"

#undef main
#include "flashvm/bitmap_paint.h"


static void pixel(
    const unsigned char *pixels, size_t stride, unsigned x, unsigned y, unsigned r, unsigned g, unsigned b, unsigned a)
{
    const unsigned char *p = pixels + y * stride + x * 4;
    if (!(p[0] == r && p[1] == g && p[2] == b && p[3] == a))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    FVM_Path *p = 0;
    FVM_BitmapPaint paint, invalid;
    unsigned char source_bytes[24] = {255, 0, 0,   255, 0,   255, 0,   255, 77, 77, 77, 77,
                                      0,   0, 255, 255, 255, 255, 255, 255, 77, 77, 77, 77};
    unsigned char pixels[16 * 72], before[sizeof(pixels)];
    FVM_RasterSurface source = {0, 2, 2, 12}, destination = {0, 16, 16, 72};
    FVM_Result result;
    unsigned x, y;
    source.pixels = source_bytes;
    destination.pixels = pixels;
    if (!(fvm_path_create(&p) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rectangle(p, 0, 16);
    if (!(fvm_bitmap_paint_init(0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_bitmap_paint_init(&paint) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    paint.source = &source;
    memset(pixels, 0, sizeof(pixels));
    result = fvm_path_draw_bitmap(p, &destination, &paint, 0);
    if (!(result == FVM_OK || result == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (result == FVM_OK)
    {
        pixel(pixels, 72, 0, 0, 255, 0, 0, 255);
        pixel(pixels, 72, 1, 0, 0, 255, 0, 255);
        pixel(pixels, 72, 2, 0, 255, 0, 0, 255);
        pixel(pixels, 72, 2, 1, 0, 0, 255, 255);
        for (y = 0; y < 16; ++y)
        {
            for (x = 64; x < 72; ++x)
            {
                if (!(pixels[y * 72 + x] == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        paint.repeat = 0;
        memset(pixels, 0, sizeof(pixels));
        if (!(fvm_path_draw_bitmap(p, &destination, &paint, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        pixel(pixels, 72, 5, 0, 0, 255, 0, 255);
        pixel(pixels, 72, 5, 5, 255, 255, 255, 255);
        paint.repeat = 1;
        paint.matrix.xx = -1;
        paint.matrix.x0 = 2;
        memset(pixels, 0, sizeof(pixels));
        if (!(fvm_path_draw_bitmap(p, &destination, &paint, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        pixel(pixels, 72, 0, 0, 0, 255, 0, 255);
        pixel(pixels, 72, 1, 0, 255, 0, 0, 255);
        if (!(fvm_bitmap_paint_init(&paint) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        paint.source = &source;
        paint.repeat = 0;
        paint.smoothing = 1;
        paint.matrix.x0 = -0.5;
        paint.matrix.y0 = 0;
        source.height = 1;
        source_bytes[3] = 0;
        source_bytes[4] = 0;
        source_bytes[5] = 0;
        source_bytes[6] = 255;
        memset(pixels, 0, sizeof(pixels));
        if (!(fvm_path_draw_bitmap(p, &destination, &paint, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        pixel(pixels, 72, 0, 0, 0, 0, 255, 128);
        paint.multiply[2] = 0;
        paint.add[0] = 255;
        memset(pixels, 0, sizeof(pixels));
        if (!(fvm_path_draw_bitmap(p, &destination, &paint, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        pixel(pixels, 72, 0, 0, 255, 0, 0, 128);
        if (!(fvm_bitmap_paint_init(&paint) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (y = 0; y < 16; ++y)
        {
            for (x = 0; x < 16; ++x)
            {
                unsigned char *v = pixels + y * 72 + x * 4;
                v[0] = (unsigned char)x;
                v[1] = v[2] = 0;
                v[3] = 255;
            }
        }
        paint.source = &destination;
        paint.matrix.x0 = 1;
        if (!(fvm_path_draw_bitmap(p, &destination, &paint, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        pixel(pixels, 72, 0, 0, 15, 0, 0, 255);
        pixel(pixels, 72, 1, 0, 0, 0, 0, 255);
        pixel(pixels, 72, 2, 0, 1, 0, 0, 255);
    }
    paint.source = &source;
    memset(pixels, 39, sizeof(pixels));
    memcpy(before, pixels, sizeof(pixels));
    invalid = paint;
    invalid.matrix.xx = invalid.matrix.xy = 0;
    if (!(fvm_path_draw_bitmap(p, &destination, &invalid, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    invalid = paint;
    invalid.repeat = 2;
    if (!(fvm_path_draw_bitmap(p, &destination, &invalid, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    invalid = paint;
    invalid.source = 0;
    if (!(fvm_path_draw_bitmap(p, &destination, &invalid, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    invalid = paint;
    invalid.add[0] = HUGE_VAL;
    if (!(fvm_path_draw_bitmap(p, &destination, &invalid, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(memcmp(before, pixels, sizeof(pixels)) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(p);
    return 0;
}
