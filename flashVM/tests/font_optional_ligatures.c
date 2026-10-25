/* Analytical fi decomposition preserves ordinary kerning and per-font feature isolation. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/font.h"
#include "fixtures/font_data.h"
#include <math.h>


static double advance(FVM_TextRun *run)
{
    size_t i;
    double sum = 0;
    FVM_ShapedGlyph g;
    for (i = 0; i < fvm_text_run_count(run); ++i)
    {
        if (!(fvm_text_run_get(run, i, &g) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sum += g.x_advance;
    }
    return sum;
}


int main(void)
{
    FVM_TextRun *run = (FVM_TextRun *)1;
    unsigned int flags;
    if (!(fvm_font_shape_configured(0, "fi", 2, FVM_TEXT_LTR, 0, 4, &run) == FVM_INVALID_ARGUMENT && !run))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef FVM_HAS_FONTS
    {
        FVM_Font *font = 0;
        FVM_ShapedGlyph first, second;
        if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (flags = 0; flags <= FVM_FONT_SHAPE_FLAGS; ++flags)
        {
            if (!(fvm_font_shape_configured(font, "fi", 2, FVM_TEXT_LTR, 0, flags, &run) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_text_run_count(run) == ((flags & FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES) ? 2 : 1)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fabs(advance(run) - ((flags & FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES) ? 24 : 28)) < 0.02))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (flags & FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES)
            {
                if (!(fvm_text_run_get(run, 0, &first) == FVM_OK && fvm_text_run_get(run, 1, &second) == FVM_OK &&
                      first.cluster == 0 && second.cluster == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_text_run_destroy(run);
            if (!(fvm_font_shape_configured(font, "AV", 2, FVM_TEXT_LTR, 0, flags, &run) == FVM_OK &&
                  fabs(advance(run) - ((flags & FVM_FONT_SHAPE_NO_KERNING) ? 48 : 44.8)) < 0.02))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_text_run_destroy(run);
        }
        if (!(fvm_font_shape(font, "fi", 2, FVM_TEXT_LTR, 0, &run) == FVM_OK && fvm_text_run_count(run) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_text_run_destroy(run);
        fvm_font_destroy(font);
    }
#else
    for (flags = 0; flags <= FVM_FONT_SHAPE_FLAGS; ++flags)
    {
        if (!(fvm_font_shape_configured(0, "fi", 2, FVM_TEXT_LTR, 0, flags, &run) == FVM_UNSUPPORTED && !run))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
#endif
    return 0;
}
