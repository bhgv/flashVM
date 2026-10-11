#include "flashvm/svg.h"
#include <assert.h>
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
    assert(fvm_svg_read(0, 8, &svg) == FVM_INVALID_ARGUMENT && svg == 0);
    assert(fvm_svg_read(source, strlen(source), 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_svg_read(source, 4 * 1024 * 1024 + 1, &svg) == FVM_INVALID_ARGUMENT);
    assert(fvm_svg_read("bad XML", 7, &svg) == FVM_INVALID_ARGUMENT && svg == 0);
    assert(fvm_svg_read(bad, strlen(bad), &svg) == FVM_INVALID_ARGUMENT && svg == 0);
    assert(fvm_svg_read(source, strlen(source), &svg) == FVM_OK);
    assert(fvm_svg_size(svg, &width, &height) == FVM_OK && width == 8 && height == 8);
    surface.pixels = pixels;
    surface.width = surface.height = 8;
    surface.stride = 32;
    assert(fvm_svg_rasterize(svg, &surface) == FVM_OK);
    assert(pixels[2] == 255 && pixels[3] == 255);
    surface.stride = 31;
    assert(fvm_svg_rasterize(svg, &surface) == FVM_INVALID_ARGUMENT);
    fvm_svg_destroy(svg);
#else
    assert(fvm_svg_read(source, strlen(source), &svg) == FVM_UNSUPPORTED && svg == 0);
    assert(fvm_svg_size(0, 0, 0) == FVM_UNSUPPORTED);
    assert(fvm_svg_rasterize(0, 0) == FVM_UNSUPPORTED);
#endif
    fvm_svg_destroy(0);
    return 0;
}
