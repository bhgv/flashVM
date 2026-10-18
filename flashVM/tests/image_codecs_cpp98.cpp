#include <cstddef>
#include <cstdio>
#include "png.h"
#include "gif_lib.h"
#include "jpeglib.h"
#include "nanosvg.h"
#include "cairo-svg.h"
#include "flashvm/svg.h"
#include <cassert>
#include <cstring>

int main()
{
    char xml[] =
        "<svg width='8' height='8'><path d='M0.123456789012345 1L7 1L7 7Z' fill='red'/></svg>";
    NSVGimage *image = nsvgParse(xml, "px", 96);
    assert(image && image->shapes && image->shapes->paths);
    assert(image->width == 8 && image->height == 8);
    assert(image->shapes->paths->pts[0] > 0.123456f);
    assert(image->shapes->paths->pts[0] < 0.123458f);
    nsvgDelete(image);
    assert(std::strcmp(png_get_libpng_ver(0), "1.6.59") == 0);
    assert(GIFLIB_MAJOR == 5 && GIFLIB_MINOR == 2 && GIFLIB_RELEASE == 2);
    assert(JPEG_LIB_VERSION == 90);
    assert(sizeof(GifBooleanType) == 1);
    /* All five headers and C entry points coexist without STL containers. */
    return 0;
}
