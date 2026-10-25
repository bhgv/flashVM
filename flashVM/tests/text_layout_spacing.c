/* Analytical geometric-font tracking oracle, independent of system fonts. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <math.h>


int main(void)
{
    FVM_TextLayout *layout = (FVM_TextLayout *)1;
    if (!(fvm_text_layout_create_spaced(0, "AV", 2, 0, 0, 4097, &layout) == FVM_INVALID_ARGUMENT && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_spaced(0, "AV", 2, 0, 0, -4097, &layout) == FVM_INVALID_ARGUMENT && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_spaced(0, "AV", 2, 0, 4, 0, &layout) == FVM_INVALID_ARGUMENT && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_spaced(0, "AV", 2, 0, 0, HUGE_VAL, &layout) == FVM_INVALID_ARGUMENT && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_spaced(0, "AV", 2, 0, 0, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef FVM_HAS_FONTS
    {
        FVM_Font *font = 0;
        FVM_TextLayoutMetrics m;
        FVM_TextLayoutGlyph g;
        FVM_TextLayoutOptions o = {0, 0, FVM_TEXT_LTR, FVM_TEXT_ALIGN_LEFT, 0};
        FVM_Path *path = 0;
        FVM_PathBounds box;
        if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_text_layout_create_spaced(font, "AV", 2, &o, FVM_FONT_SHAPE_NO_KERNING, 2.5, &layout) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_text_layout_metrics(layout, &m) == FVM_OK && fabs(m.advance_width - 53) < 0.02))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_text_layout_glyph(layout, 1, &g) == FVM_OK && fabs(g.x - 26.5) < 0.02 &&
              fabs(g.shaped.x_advance - 26.5) < 0.02))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_text_layout_path(layout, &path) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_text_layout_destroy(layout);
        o.max_width = 50;
        if (!(fvm_text_layout_create_spaced(font, "AV", 2, &o, FVM_FONT_SHAPE_NO_KERNING, 2.5, &layout) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_text_layout_metrics(layout, &m) == FVM_OK && m.line_count == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_text_layout_destroy(layout);
        o.max_width = 0;
        if (!(fvm_text_layout_create_spaced(font, "AV", 2, &o, FVM_FONT_SHAPE_NO_KERNING, -2, &layout) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_text_layout_metrics(layout, &m) == FVM_OK && fabs(m.advance_width - 44) < 0.02))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_text_layout_destroy(layout);
        if (!(fvm_text_layout_create_spaced(font, "AV", 2, &o, FVM_FONT_SHAPE_NO_KERNING, -25, &layout) ==
                  FVM_UNSUPPORTED &&
              !layout))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_text_layout_create_spaced(font, "fi", 2, &o, 0, 1, &layout) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_text_layout_metrics(layout, &m) == FVM_OK && m.glyph_count == 2 && fabs(m.advance_width - 26) < 0.02))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_text_layout_destroy(layout);
        if (!(fvm_text_layout_create_spaced(font, "fi", 2, &o, 0, 0, &layout) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_text_layout_destroy(layout);
        fvm_font_destroy(font);
        if (!(fvm_path_bounds(path, &box) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_path_destroy(path);
    }
#else
    if (!(fvm_text_layout_create_spaced(0, "AV", 2, 0, 0, 1, &layout) == FVM_UNSUPPORTED && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
