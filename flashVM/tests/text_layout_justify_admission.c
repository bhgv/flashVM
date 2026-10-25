/* C enum bytes are admitted before C++ loads; unsupported directions clear owned outputs. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <string.h>


int main(void)
{
    FVM_TextLayout *layout = (FVM_TextLayout *)1;
    FVM_TextLayoutOptions o = {90, 0, FVM_TEXT_LTR, FVM_TEXT_ALIGN_JUSTIFY, 0};
#ifdef FVM_HAS_FONTS
    FVM_Font *font = 0;
    if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    o.alignment = (FVM_TextAlignment)5;
    if (!(fvm_text_layout_create(font, "A V", 3, &o, &layout) == FVM_INVALID_ARGUMENT && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&o.alignment, 127, sizeof(o.alignment));
    if (!(fvm_text_layout_create(font, "A V", 3, &o, &layout) == FVM_INVALID_ARGUMENT && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    o.alignment = FVM_TEXT_ALIGN_JUSTIFY;
    memset(&o.direction, 127, sizeof(o.direction));
    if (!(fvm_text_layout_create(font, "A V", 3, &o, &layout) == FVM_INVALID_ARGUMENT && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    o.direction = FVM_TEXT_RTL;
    if (!(fvm_text_layout_create(font, "A V", 3, &o, &layout) == FVM_UNSUPPORTED && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    o.direction = FVM_TEXT_LTR;
    if (!(fvm_text_layout_create(font, "A V", 3, &o, &layout) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_text_layout_destroy(layout);
    fvm_font_destroy(font);
#else
    if (!(fvm_text_layout_create(0, "A V", 3, &o, &layout) == FVM_UNSUPPORTED && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&o.alignment, 127, sizeof(o.alignment));
    if (!(fvm_text_layout_create(0, "A V", 3, &o, &layout) == FVM_UNSUPPORTED && !layout))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
