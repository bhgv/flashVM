/* Byte clusters, bidi placement, separators and alignment share signed tracking. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <math.h>


int main(void)
{
#ifdef FVM_HAS_FONTS
    FVM_Font *font = 0;
    FVM_TextLayout *layout = 0;
    FVM_TextLayoutMetrics m;
    FVM_TextLayoutOptions o = {100, 48, FVM_TEXT_LTR, FVM_TEXT_ALIGN_RIGHT, 0};
    FVM_TextLayoutGlyph a, b;
    FVM_TextLayoutLine line;
    unsigned i;
    if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_spaced(font, "AV", 2, &o, FVM_FONT_SHAPE_NO_KERNING, 2, &layout) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_line(layout, 0, &line) == FVM_OK && fabs(line.x - 48) < 0.02 &&
          fabs(line.advance - 52) < 0.02))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_glyph(layout, 0, &a) == FVM_OK && fvm_text_layout_glyph(layout, 1, &b) == FVM_OK &&
          fabs(b.x - a.x - 26) < 0.02))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(layout);
    o.alignment = FVM_TEXT_ALIGN_CENTER;
    if (!(fvm_text_layout_create_spaced(font, "AV", 2, &o, 0, 2, &layout) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_line(layout, 0, &line) == FVM_OK && fabs(line.advance - 48.8) < 0.02 &&
          fabs(line.x - 25.6) < 0.02))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(layout);
    o.max_width = 0;
    o.alignment = FVM_TEXT_ALIGN_START;
    o.direction = FVM_TEXT_AUTO;
    if (!(fvm_text_layout_create_spaced(font, "\xd7\x90\xd7\x91", 4, &o, 0, 2, &layout) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_glyph(layout, 0, &a) == FVM_OK && fvm_text_layout_glyph(layout, 1, &b) == FVM_OK &&
          a.shaped.cluster == 2 && b.shaped.cluster == 0 && (a.bidi_level & 1) && fabs(b.x - a.x - 26) < 0.02))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(layout);
    if (!(fvm_text_layout_create_spaced(font, "A\0B", 3, &o, 0, 1, &layout) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_metrics(layout, &m) == FVM_OK && m.glyph_count == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 3; ++i)
    {
        if (!(fvm_text_layout_glyph(layout, i, &a) == FVM_OK && a.shaped.cluster == i))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_text_layout_destroy(layout);
    if (!(fvm_text_layout_create_spaced(font, "A\r\nV\n", 5, &o, 0, 2, &layout) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_metrics(layout, &m) == FVM_OK && m.line_count == 3 && m.glyph_count == 2 &&
          fabs(m.advance_width - 26) < 0.02))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_line(layout, 2, &line) == FVM_OK && line.byte_start == 5 && line.byte_end == 5 &&
          line.advance == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(layout);
    if (!(fvm_text_layout_create_spaced(font, "A\xcc\x81", 3, &o, 0, 2, &layout) == FVM_UNSUPPORTED && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_spaced(font, "AV", 2, &o, FVM_FONT_SHAPE_NO_KERNING, -24, &layout) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_metrics(layout, &m) == FVM_OK && m.advance_width == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(layout);
    fvm_font_destroy(font);
#endif
    return 0;
}
