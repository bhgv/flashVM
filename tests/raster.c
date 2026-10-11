#include "flashvm/raster.h"
#include <assert.h>
#include <string.h>
int main(void)
{
    unsigned char buffer[8 * 40];
    FVM_RasterSurface surface;
    FVM_RasterPoint rectangle[4];
    FVM_RasterColor color;
    FVM_Result result;
    unsigned int x, y;
    memset(buffer, 0, sizeof(buffer));
    surface.pixels = buffer;
    surface.width = 8;
    surface.height = 8;
    surface.stride = 40; /* Padding must remain untouched. */
    rectangle[0].x = 2;
    rectangle[0].y = 2;
    rectangle[1].x = 6;
    rectangle[1].y = 2;
    rectangle[2].x = 6;
    rectangle[2].y = 6;
    rectangle[3].x = 2;
    rectangle[3].y = 6;
    color.r = 200;
    color.g = 100;
    color.b = 50;
    color.a = 128;
    result = fvm_raster_fill_polygon(&surface, rectangle, 4, color);
#ifdef FVM_HAS_AGG
    assert(result == FVM_OK);
    for (y = 0; y < 8; ++y)
    {
        for (x = 0; x < 8; ++x)
        {
            unsigned char *pixel = buffer + y * 40 + x * 4;
            if (x >= 2 && x < 6 && y >= 2 && y < 6)
            {
                /* AGG source-over rounding can lose one RGB unit at alpha 128. */
                assert(pixel[0] >= 199 && pixel[0] <= 200);
                assert(pixel[1] >= 99 && pixel[1] <= 100);
                assert(pixel[2] >= 49 && pixel[2] <= 50);
                assert(pixel[3] == 128);
            }
            else
                assert(pixel[0] == 0 && pixel[3] == 0);
        }
        for (x = 32; x < 40; ++x)
            assert(buffer[y * 40 + x] == 0);
    }
    /* Clip a polygon that crosses all four image boundaries. */
    rectangle[0].x = -10;
    rectangle[0].y = -10;
    rectangle[1].x = 10;
    rectangle[1].y = -10;
    rectangle[2].x = 10;
    rectangle[2].y = 10;
    rectangle[3].x = -10;
    rectangle[3].y = 10;
    color.a = 255;
    assert(fvm_raster_fill_polygon(&surface, rectangle, 4, color) == FVM_OK);
    assert(buffer[3] == 255 && buffer[7 * 40 + 7 * 4 + 3] == 255);
#else
    (void)x;
    (void)y;
    assert(result == FVM_UNSUPPORTED);
#endif
    surface.stride = 31;
    assert(fvm_raster_fill_polygon(&surface, rectangle, 4, color) == FVM_INVALID_ARGUMENT);
    surface.stride = 40;
    rectangle[0].x = 4097;
    assert(fvm_raster_fill_polygon(&surface, rectangle, 4, color) == FVM_INVALID_ARGUMENT);
    assert(fvm_raster_fill_polygon(&surface, rectangle, 2, color) == FVM_INVALID_ARGUMENT);
    return 0;
}
