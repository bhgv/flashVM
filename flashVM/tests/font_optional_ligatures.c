/* Analytical fi decomposition preserves ordinary kerning and per-font feature isolation. */
#include "flashvm/font.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
static double advance(FVM_TextRun *run)
{ size_t i;double sum=0;FVM_ShapedGlyph g;for(i=0;i<fvm_text_run_count(run);++i){assert(fvm_text_run_get(run,i,&g)==FVM_OK);sum+=g.x_advance;}return sum; }
int main(void)
{
 FVM_TextRun *run=(FVM_TextRun*)1;unsigned int flags;
 assert(fvm_font_shape_configured(0,"fi",2,FVM_TEXT_LTR,0,4,&run)==FVM_INVALID_ARGUMENT && !run);
#ifdef FVM_HAS_FONTS
 { FVM_Font *font=0;FVM_ShapedGlyph first,second;
  assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
  for(flags=0;flags<=FVM_FONT_SHAPE_FLAGS;++flags){
   assert(fvm_font_shape_configured(font,"fi",2,FVM_TEXT_LTR,0,flags,&run)==FVM_OK);
   assert(fvm_text_run_count(run)==((flags&FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES)?2:1));
   assert(fabs(advance(run)-((flags&FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES)?24:28))<0.02);
   if(flags&FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES){assert(fvm_text_run_get(run,0,&first)==FVM_OK && fvm_text_run_get(run,1,&second)==FVM_OK && first.cluster==0 && second.cluster==1);}
   fvm_text_run_destroy(run);
   assert(fvm_font_shape_configured(font,"AV",2,FVM_TEXT_LTR,0,flags,&run)==FVM_OK && fabs(advance(run)-((flags&FVM_FONT_SHAPE_NO_KERNING)?48:44.8))<0.02);fvm_text_run_destroy(run);
  }
  assert(fvm_font_shape(font,"fi",2,FVM_TEXT_LTR,0,&run)==FVM_OK && fvm_text_run_count(run)==1);fvm_text_run_destroy(run);fvm_font_destroy(font);
 }
#else
 for(flags=0;flags<=FVM_FONT_SHAPE_FLAGS;++flags)assert(fvm_font_shape_configured(0,"fi",2,FVM_TEXT_LTR,0,flags,&run)==FVM_UNSUPPORTED && !run);
#endif
 return 0;
}
