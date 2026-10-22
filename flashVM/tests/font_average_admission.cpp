/* Missing/nonpositive declared metrics never fall back to an arbitrary glyph width. */
#include "flashvm/font.h"
#include "fixtures/font_data.h"
#include <cassert>
#include <cstring>
int main()
{
#ifdef FVM_HAS_FONTS
 for(unsigned mode=0;mode<4;++mode){unsigned char bytes[sizeof(fvm_test_font)];std::memcpy(bytes,fvm_test_font,sizeof(bytes));
  /* Independent fixture directory places OS/2 at 328, version=3, average=547. */
  assert(bytes[328]==0 && bytes[329]==3 && bytes[330]==2 && bytes[331]==35);
  if(mode==0)bytes[330]=bytes[331]=0;
  if(mode==1)bytes[330]=bytes[331]=255;
  if(mode==2)bytes[328]=bytes[329]=255;
  if(mode==3)std::memcpy(bytes+44,"TEST",4); /* Remove OS/2 directory tag, keep the rest usable. */
  FVM_Font *font=0;double width=1;assert(fvm_font_read(bytes,sizeof(bytes),0,40,&font)==FVM_OK);
  assert(fvm_font_average_width(font,&width)==FVM_UNSUPPORTED && width==0);
  FVM_TextRun *run=0;assert(fvm_font_shape(font,"AV",2,FVM_TEXT_LTR,0,&run)==FVM_OK && fvm_text_run_count(run)==2);fvm_text_run_destroy(run);fvm_font_destroy(font);
 }
#endif
 return 0;
}
