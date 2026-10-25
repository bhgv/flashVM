#include <stdio.h>
#include <stdlib.h>
#include "flashvm/raster.h"
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
#if defined(FVM_HAS_AGG) || defined(FVM_HAS_CAIRO)
    if (!(result == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (y = 0; y < 8; ++y)
    {
        for (x = 0; x < 8; ++x)
        {
            unsigned char *pixel = buffer + y * 40 + x * 4;
            if (x >= 2 && x < 6 && y >= 2 && y < 6)
            { /* AGG source-over rounding can lose one RGB unit at alpha 128. */
                if (!(pixel[0] >= 199 && pixel[0] <= 200))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(pixel[1] >= 99 && pixel[1] <= 100))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(pixel[2] >= 49 && pixel[2] <= 50))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(pixel[3] == 128))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(pixel[0] == 0 && pixel[3] == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        for (x = 32; x < 40; ++x)
        {
            if (!(buffer[y * 40 + x] == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
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
    if (!(fvm_raster_fill_polygon(&surface, rectangle, 4, color) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(buffer[3] == 255 && buffer[7 * 40 + 7 * 4 + 3] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    (void)x;
    (void)y;
    if (!(result == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    surface.stride = 31;
    if (!(fvm_raster_fill_polygon(&surface, rectangle, 4, color) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    surface.stride = 40;
    rectangle[0].x = 4097;
    if (!(fvm_raster_fill_polygon(&surface, rectangle, 4, color) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_raster_fill_polygon(&surface, rectangle, 2, color) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
