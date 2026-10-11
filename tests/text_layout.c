#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifdef FVM_HAS_FONTS
static void close_layout(FVM_TextLayout **layout)
{
    fvm_text_layout_destroy(*layout);
    *layout = 0;
}
static void check_visual(FVM_Font *font, const char *text, const unsigned int *codes,
                         const size_t *bytes, size_t count, FVM_TextDirection requested)
{
    unsigned int visual[32];
    size_t map[32], i;
    unsigned char levels[32];
    FVM_TextDirection direction = requested;
    FVM_TextLayoutOptions options = {0, 0, FVM_TEXT_AUTO, FVM_TEXT_ALIGN_START, 0};
    FVM_TextLayout *layout = 0;
    FVM_TextLayoutMetrics metrics;
    FVM_TextLayoutGlyph g;
    options.direction = requested;
    assert(count <= 32);
    assert(fvm_text_bidi(codes, count, &direction, visual, map, levels) == FVM_OK);
    assert(fvm_text_layout_create(font, text, strlen(text), &options, &layout) == FVM_OK);
    assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.glyph_count == count);
    for (i = 0; i < count; ++i)
    {
        assert(fvm_text_layout_glyph(layout, i, &g) == FVM_OK);
        assert(g.shaped.cluster == bytes[map[i]]);
        assert(g.shaped.glyph == fvm_font_char(font, visual[i]));
    }
    close_layout(&layout);
}
static void bidi_corpus(FVM_Font *font)
{
    unsigned int seed = 1729, codes[16];
    size_t bytes[16], sample, i, size;
    char text[33];
    for (sample = 0; sample < 128; ++sample)
    {
        size = 0;
        for (i = 0; i < 16; ++i)
        {
            seed = seed * 1664525u + 1013904223u;
            codes[i] = (seed >> 24) % 4;
            codes[i] = codes[i] == 0 ? 65 : codes[i] == 1 ? 32 : codes[i] == 2 ? 0x5d0 : 0x5d1;
            bytes[i] = size;
            if (codes[i] < 128)
                text[size++] = (char)codes[i];
            else
            {
                text[size++] = (char)0xd7;
                text[size++] = (char)(codes[i] == 0x5d0 ? 0x90 : 0x91);
            }
        }
        text[size] = 0;
        check_visual(font, text, codes, bytes, 16, FVM_TEXT_AUTO);
        check_visual(font, text, codes, bytes, 16, FVM_TEXT_LTR);
        check_visual(font, text, codes, bytes, 16, FVM_TEXT_RTL);
    }
}
#endif
int main(void)
{
    FVM_TextLayout *layout = (FVM_TextLayout *)1;
    FVM_TextLayoutMetrics metrics;
    FVM_TextLayoutLine line;
    FVM_TextLayoutGlyph g;
    FVM_Path *path = (FVM_Path *)1;
    assert(fvm_text_layout_create(0, 0, 0, 0, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_text_layout_metrics(0, &metrics) == FVM_INVALID_ARGUMENT);
    assert(fvm_text_layout_line(0, 0, &line) == FVM_INVALID_ARGUMENT);
    assert(fvm_text_layout_glyph(0, 0, &g) == FVM_INVALID_ARGUMENT);
    assert(fvm_text_layout_path(0, &path) == FVM_INVALID_ARGUMENT && !path);
    assert(fvm_text_layout_glyph_path(0, 0, &path) == FVM_INVALID_ARGUMENT && !path);
    fvm_text_layout_destroy(0);
#ifndef FVM_HAS_FONTS
    assert(fvm_text_layout_create(0, "A", 1, 0, &layout) == FVM_UNSUPPORTED && !layout);
#else
    {
        FVM_Font *font = 0;
        FVM_TextLayoutOptions options = {0, 0, FVM_TEXT_AUTO, FVM_TEXT_ALIGN_START, "en"};
        FVM_Path *copy = 0;
        FVM_PathBounds bounds;
        FVM_PathCommand command;
        FVM_Affine shift = {1, 0, 0, 1, 100, 0};
        unsigned int codes[] = {65, 32, 0x5d0, 0x5d1, 32, 86};
        size_t bytes[] = {0, 1, 2, 4, 6, 7};
        unsigned int rtl_codes[] = {0x5d0, 0x5d1, 32, 65, 86};
        size_t rtl_bytes[] = {0, 2, 4, 5, 6};
        char large[4097];
        unsigned char pixels[128 * 128 * 4];
        FVM_RasterSurface surface = {0, 128, 128, 512};
        FVM_RasterColor color = {255, 0, 0, 255};
        surface.pixels = pixels;
        assert(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK);
        assert(fvm_text_layout_create(font, 0, 0, 0, &layout) == FVM_OK);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK);
        assert(metrics.line_count == 1 && metrics.glyph_count == 0 && metrics.height == 44);
        assert(fvm_text_layout_line(layout, 0, &line) == FVM_OK && line.baseline == 36);
        assert(fvm_text_layout_path(layout, &path) == FVM_OK && fvm_path_count(path) == 0);
        fvm_path_destroy(path);
        close_layout(&layout);
        assert(fvm_text_layout_create(font, "AV fi", 5, 0, &layout) == FVM_OK);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.glyph_count == 4);
        assert(fabs(metrics.advance_width - 84.8) < 0.02 && metrics.height == 44);
        assert(fvm_text_layout_glyph(layout, 3, &g) == FVM_OK && g.shaped.glyph == 6 &&
               g.shaped.cluster == 3);
        close_layout(&layout);
        options.max_width = 57;
        assert(fvm_text_layout_create(font, "AV fi", 5, &options, &layout) == FVM_OK);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.line_count == 2);
        assert(fvm_text_layout_line(layout, 0, &line) == FVM_OK && line.byte_end == 3 &&
               line.byte_next == 3);
        assert(fabs(line.advance - 56.8) < 0.02);
        assert(fvm_text_layout_line(layout, 1, &line) == FVM_OK && line.byte_start == 3 &&
               line.baseline == 84);
        close_layout(&layout);
        options.max_width = 24;
        assert(fvm_text_layout_create(font, "fi", 2, &options, &layout) == FVM_OK);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.line_count == 2);
        assert(fvm_text_layout_glyph(layout, 0, &g) == FVM_OK && g.shaped.glyph == 4);
        assert(fvm_text_layout_glyph(layout, 1, &g) == FVM_OK && g.shaped.glyph == 5);
        close_layout(&layout);
        options.max_width = 28;
        assert(fvm_text_layout_create(font, "fi", 2, &options, &layout) == FVM_OK);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.line_count == 1 &&
               metrics.glyph_count == 1);
        close_layout(&layout);
        options.max_width = 10;
        assert(fvm_text_layout_create(font, "A\xcc\x81V", 4, &options, &layout) == FVM_OK);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.line_count == 2);
        assert(fvm_text_layout_line(layout, 0, &line) == FVM_OK && line.byte_end == 3 &&
               line.advance > 10);
        close_layout(&layout);
        options.max_width = 0;
        assert(fvm_text_layout_create(font, "A\r\n\nV\xe2\x80\xa8", 8, &options, &layout) ==
               FVM_OK);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.line_count == 4 &&
               metrics.height == 188);
        assert(fvm_text_layout_line(layout, 0, &line) == FVM_OK && line.byte_end == 1 &&
               line.byte_next == 3);
        assert(fvm_text_layout_line(layout, 1, &line) == FVM_OK && line.byte_start == 3 &&
               line.byte_end == 3 && line.byte_next == 4);
        assert(fvm_text_layout_line(layout, 3, &line) == FVM_OK && line.byte_start == 8 &&
               line.glyph_count == 0);
        close_layout(&layout);
        bidi_corpus(font);
        check_visual(font, "A \xd7\x90\xd7\x91 V", codes, bytes, 6, FVM_TEXT_AUTO);
        /* No AV pair: split with space to keep this reference free of kerning. */
        rtl_codes[4] = 65;
        check_visual(font, "\xd7\x90\xd7\x91 AA", rtl_codes, rtl_bytes, 5, FVM_TEXT_AUTO);
        assert(fvm_text_layout_create(font, "fi \xd8\xa8\xd8\xa8\xd8\xa8", 9, 0, &layout) ==
               FVM_OK);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.glyph_count == 5);
        assert(fvm_text_layout_glyph(layout, 0, &g) == FVM_OK && g.shaped.glyph == 6);
        assert(fvm_text_layout_glyph(layout, 2, &g) == FVM_OK && g.shaped.glyph == 15 &&
               g.shaped.cluster == 7 && g.bidi_level == 1);
        assert(fvm_text_layout_glyph(layout, 4, &g) == FVM_OK && g.shaped.glyph == 13 &&
               g.shaped.cluster == 3);
        close_layout(&layout);
        assert(fvm_text_layout_create(font,
                                      "A\xe2\x80\xaeVV\xe2\x80\xac"
                                      "A",
                                      10, 0, &layout) == FVM_OK);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.glyph_count == 4);
        assert(fvm_text_layout_glyph(layout, 1, &g) == FVM_OK && g.shaped.cluster == 5);
        assert(fvm_text_layout_glyph(layout, 2, &g) == FVM_OK && g.shaped.cluster == 4);
        close_layout(&layout);
        assert(fvm_text_layout_create(font, "A\xe2\x81\xa7\xd7\x90\xd7\x91\xe2\x81\xa9V", 12, 0,
                                      &layout) == FVM_OK);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.glyph_count == 4);
        assert(fvm_text_layout_glyph(layout, 1, &g) == FVM_OK && g.shaped.cluster == 6);
        assert(fvm_text_layout_glyph(layout, 2, &g) == FVM_OK && g.shaped.cluster == 4);
        close_layout(&layout);
        options.max_width = 48;
        assert(fvm_text_layout_create(font, "\xd8\xa8\xd8\xa8\xd8\xa8", 6, &options, &layout) ==
               FVM_OK);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.line_count == 2);
        assert(fvm_text_layout_glyph(layout, 0, &g) == FVM_OK && g.shaped.glyph == 15);
        assert(fvm_text_layout_glyph(layout, 1, &g) == FVM_OK && g.shaped.glyph == 13);
        assert(fvm_text_layout_glyph(layout, 2, &g) == FVM_OK && g.shaped.glyph == 12);
        assert(fvm_text_layout_line(layout, 1, &line) == FVM_OK && line.direction == FVM_TEXT_RTL &&
               line.x == 24);
        close_layout(&layout);
        options.max_width = 100;
        options.alignment = FVM_TEXT_ALIGN_CENTER;
        options.line_height = 60;
        assert(fvm_text_layout_create(font, "A\nB", 3, &options, &layout) == FVM_OK);
        assert(fvm_text_layout_line(layout, 0, &line) == FVM_OK && line.x == 38 &&
               line.baseline == 36);
        assert(fvm_text_layout_line(layout, 1, &line) == FVM_OK && line.x == 38 &&
               line.baseline == 96);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.height == 104);
        close_layout(&layout);
        assert(fvm_text_layout_create(font, "\xc0\xaf", 2, 0, &layout) == FVM_INVALID_ARGUMENT &&
               !layout);
        assert(fvm_text_layout_create(font, "\xed\xa0\x80", 3, 0, &layout) ==
                   FVM_INVALID_ARGUMENT &&
               !layout);
        assert(fvm_text_layout_create(font, "\xf4\x90\x80\x80", 4, 0, &layout) ==
                   FVM_INVALID_ARGUMENT &&
               !layout);
        assert(fvm_text_layout_create(font, "\xf0\x90", 2, 0, &layout) == FVM_INVALID_ARGUMENT &&
               !layout);
        assert(fvm_text_layout_create(font, "\t", 1, 0, &layout) == FVM_UNSUPPORTED && !layout);
        assert(fvm_text_layout_create(font, "A\xc2\xadV", 4, 0, &layout) == FVM_UNSUPPORTED &&
               !layout);
        options.max_width = -1;
        assert(fvm_text_layout_create(font, "A", 1, &options, &layout) == FVM_INVALID_ARGUMENT &&
               !layout);
        options.max_width = HUGE_VAL;
        assert(fvm_text_layout_create(font, "A", 1, &options, &layout) == FVM_INVALID_ARGUMENT &&
               !layout);
        memset(large, 'A', sizeof(large));
        assert(fvm_text_layout_create(font, large, sizeof(large), 0, &layout) == FVM_UNSUPPORTED &&
               !layout);
        options.max_width = 100;
        options.direction = (FVM_TextDirection)99;
        assert(fvm_text_layout_create(font, "A", 1, &options, &layout) == FVM_INVALID_ARGUMENT &&
               !layout);
        options.direction = FVM_TEXT_AUTO;
        options.alignment = (FVM_TextAlignment)99;
        assert(fvm_text_layout_create(font, "A", 1, &options, &layout) == FVM_INVALID_ARGUMENT &&
               !layout);
        options.alignment = FVM_TEXT_ALIGN_START;
        options.max_width = 1000000;
        /* Explicit shaping-work limit: fail transactionally rather than letting
         * repeated width trials monopolize an embedding host indefinitely. */
        {
            size_t n;
            for (n = 0; n < 4096; ++n)
                large[n] = (n & 1) ? ' ' : 'A';
        }
        assert(fvm_text_layout_create(font, large, 4096, &options, &layout) == FVM_UNSUPPORTED &&
               !layout);
        /* Ownership and editable copies, including exact quadratic ink bounds. */
        assert(fvm_text_layout_create(font, "B", 1, 0, &layout) == FVM_OK);
        fvm_font_destroy(font);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.ink.x_max == 20 &&
               metrics.advance_width == 24);
        assert(fvm_text_layout_path(layout, &path) == FVM_OK);
        assert(fvm_text_layout_glyph_path(layout, 0, &copy) == FVM_OK);
        assert(fvm_path_transform(copy, &shift) == FVM_OK);
        fvm_path_destroy(copy);
        assert(fvm_text_layout_glyph_path(layout, 1, &copy) == FVM_INVALID_ARGUMENT && !copy);
        assert(fvm_text_layout_line(layout, 1, &line) == FVM_INVALID_ARGUMENT);
        assert(fvm_text_layout_glyph(layout, 1, &g) == FVM_INVALID_ARGUMENT);
        close_layout(&layout);
        assert(fvm_path_bounds(path, &bounds) == FVM_OK && bounds.x_max == 20);
        assert(fvm_path_get(path, 1, &command) == FVM_OK && command.verb == FVM_PATH_QUADRATIC);
        assert(fvm_path_set_point(path, 1, 0, 50, 0) == FVM_OK);
        assert(fvm_path_bounds(path, &bounds) == FVM_OK && bounds.x_max > 20);
        memset(pixels, 0, sizeof(pixels));
#if defined(FVM_HAS_AGG) || defined(FVM_HAS_CAIRO)
        assert(fvm_path_draw(path, &surface, color, 0, 0) == FVM_OK);
#else
        assert(fvm_path_draw(path, &surface, color, 0, 0) == FVM_UNSUPPORTED);
#endif
        fvm_path_destroy(path);
    }
#endif
    return 0;
}
