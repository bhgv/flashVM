/* Failed tracked layout construction releases partial paths and leaves the font reusable. */
#define main allocation_reference_main
#include "textfield_counted_text_allocation.cpp"
#undef main
#include <cmath>
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Font *font=0;assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
 bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<2048 && !success;++boundary){
  FVM_TextLayout *layout=(FVM_TextLayout*)1;
  allowance=boundary;FVM_Result result=fvm_text_layout_create_spaced(font,"AV",2,0,FVM_FONT_SHAPE_NO_KERNING,2,&layout);allowance=-1;
  assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);success=result==FVM_OK;
  if(!success){++failures;assert(!layout);assert(fvm_text_layout_create_spaced(font,"AV",2,0,FVM_FONT_SHAPE_NO_KERNING,2,&layout)==FVM_OK);}
  FVM_TextLayoutMetrics metrics;assert(fvm_text_layout_metrics(layout,&metrics)==FVM_OK && std::fabs(metrics.advance_width-52)<0.02);fvm_text_layout_destroy(layout);
 }
 assert(success && failures);fvm_font_destroy(font);
#endif
 return 0;
}
