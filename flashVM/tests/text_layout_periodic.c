/* Periodic stops are not restricted to the 64-entry custom-table representation. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"


int main(void)
{
    FVM_TextLayout *l = 0;
    FVM_Font *font = 0;
    FVM_TextLayoutOptions o = {0, 0, FVM_TEXT_LTR, FVM_TEXT_ALIGN_LEFT, 0};
#ifdef FVM_HAS_FONTS
    FVM_TextLayoutMetrics m;
    FVM_TextLayoutGlyph g;
    char text[100];
    unsigned i;
    if (!(fvm_text_layout_create_periodic(font, 0, 0, &o, 0, 0, 0, 0, &l) == FVM_INVALID_ARGUMENT && !l))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_periodic(font, "A\tV", 3, &o, 0, 0, 0, 40, &l) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_metrics(l, &m) == FVM_OK && m.advance_width == 64))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_glyph(l, 1, &g) == FVM_OK && g.shaped.glyph == 0 && g.shaped.x_advance == 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(l);
    if (!(fvm_text_layout_create_periodic(font, "A\tV", 3, &o, 0, 0, 16, 40, &l) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_glyph(l, 2, &g) == FVM_OK && g.x == 80))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(l);
    for (i = 0; i < 100; ++i)
    {
        text[i] = '\t';
    }
    if (!(fvm_text_layout_create_periodic(font, text, 100, &o, 0, 0, 0, 10, &l) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_metrics(l, &m) == FVM_OK && m.glyph_count == 100 && m.advance_width == 1000))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(l);
    fvm_font_destroy(font);
#else
    if (!(fvm_text_layout_create_periodic(font, 0, 0, &o, 0, 0, 0, 40, &l) == FVM_UNSUPPORTED && !l))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
