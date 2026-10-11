#include "fixtures/font_data.h"
#include <cassert>
#include <new>
#include <ft2build.h>
#include FT_FREETYPE_H
#ifdef FVM_HAS_AGG
#include "agg_font_freetype.h"
#endif
#if defined(FVM_HAS_CAIRO) || defined(FVM_HAS_IMAGE_CODECS)
#include "cairo.h"
#include "cairo-ft.h"
struct FontOwner
{
    FT_Library library;
    FT_Face face;
};
static void release_font(void *value)
{
    FontOwner *owner = (FontOwner *)value;
    FT_Done_Face(owner->face);
    FT_Done_FreeType(owner->library);
    delete owner;
}
static cairo_user_data_key_t font_owner_key;
#endif
int main()
{
#ifdef FVM_HAS_AGG
    agg::font_engine_freetype_int32 engine;
    assert(engine.load_font("FlashVM fixture", 0, agg::glyph_ren_outline,
                            (const char *)fvm_test_font, (long)sizeof(fvm_test_font)));
    assert(engine.height(40));
    engine.hinting(false);
    assert(engine.prepare_glyph('B'));
    assert(engine.data_size() > 0 && engine.advance_x() == 24);
#endif
#if defined(FVM_HAS_CAIRO) || defined(FVM_HAS_IMAGE_CODECS)
    FontOwner *owner = new FontOwner;
    assert(FT_Init_FreeType(&owner->library) == 0);
    assert(FT_New_Memory_Face(owner->library, fvm_test_font, sizeof(fvm_test_font), 0,
                              &owner->face) == 0);
    cairo_font_face_t *font =
        cairo_ft_font_face_create_for_ft_face(owner->face, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
    assert(cairo_font_face_status(font) == CAIRO_STATUS_SUCCESS);
    assert(cairo_font_face_set_user_data(font, &font_owner_key, owner, release_font) ==
           CAIRO_STATUS_SUCCESS);
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 64, 64);
    cairo_t *cr = cairo_create(surface);
    cairo_set_font_face(cr, font);
    cairo_set_font_size(cr, 40);
    cairo_font_options_t *options = cairo_font_options_create();
    cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_NONE);
    cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);
    cairo_set_font_options(cr, options);
    cairo_font_options_destroy(options);
    cairo_glyph_t glyph = {7, 0, 40};
    cairo_glyph_path(cr, &glyph, 1);
    double x0, y0, x1, y1;
    cairo_path_extents(cr, &x0, &y0, &x1, &y1);
    assert(x0 >= 3.9 && x0 <= 4.1 && x1 >= 19.9 && x1 <= 20.1);
    assert(y0 >= 3.9 && y0 <= 4.1 && y1 >= 39.9 && y1 <= 40.1);
    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_fill_preserve(cr);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);
    assert(cairo_status(cr) == CAIRO_STATUS_SUCCESS);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cairo_font_face_destroy(font);
    /* Release Cairo's cached scaled-font references before its FreeType owner. */
    cairo_debug_reset_static_data();
#endif
    return 0;
}
