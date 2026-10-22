/* Synthetic independent GSUB oracle separates optional liga/clig from required rlig. */
#include "flashvm/text_layout.h"
#include "fixtures/font_ligature_data.h"
#include <assert.h>
#include <math.h>
int main(void)
{
#ifdef FVM_HAS_FONTS
 FVM_Font *font=0;FVM_TextRun *run=0;FVM_TextLayout *layout=0;FVM_TextLayoutMetrics metrics;unsigned int flags;
 assert(fvm_font_read(fvm_test_font_ligatures,sizeof(fvm_test_font_ligatures),0,40,&font)==FVM_OK);
 for(flags=0;flags<=FVM_FONT_SHAPE_FLAGS;++flags){
  assert(fvm_font_shape_configured(font,"ff",2,FVM_TEXT_LTR,0,flags,&run)==FVM_OK && fvm_text_run_count(run)==((flags&FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES)?2:1));fvm_text_run_destroy(run);
  assert(fvm_font_shape_configured(font,"VA",2,FVM_TEXT_LTR,0,flags,&run)==FVM_OK && fvm_text_run_count(run)==1);fvm_text_run_destroy(run);
 }
 assert(fvm_text_layout_create_spaced(font,"ff",2,0,0,1,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&metrics)==FVM_OK && metrics.glyph_count==2 && fabs(metrics.advance_width-30)<0.02);fvm_text_layout_destroy(layout);
 assert(fvm_text_layout_create_spaced(font,"VA",2,0,0,1,&layout)==FVM_UNSUPPORTED && !layout);
 assert(fvm_text_layout_create_spaced(font,"VA",2,0,FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES,0,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&metrics)==FVM_OK && metrics.glyph_count==1);fvm_text_layout_destroy(layout);
 assert(fvm_font_shape(font,"ff",2,FVM_TEXT_LTR,0,&run)==FVM_OK && fvm_text_run_count(run)==1);fvm_text_run_destroy(run);fvm_font_destroy(font);
#endif
 return 0;
}
