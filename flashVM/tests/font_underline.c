/* Analytic CC0 post-table oracle: 1000 units/em, top -75, thickness 50. */
#include "flashvm/font.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
int main(void)
{
 FVM_FontUnderlineMetrics m;
 assert(fvm_font_underline_metrics(0,0)==FVM_INVALID_ARGUMENT);
#ifdef FVM_HAS_FONTS
 {
  FVM_Font *font=0;unsigned i;
  assert(fvm_font_underline_metrics(0,&m)==FVM_INVALID_ARGUMENT && m.center_y==0 && m.thickness==0);
  for(i=1;i<=3;++i){
   assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,20*i,&font)==FVM_OK);
   assert(fvm_font_underline_metrics(font,&m)==FVM_OK);
   assert(fabs(m.center_y-2*i)<0.02 && fabs(m.thickness-i)<0.02);
   fvm_font_destroy(font);assert(fabs(m.thickness-i)<0.02);
  }
 }
#else
 m.center_y=9;m.thickness=8;
 assert(fvm_font_underline_metrics(0,&m)==FVM_UNSUPPORTED && m.center_y==0 && m.thickness==0);
#endif
 return 0;
}
