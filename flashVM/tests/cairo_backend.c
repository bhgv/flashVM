#include <assert.h>
#ifdef FVM_HAS_CAIRO
#include "cairo.h"
#include <string.h>
#endif

int main(void)
{
#ifdef FVM_HAS_CAIRO
    cairo_surface_t *image, *recording;
    cairo_t *context;
    unsigned int pixel;
    /* Test the pinned implementation and scalar Pixman compositing path. */
    assert(cairo_version() == 11600);
    assert(strcmp(cairo_version_string(), "1.16.0") == 0);
    image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 4, 4);
    context = cairo_create(image);
    cairo_set_source_rgb(context, 0.25, 0.25, 0.25);
    cairo_paint(context);
    cairo_set_operator(context, CAIRO_OPERATOR_SOFT_LIGHT);
    cairo_set_source_rgb(context, 0.75, 0.75, 0.75);
    cairo_paint(context);
    assert(cairo_status(context) == CAIRO_STATUS_SUCCESS);
    cairo_destroy(context);
    cairo_surface_flush(image);
    memcpy(&pixel, cairo_image_surface_get_data(image), 4);
    assert((pixel >> 24) == 255);
    assert((pixel & 255) >= 94 && (pixel & 255) <= 98);
    /* Recording support is included without a display server or font library. */
    recording = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, 0);
    context = cairo_create(recording);
    cairo_rectangle(context, 0, 0, 4, 4);
    cairo_set_source_rgb(context, 1, 0, 0);
    cairo_fill(context);
    assert(cairo_status(context) == CAIRO_STATUS_SUCCESS);
    cairo_destroy(context);
    context = cairo_create(image);
    cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(context, recording, 0, 0);
    cairo_paint(context);
    assert(cairo_status(context) == CAIRO_STATUS_SUCCESS);
    cairo_destroy(context);
    cairo_surface_flush(image);
    memcpy(&pixel, cairo_image_surface_get_data(image), 4);
    assert(pixel == 0xffff0000U);
    cairo_surface_destroy(recording);
    cairo_surface_destroy(image);
#endif
    return 0;
}
