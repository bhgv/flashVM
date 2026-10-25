/* Indentation preserves ligature selection, tracked decomposition and source clusters. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/text_layout.h"
#include "fixtures/font_ligature_data.h"
#include <math.h>


int main(void)
{
#ifdef FVM_HAS_FONTS
    FVM_Font *font = 0;
    FVM_TextLayout *a = 0, *b = 0;
    FVM_TextLayoutOptions o = {0, 0, FVM_TEXT_LTR, FVM_TEXT_ALIGN_LEFT, 0};
    size_t pass, i;
    if (!(fvm_font_read(fvm_test_font_ligatures, sizeof(fvm_test_font_ligatures), 0, 40, &font) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (pass = 0; pass < 2; ++pass)
    {
        FVM_TextLayoutMetrics m, n;
        double spacing = pass ? 0.5 : 0;
        if (!(fvm_text_layout_create_spaced(font, "fi", 2, &o, 0, spacing, &a) == FVM_OK &&
              fvm_text_layout_create_indented(font, "fi", 2, &o, 0, spacing, 12.5, &b) == FVM_OK &&
              fvm_text_layout_metrics(a, &m) == FVM_OK && fvm_text_layout_metrics(b, &n) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(m.glyph_count == (pass ? 2 : 1) && n.glyph_count == m.glyph_count &&
              fabs(n.advance_width - m.advance_width - 12.5) < 1e-9))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < m.glyph_count; ++i)
        {
            FVM_TextLayoutGlyph x, y;
            if (!(fvm_text_layout_glyph(a, i, &x) == FVM_OK && fvm_text_layout_glyph(b, i, &y) == FVM_OK &&
                  x.shaped.cluster == y.shaped.cluster && x.shaped.glyph == y.shaped.glyph &&
                  fabs(y.x - x.x - 12.5) < 1e-9))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_text_layout_destroy(a);
        fvm_text_layout_destroy(b);
    }
    fvm_font_destroy(font);
#endif
    return 0;
}
