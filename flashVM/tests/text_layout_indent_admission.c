/* C ABI admission rejects nonfinite/bad offsets and nonzero-indent bidi without undefined enum reads. */
#include <stdio.h>
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>


int main(void)
{
    FVM_Font *font = 0;
    FVM_TextLayout *layout = 0;
    FVM_TextLayoutOptions o = {90, 0, FVM_TEXT_LTR, FVM_TEXT_ALIGN_LEFT, 0};
    double bad[4];
    size_t i;
    bad[0] = -4097;
    bad[1] = 4097;
    bad[2] = HUGE_VAL;
    bad[3] = strtod("nan", 0);
#ifdef FVM_HAS_FONTS
    if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    for (i = 0; i < 4; ++i)
    {
        layout = (FVM_TextLayout *)font;
        if (!(fvm_text_layout_create_indented(font, "AV", 2, &o, 0, 0, bad[i], &layout) == FVM_INVALID_ARGUMENT &&
              !layout))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_text_layout_create_indented(font, "AV", 2, &o, 0, 0, 1, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef FVM_HAS_FONTS
    if (!(fvm_text_layout_create_indented(font, "\xd7\x90", 2, &o, 0, 0, 1, &layout) == FVM_UNSUPPORTED && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_layout_create_indented(font, "\xd7\x90", 2, &o, 0, 0, 0, &layout) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(layout);
    o.direction = FVM_TEXT_RTL;
    if (!(fvm_text_layout_create_indented(font, "AV", 2, &o, 0, 0, 1, &layout) == FVM_UNSUPPORTED && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    o.direction = FVM_TEXT_LTR;
    memset(&o.alignment, 127, sizeof(o.alignment));
    if (!(fvm_text_layout_create_indented(font, "AV", 2, &o, 0, 0, 1, &layout) == FVM_INVALID_ARGUMENT && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    o.alignment = FVM_TEXT_ALIGN_LEFT;
    o.max_width = 1000000;
    if (!(fvm_text_layout_create_indented(font, "AV", 2, &o, 0, 0, -1, &layout) == FVM_UNSUPPORTED && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_font_destroy(font);
#else
    if (!(fvm_text_layout_create_indented(font, "AV", 2, &o, 0, 0, 1, &layout) == FVM_UNSUPPORTED && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
