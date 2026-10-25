#include <stdio.h>
#include <stdlib.h>
#include "flashvm/font.h"


int main(void)
{
    FVM_Font *font = (FVM_Font *)1;
    FVM_TextRun *run = (FVM_TextRun *)1;
    if (!(fvm_font_read(0, 0, 0, 40, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef FVM_HAS_FONTS
    if (!(fvm_font_read(0, 0, 0, 40, &font) == FVM_INVALID_ARGUMENT && !font))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_font_shape(0, "x", 1, FVM_TEXT_AUTO, 0, &run) == FVM_INVALID_ARGUMENT && !run))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    if (!(fvm_font_read(0, 0, 0, 40, &font) == FVM_UNSUPPORTED && !font))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_font_shape(0, "x", 1, FVM_TEXT_AUTO, 0, &run) == FVM_UNSUPPORTED && !run))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    fvm_font_destroy(0);
    fvm_text_run_destroy(0);
    return 0;
}
