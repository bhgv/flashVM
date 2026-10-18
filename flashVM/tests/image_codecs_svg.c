#include "flashvm/svg.h"
#include "cairo.h"
#include "cairo-svg.h"
#include "image_codecs_memory.h"
#include <assert.h>

static cairo_status_t write_bytes(void *closure, const unsigned char *bytes, unsigned int count)
{
    TestImageBytes *out = (TestImageBytes *)closure;
    if (count > sizeof(out->data) - out->size)
        return CAIRO_STATUS_WRITE_ERROR;
    memcpy(out->data + out->size, bytes, count);
    out->size += count;
    return CAIRO_STATUS_SUCCESS;
}
static cairo_status_t read_bytes(void *closure, unsigned char *bytes, unsigned int count)
{
    TestImageBytes *input = (TestImageBytes *)closure;
    if (count > input->size - input->position)
        return CAIRO_STATUS_READ_ERROR;
    memcpy(bytes, input->data + input->position, count);
    input->position += count;
    return CAIRO_STATUS_SUCCESS;
}
int main(void)
{
    TestImageBytes svg_bytes, png_bytes;
    cairo_surface_t *svg_surface, *image;
    cairo_t *context;
    FVM_SVG *svg = 0;
    FVM_RasterSurface surface;
    unsigned char pixels[8 * 40];
    double width, height;
    unsigned int y, x;
    memset(&svg_bytes, 0, sizeof(svg_bytes));
    memset(&png_bytes, 0, sizeof(png_bytes));
    svg_surface = cairo_svg_surface_create_for_stream(write_bytes, &svg_bytes, 8, 8);
    assert(cairo_surface_status(svg_surface) == CAIRO_STATUS_SUCCESS);
    cairo_svg_surface_restrict_to_version(svg_surface, CAIRO_SVG_VERSION_1_1);
    /* Match NanoSVG's px/96dpi reader: Cairo otherwise writes dimensions in pt. */
    cairo_svg_surface_set_document_unit(svg_surface, CAIRO_SVG_UNIT_PX);
    context = cairo_create(svg_surface);
    cairo_set_source_rgb(context, 1, 0, 0);
    cairo_rectangle(context, 2, 2, 4, 4);
    cairo_fill(context);
    assert(cairo_status(context) == CAIRO_STATUS_SUCCESS);
    cairo_destroy(context);
    cairo_surface_finish(svg_surface);
    assert(cairo_surface_status(svg_surface) == CAIRO_STATUS_SUCCESS);
    cairo_surface_destroy(svg_surface);
    assert(svg_bytes.size > 20);
    assert(fvm_svg_read((const char *)svg_bytes.data, svg_bytes.size, &svg) == FVM_OK);
    assert(fvm_svg_size(svg, &width, &height) == FVM_OK);
    assert(width == 8 && height == 8);
    memset(pixels, 0x5a, sizeof(pixels));
    surface.pixels = pixels;
    surface.width = surface.height = 8;
    surface.stride = 40;
    assert(fvm_svg_rasterize(svg, &surface) == FVM_OK);
    assert(pixels[3] == 0);
    assert(pixels[3 * 40 + 3 * 4] == 255);
    assert(pixels[3 * 40 + 3 * 4 + 1] == 0);
    assert(pixels[3 * 40 + 3 * 4 + 3] == 255);
    for (y = 0; y < 8; ++y)
        for (x = 32; x < 40; ++x)
            assert(pixels[y * 40 + x] == 0x5a);
    fvm_svg_destroy(svg);

    /* PNG read/write functions are enabled inside the bundled Cairo library. */
    image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 8, 8);
    context = cairo_create(image);
    cairo_set_source_rgb(context, 0, 1, 0);
    cairo_paint(context);
    cairo_destroy(context);
    assert(cairo_surface_write_to_png_stream(image, write_bytes, &png_bytes) ==
           CAIRO_STATUS_SUCCESS);
    cairo_surface_destroy(image);
    assert(memcmp(png_bytes.data, "\211PNG\r\n\032\n", 8) == 0);
    image = cairo_image_surface_create_from_png_stream(read_bytes, &png_bytes);
    assert(cairo_surface_status(image) == CAIRO_STATUS_SUCCESS);
    assert(cairo_image_surface_get_width(image) == 8);
    assert(cairo_image_surface_get_height(image) == 8);
    cairo_surface_destroy(image);
    /* A rejected writer callback must propagate an error. */
    svg_bytes.size = sizeof(svg_bytes.data);
    svg_surface = cairo_svg_surface_create_for_stream(write_bytes, &svg_bytes, 8, 8);
    context = cairo_create(svg_surface);
    cairo_paint(context);
    cairo_destroy(context);
    cairo_surface_finish(svg_surface);
    assert(cairo_surface_status(svg_surface) == CAIRO_STATUS_WRITE_ERROR);
    cairo_surface_destroy(svg_surface);
    return 0;
}
