/* CRLF, empty paragraphs, fractional/hanging offsets, line alignment and legacy-zero identity. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <math.h>


int main(void)
{
#ifdef FVM_HAS_FONTS
    FVM_Font *font = 0;
    FVM_TextLayout *a = 0, *b = 0;
    FVM_TextLayoutOptions o = {102, 0, FVM_TEXT_LTR, FVM_TEXT_ALIGN_LEFT, 0};
    FVM_TextLayoutLine l;
    FVM_TextLayoutMetrics m, n;
    size_t i;
    if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_indented(font, "AV\r\n\r\nAV\n", 9, &o, FVM_FONT_SHAPE_NO_KERNING, 0, 18.5, &a) ==
              FVM_OK &&
          fvm_text_layout_metrics(a, &m) == FVM_OK && m.line_count == 4 && m.advance_width == 66.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_line(a, 0, &l) == FVM_OK && l.x == 18.5 && l.byte_next == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_line(a, 1, &l) == FVM_OK && l.x == 0 && l.byte_start == 4 && l.byte_next == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_line(a, 2, &l) == FVM_OK && l.x == 18.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_line(a, 3, &l) == FVM_OK && l.x == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(a);
    o.max_width = 90;
    if (!(fvm_text_layout_create_indented(font, "A V A V", 7, &o, FVM_FONT_SHAPE_NO_KERNING, 0, -18, &a) == FVM_OK &&
          fvm_text_layout_metrics(a, &m) == FVM_OK && m.line_count == 2 && m.advance_width == 90))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_line(a, 0, &l) == FVM_OK && l.x == -18 && l.advance == 108))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_line(a, 1, &l) == FVM_OK && l.x == 0 && l.advance == 24))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(a);
    o.max_width = 102;
    for (i = 0; i < 4; ++i)
    {
        o.alignment = (FVM_TextAlignment)i;
        if (!(fvm_text_layout_create_spaced(font, "A V A V\nAV", 10, &o, 0, 0, &a) == FVM_OK &&
              fvm_text_layout_create_indented(font, "A V A V\nAV", 10, &o, 0, 0, 0, &b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_text_layout_metrics(a, &m) == FVM_OK && fvm_text_layout_metrics(b, &n) == FVM_OK &&
              m.advance_width == n.advance_width && m.line_count == n.line_count))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        {
            size_t j;
            for (j = 0; j < m.glyph_count; ++j)
            {
                FVM_TextLayoutGlyph x, y;
                if (!(fvm_text_layout_glyph(a, j, &x) == FVM_OK && fvm_text_layout_glyph(b, j, &y) == FVM_OK &&
                      x.x == y.x && x.y == y.y && x.shaped.cluster == y.shaped.cluster))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        fvm_text_layout_destroy(a);
        fvm_text_layout_destroy(b);
    }
    for (i = 0; i < 4; ++i)
    {
        FVM_TextLayoutLine first, next;
        o.alignment = (FVM_TextAlignment)i;
        if (!(fvm_text_layout_create_indented(font, "A V A V", 7, &o, FVM_FONT_SHAPE_NO_KERNING, 0, 12, &a) == FVM_OK &&
              fvm_text_layout_metrics(a, &m) == FVM_OK && m.advance_width == 84))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_text_layout_line(a, 0, &first) == FVM_OK && fvm_text_layout_line(a, 1, &next) == FVM_OK &&
              first.x == (i == 1   ? 21
                          : i == 2 ? 30
                                   : 12) &&
              next.x == (i == 1   ? 21
                         : i == 2 ? 42
                                  : 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_text_layout_destroy(a);
    }
    o.alignment = FVM_TEXT_ALIGN_JUSTIFY;
    if (!(fvm_text_layout_create_indented(font, "A V A V", 7, &o, FVM_FONT_SHAPE_NO_KERNING, 0, 12, &a) == FVM_OK &&
          fvm_text_layout_line(a, 0, &l) == FVM_OK && l.x == 12 && l.advance == 90))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(a);
    o.max_width = 1;
    if (!(fvm_text_layout_create_indented(font, "AV", 2, &o, 0, 0, 4096, &a) == FVM_OK &&
          fvm_text_layout_metrics(a, &m) == FVM_OK && m.line_count == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(a);
    fvm_font_destroy(font);
#endif
    return 0;
}
