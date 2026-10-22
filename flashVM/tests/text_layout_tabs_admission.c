/* Explicit C table bounds and invalid geometry are validated in both font profiles. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
int main(void)
{
 FVM_Font *font=0;FVM_TextLayout *l=0;FVM_TextLayoutOptions o={0,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_LEFT,0};double stops[65];unsigned i;for(i=0;i<65;++i)stops[i]=40*(i+1);
#ifdef FVM_HAS_FONTS
 assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
#endif
 assert(fvm_text_layout_create_tabbed(font,"A\tV",3,&o,0,0,0,stops,65,&l)==FVM_INVALID_ARGUMENT && !l);assert(fvm_text_layout_create_tabbed(font,"AV",2,&o,0,0,0,0,1,&l)==FVM_INVALID_ARGUMENT && !l);
 for(i=0;i<5;++i){stops[1]=i==0?40:i==1?20:i==2?-1:i==3?HUGE_VAL:strtod("nan",0);assert(fvm_text_layout_create_tabbed(font,"AV",2,&o,0,0,0,stops,2,&l)==FVM_INVALID_ARGUMENT && !l);}
#ifdef FVM_HAS_FONTS
 stops[1]=80;assert(fvm_text_layout_create_tabbed(font,"\xd7\x90\tA",4,&o,0,0,0,stops,2,&l)==FVM_UNSUPPORTED && !l);assert(fvm_text_layout_create_indented(font,"A\tV",3,&o,0,0,0,&l)==FVM_UNSUPPORTED && !l);fvm_font_destroy(font);
#endif
 return 0;
}
