/* Invalid period/geometry, unsupported tab paragraphs and bounded arithmetic fail explicitly. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <limits>


int main()
{
    FVM_Font *font = 0;
    FVM_TextLayout *l = 0;
    FVM_TextLayoutOptions o = {0, 0, FVM_TEXT_LTR, FVM_TEXT_ALIGN_LEFT, 0};
    const double bad[] = {
        0, -1, 1000001, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()};
    for (unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i)
    {
        if (!(fvm_text_layout_create_periodic(font, 0, 0, &o, 0, 0, 0, bad[i], &l) == FVM_INVALID_ARGUMENT && !l))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_text_layout_create_periodic(font, 0, 0, &o, 0, 0, 0, 40, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef FVM_HAS_FONTS
    if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    o.alignment = FVM_TEXT_ALIGN_JUSTIFY;
    if (!(fvm_text_layout_create_periodic(font, "A\tV", 3, &o, 0, 0, 0, 40, &l) == FVM_UNSUPPORTED && !l))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_periodic(font, "AV", 2, &o, 0, 0, 0, 40, &l) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(l);
    o.alignment = FVM_TEXT_ALIGN_LEFT;
    o.direction = FVM_TEXT_RTL;
    if (!(fvm_text_layout_create_periodic(font, "A\tV", 3, &o, 0, 0, 0, 40, &l) == FVM_UNSUPPORTED && !l))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    o.direction = FVM_TEXT_LTR;
    if (!(fvm_text_layout_create_periodic(font, "\t\t", 2, &o, 0, 0, 0, 1000000, &l) == FVM_UNSUPPORTED && !l))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_periodic(
              font, "A\tV", 3, &o, 0, 0, 0, std::numeric_limits<double>::denorm_min(), &l) == FVM_UNSUPPORTED &&
          !l))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_periodic(font, "A\tV", 3, &o, 0, 0, 0, 0.125, &l) == FVM_OK))
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
