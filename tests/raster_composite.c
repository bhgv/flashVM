#include "flashvm/raster.h"
#include <assert.h>
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
    assert(result == FVM_OK);
    color.r = 0;
    color.b = 255;
    assert(fvm_raster_fill_polygon(&surface, points, 4, color) == FVM_OK);
    center = pixels + 20 + 4;
    assert(center[0] >= 83 && center[0] <= 86);
    assert(center[1] == 0);
    assert(center[2] >= 169 && center[2] <= 172);
    assert(center[3] >= 191 && center[3] <= 193);
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
    assert(fvm_raster_fill_polygon(&surface, points, 4, color) == FVM_OK);
    assert(memcmp(before, pixels, sizeof(pixels)) == 0);
#else
    (void)center;
    (void)before;
    assert(result == FVM_UNSUPPORTED);
#endif
    return 0;
}
