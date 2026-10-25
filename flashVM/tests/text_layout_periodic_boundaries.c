/* Repeating tabs reset at visual/hard lines, include tracking/indent, and retain owned paths. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <math.h>


int main(void)
{
    FVM_Font *font = 0;
    FVM_TextLayout *l = 0;
    FVM_TextLayoutOptions o = {50, 0, FVM_TEXT_LTR, FVM_TEXT_ALIGN_LEFT, 0};
#ifdef FVM_HAS_FONTS
    FVM_TextLayoutMetrics m;
    FVM_TextLayoutLine line;
    FVM_TextLayoutGlyph g;
    FVM_Path *p = 0, *tab = 0;
    unsigned i;
    if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_periodic(font, "A\tV", 3, &o, 0, 0, 0, 40, &l) == FVM_OK &&
          fvm_text_layout_metrics(l, &m) == FVM_OK && m.line_count == 2 && m.advance_width == 40))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_line(l, 0, &line) == FVM_OK && line.byte_end == 2 && line.advance == 40))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(l);
    o.max_width = 1000000;
    if (!(fvm_text_layout_create_periodic(font, "\t\t", 2, &o, 0, 0, 0, 1000000, &l) == FVM_OK &&
          fvm_text_layout_metrics(l, &m) == FVM_OK && m.line_count == 2 && m.advance_width == 1000000))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(l);
    o.max_width = 50;
    if (!(fvm_text_layout_create_periodic(font, "A\tV", 3, &o, 0, 0, 0, 87.5, &l) == FVM_OK &&
          fvm_text_layout_metrics(l, &m) == FVM_OK && m.line_count == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(l);
    o.max_width = 0;
    if (!(fvm_text_layout_create_periodic(font, "A\tV\r\nA\tV", 8, &o, 0, 0.5, -10, 40, &l) == FVM_OK &&
          fvm_text_layout_metrics(l, &m) == FVM_OK && m.line_count == 2 && m.advance_width == 64.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_glyph(l, 2, &g) == FVM_OK && g.x == 40 && fvm_text_layout_glyph(l, 5, &g) == FVM_OK &&
          g.x == 40))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(l);
    o.max_width = 100;
    for (i = 0; i < 4; ++i)
    {
        o.alignment = (FVM_TextAlignment)i;
        if (!(fvm_text_layout_create_periodic(font, "A\tV", 3, &o, 0, 0, 0, 40, &l) == FVM_OK &&
              fvm_text_layout_line(l, 0, &line) == FVM_OK &&
              line.x == (i == 1   ? 18
                         : i == 2 ? 36
                                  : 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_text_layout_destroy(l);
    }
    o.alignment = FVM_TEXT_ALIGN_LEFT;
    o.max_width = 0;
    if (!(fvm_text_layout_create_periodic(font, "A\tV", 3, &o, 0, 0, 0, 40, &l) == FVM_OK &&
          fvm_text_layout_path(l, &p) == FVM_OK && fvm_text_layout_glyph_path(l, 1, &tab) == FVM_OK &&
          fvm_path_count(tab) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_font_destroy(font);
    fvm_text_layout_destroy(l);
    if (!(fvm_path_count(p) > 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(tab);
    fvm_path_destroy(p);
#else
    if (!(fvm_text_layout_create_periodic(font, 0, 0, &o, 0, 0, 0, 40, &l) == FVM_UNSUPPORTED && !l))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
