#include "flashvm/font.h"
#include <assert.h>
int main(void)
{
    FVM_Font *font = (FVM_Font *)1;
    FVM_TextRun *run = (FVM_TextRun *)1;
    assert(fvm_font_read(0, 0, 0, 40, 0) == FVM_INVALID_ARGUMENT);
#ifdef FVM_HAS_FONTS
    assert(fvm_font_read(0, 0, 0, 40, &font) == FVM_INVALID_ARGUMENT && !font);
    assert(fvm_font_shape(0, "x", 1, FVM_TEXT_AUTO, 0, &run) == FVM_INVALID_ARGUMENT && !run);
#else
    assert(fvm_font_read(0, 0, 0, 40, &font) == FVM_UNSUPPORTED && !font);
    assert(fvm_font_shape(0, "x", 1, FVM_TEXT_AUTO, 0, &run) == FVM_UNSUPPORTED && !run);
#endif
    fvm_font_destroy(0);
    fvm_text_run_destroy(0);
    return 0;
}
