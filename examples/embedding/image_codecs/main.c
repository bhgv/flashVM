#include "flashvm/svg.h"
#include "png.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    const char *xml = "<svg width='8' height='8'><circle cx='4' cy='4' r='3' fill='red'/></svg>";
    FVM_SVG *svg = 0;
    FVM_RasterSurface surface;
    unsigned char rgba[8 * 8 * 4], png[4096];
    png_image image;
    png_alloc_size_t size = sizeof(png);
    if (fvm_svg_read(xml, strlen(xml), &svg) != FVM_OK)
        return 1;
    surface.pixels = rgba;
    surface.width = surface.height = 8;
    surface.stride = 32;
    if (fvm_svg_rasterize(svg, &surface) != FVM_OK)
    {
        fvm_svg_destroy(svg);
        return 2;
    }
    fvm_svg_destroy(svg);
    memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;
    image.width = image.height = 8;
    image.format = PNG_FORMAT_RGBA;
    if (!png_image_write_to_memory(&image, png, &size, 0, rgba, 0, 0))
        return 3;
    printf("SVG decoded to 8x8 RGBA; PNG encoded to %lu bytes\n", (unsigned long)size);
    return 0;
}
