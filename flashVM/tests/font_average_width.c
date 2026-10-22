/* Independent fixture OS/2 xAvgCharWidth=547, unitsPerEm=1000; scaled 26.6 rounding. */
#include "flashvm/font.h"
#include "fixtures/font_data.h"
#include <assert.h>
int main(void)
{
 double width=123;FVM_Font *font=0;
 assert(fvm_font_average_width(0,0)==FVM_INVALID_ARGUMENT);
#ifdef FVM_HAS_FONTS
 assert(fvm_font_average_width(0,&width)==FVM_INVALID_ARGUMENT && width==0);
 assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
 assert(fvm_font_average_width(font,&width)==FVM_OK && width==21.875);
 fvm_font_destroy(font);
 assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,20,&font)==FVM_OK);
 assert(fvm_font_average_width(font,&width)==FVM_OK && width==10.9375);
 fvm_font_destroy(font);
#else
 assert(fvm_font_average_width(font,&width)==FVM_UNSUPPORTED && width==0);
#endif
 return 0;
}
