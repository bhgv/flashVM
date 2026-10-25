#include <stdio.h>
#include <stdlib.h>
#include "swf_stroke_caps_fixture.hpp"


int main()
{
    for (unsigned type = 0; type < 4; ++type)
    {
        Bytes b = caps_shape(type, 2, 1, 80, true, true);
        FVM_SWFShape out;
        for (size_t n = 0; n < b.size(); ++n)
        {
            out.id = 777;
            if (!(fvm_swf_read_shape(b.begin(), n, 83, out) == FVM_INVALID_ARGUMENT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(out.id == 777))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        size_t at = 0;
        Bytes raw = bs_shape(0x42, 80, false, false, 0, 7, &at);
        b = caps_shape(type, 2, 1);
        b[at + 3] = 1;
        if (!(fvm_swf_read_shape(b.begin(), b.size(), 83, out) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        b[at + 3] = 2;
        b[at + 2] = (unsigned char)((2 << 6) | (2 << 4) | 8);
        if (!(fvm_swf_read_shape(b.begin(), b.size(), 83, out) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    unsigned char pixels[1024], before[1024], rgba[4] = {255, 0, 0, 255};
    std::memset(pixels, 71, sizeof(pixels));
    std::memcpy(before, pixels, sizeof(pixels));
    FVM_RasterSurface dst = {pixels, 16, 16, 64}, mask_source = {before, 16, 16, 64}, image = {rgba, 1, 1, 4};
    FVM_Path *path = bs_path();
    FVM_Gradient gradient;
    FVM_GradientStop stop;
    if (!(fvm_gradient_init(&gradient) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::memset(&stop, 0, sizeof(stop));
    stop.color.a = 255;
    gradient.stops = &stop;
    gradient.count = 1;
    FVM_BitmapPaint bitmap;
    if (!(fvm_bitmap_paint_init(&bitmap) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bitmap.source = &image;
    FVM_RasterColor red = {255, 0, 0, 255};
    const unsigned bad[][2] = {{3, 0}, {0, 2}, {~0U, 0}, {0, ~0U}};
    for (unsigned i = 0; i < 4; ++i)
    {
        if (!(fvm_path_draw_quality(path, &dst, red, 4, 0, true, bad[i][0], bad[i][1]) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_path_mask_width_quality(path, &dst, &mask_source, 4, 0, true, bad[i][0], bad[i][1]) ==
              FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_path_gradient_focal_quality(path, &dst, &gradient, 4, 0, 0, 0, 0, true, bad[i][0], bad[i][1]) ==
              FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_path_bitmap_paint_quality(path, &dst, &bitmap, 4, 0, true, bad[i][0], bad[i][1]) ==
              FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(std::memcmp(pixels, before, sizeof(pixels)) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    return 0;
}
