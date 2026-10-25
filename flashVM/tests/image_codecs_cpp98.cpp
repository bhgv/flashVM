#include <stdio.h>
#include <stdlib.h>
#include <cstddef>
#include <cstdio>
#include "png.h"
#include "gif_lib.h"
#include "jpeglib.h"
#include "nanosvg.h"
#include "cairo-svg.h"
#include "flashvm/svg.h"
#include <cstring>


int main()
{
    char xml[] = "<svg width='8' height='8'><path d='M0.123456789012345 1L7 1L7 7Z' fill='red'/></svg>";
    NSVGimage *image = nsvgParse(xml, "px", 96);
    if (!(image && image->shapes && image->shapes->paths))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(image->width == 8 && image->height == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(image->shapes->paths->pts[0] > 0.123456f))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(image->shapes->paths->pts[0] < 0.123458f))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    nsvgDelete(image);
    if (!(std::strcmp(png_get_libpng_ver(0), "1.6.59") == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(GIFLIB_MAJOR == 5 && GIFLIB_MINOR == 2 && GIFLIB_RELEASE == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(JPEG_LIB_VERSION == 90))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sizeof(GifBooleanType) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* All five headers and C entry points coexist without STL containers. */
    return 0;
}
