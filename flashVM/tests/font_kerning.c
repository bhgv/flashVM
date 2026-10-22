/* Native feature selection preserves ligatures while changing the original font's AV pair geometry. */
#include "flashvm/font.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
#include <string.h>
/* Sum the logical advances independently of path positioning or layout wrapping. */
static double advance(FVM_TextRun *run)
{
    size_t i;double result=0;FVM_ShapedGlyph glyph;
    for(i=0;i<fvm_text_run_count(run);++i){assert(fvm_text_run_get(run,i,&glyph)==FVM_OK);result+=glyph.x_advance;}
    return result;
}
int main(void)
{
    FVM_Font *font=0;FVM_TextRun *run=(FVM_TextRun*)1;
    assert(fvm_font_shape_configured(0,"AV",2,FVM_TEXT_LTR,0,4,&run)==FVM_INVALID_ARGUMENT && !run);
    { FVM_TextDirection bad;memset(&bad,127,sizeof(bad));
      assert(fvm_font_shape_configured(0,"AV",2,bad,0,0,&run)==FVM_INVALID_ARGUMENT && !run);
      assert(fvm_font_shape(0,"AV",2,bad,0,&run)==FVM_INVALID_ARGUMENT && !run); }
#ifdef FVM_HAS_FONTS
    {
        FVM_TextRun *legacy=0,*enabled=0,*disabled=0;double base;
        assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
        assert(fvm_font_shape(font,"AV",2,FVM_TEXT_LTR,0,&legacy)==FVM_OK);
        assert(fvm_font_shape_configured(font,"AV",2,FVM_TEXT_LTR,0,0,&enabled)==FVM_OK);
        assert(fvm_font_shape_configured(font,"AV",2,FVM_TEXT_LTR,0,FVM_FONT_SHAPE_NO_KERNING,&disabled)==FVM_OK);
        base=advance(legacy);assert(advance(enabled)==base && fabs(advance(disabled)-48)<0.001 && fabs(48-base-3.2)<0.02);
        fvm_text_run_destroy(legacy);fvm_text_run_destroy(enabled);fvm_text_run_destroy(disabled);
        assert(fvm_font_shape_configured(font,"fi",2,FVM_TEXT_LTR,0,FVM_FONT_SHAPE_NO_KERNING,&run)==FVM_OK && fvm_text_run_count(run)==1);
        fvm_font_destroy(font);font=0;assert(fabs(advance(run)-28)<0.001);
    }
#else
    assert(fvm_font_shape_configured(0,"AV",2,FVM_TEXT_LTR,0,FVM_FONT_SHAPE_NO_KERNING,&run)==FVM_UNSUPPORTED && !run);
#endif
    fvm_text_run_destroy(run);fvm_font_destroy(font);return 0;
}
