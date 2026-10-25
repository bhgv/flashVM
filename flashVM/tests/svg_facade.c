#include <stdio.h>
#include <stdlib.h>
#include "flashvm/svg.h"
#include <string.h>


int main(void)
{
    const char *source = "<svg width='8' height='8'><rect width='8' height='8' fill='blue'/></svg>";
    FVM_SVG *svg = (FVM_SVG *)1;
#ifdef FVM_HAS_IMAGE_CODECS
    const char *bad = "<svg width='5000' height='8'/>";
    unsigned char pixels[64 * 4];
    FVM_RasterSurface surface;
    double width, height;
    if (!(fvm_svg_read(0, 8, &svg) == FVM_INVALID_ARGUMENT && svg == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_svg_read(source, strlen(source), 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_svg_read(source, 4 * 1024 * 1024 + 1, &svg) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_svg_read("bad XML", 7, &svg) == FVM_INVALID_ARGUMENT && svg == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_svg_read(bad, strlen(bad), &svg) == FVM_INVALID_ARGUMENT && svg == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_svg_read(source, strlen(source), &svg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_svg_size(svg, &width, &height) == FVM_OK && width == 8 && height == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    surface.pixels = pixels;
    surface.width = surface.height = 8;
    surface.stride = 32;
    if (!(fvm_svg_rasterize(svg, &surface) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[2] == 255 && pixels[3] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    surface.stride = 31;
    if (!(fvm_svg_rasterize(svg, &surface) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_svg_destroy(svg);
#else
    if (!(fvm_svg_read(source, strlen(source), &svg) == FVM_UNSUPPORTED && svg == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_svg_size(0, 0, 0) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_svg_rasterize(0, 0) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    fvm_svg_destroy(0);
    return 0;
}
