/* Explicit tab stops place empty source glyphs between independently shaped words. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"


int main(void)
{
    FVM_Font *font = 0;
    FVM_TextLayout *layout = 0;
    double stops[] = {40, 80};
    FVM_TextLayoutOptions o = {0, 0, FVM_TEXT_LTR, FVM_TEXT_ALIGN_LEFT, 0};
#ifdef FVM_HAS_FONTS
    FVM_TextLayoutMetrics m;
    FVM_TextLayoutGlyph g;
    FVM_Path *p = 0;
    if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_tabbed(font, "A\tV\tA", 5, &o, FVM_FONT_SHAPE_NO_KERNING, 0, 0, stops, 2, &layout) ==
              FVM_OK &&
          fvm_text_layout_metrics(layout, &m) == FVM_OK && m.line_count == 1 && m.glyph_count == 5 &&
          m.advance_width == 104))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_glyph(layout, 1, &g) == FVM_OK && g.shaped.cluster == 1 && g.shaped.glyph == 0 && g.x == 24 &&
          g.shaped.x_advance == 16 && fvm_text_layout_glyph_path(layout, 1, &p) == FVM_OK && fvm_path_count(p) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(p);
    if (!(fvm_text_layout_glyph(layout, 4, &g) == FVM_OK && g.x == 80 && fvm_text_layout_path(layout, &p) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(layout);
    fvm_font_destroy(font);
    if (!(fvm_path_count(p) > 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(p);
#else
    if (!(fvm_text_layout_create_tabbed(font, "A\tV", 3, &o, 0, 0, 0, stops, 2, &layout) == FVM_UNSUPPORTED && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
