#include <stdio.h>
#include <stdlib.h>
#include "flashvm/raster.h"
#include <string.h>


int main(void)
{
    unsigned char pixels[4 * 20], before[4 * 20];
    FVM_RasterSurface surface;
    FVM_RasterPoint points[4];
    FVM_RasterColor color;
    unsigned char *center;
    FVM_Result result;
    memset(pixels, 0, sizeof(pixels));
    surface.pixels = pixels;
    surface.width = surface.height = 4;
    surface.stride = 20;
    points[0].x = points[0].y = 1;
    points[1].x = 3;
    points[1].y = 1;
    points[2].x = points[2].y = 3;
    points[3].x = 1;
    points[3].y = 3;
    color.r = 255;
    color.g = color.b = 0;
    color.a = 128;
    result = fvm_raster_fill_polygon(&surface, points, 4, color);
#if defined(FVM_HAS_AGG) || defined(FVM_HAS_CAIRO)
    if (!(result == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    color.r = 0;
    color.b = 255;
    if (!(fvm_raster_fill_polygon(&surface, points, 4, color) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    center = pixels + 20 + 4;
    if (!(center[0] >= 83 && center[0] <= 86))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(center[1] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(center[2] >= 169 && center[2] <= 172))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(center[3] >= 191 && center[3] <= 193))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Transparent fills must preserve hidden RGB and padding exactly. */
    pixels[0] = 73;
    memcpy(before, pixels, sizeof(pixels));
    color.a = 0;
    points[0].x = points[0].y = -1;
    points[1].x = 5;
    points[1].y = -1;
    points[2].x = points[2].y = 5;
    points[3].x = -1;
    points[3].y = 5;
    if (!(fvm_raster_fill_polygon(&surface, points, 4, color) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(memcmp(before, pixels, sizeof(pixels)) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    (void)center;
    (void)before;
    if (!(result == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
