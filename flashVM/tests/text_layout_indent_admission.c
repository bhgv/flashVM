/* C ABI admission rejects nonfinite/bad offsets and nonzero-indent bidi without undefined enum reads. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
int main(void)
{
 FVM_Font *font=0;FVM_TextLayout *layout=0;FVM_TextLayoutOptions o={90,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_LEFT,0};double bad[4];size_t i;
 bad[0]=-4097;bad[1]=4097;bad[2]=HUGE_VAL;bad[3]=strtod("nan",0);
#ifdef FVM_HAS_FONTS
 assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
#endif
 for(i=0;i<4;++i){layout=(FVM_TextLayout*)font;assert(fvm_text_layout_create_indented(font,"AV",2,&o,0,0,bad[i],&layout)==FVM_INVALID_ARGUMENT && !layout);}
 assert(fvm_text_layout_create_indented(font,"AV",2,&o,0,0,1,0)==FVM_INVALID_ARGUMENT);
#ifdef FVM_HAS_FONTS
 assert(fvm_text_layout_create_indented(font,"\xd7\x90",2,&o,0,0,1,&layout)==FVM_UNSUPPORTED && !layout);assert(fvm_text_layout_create_indented(font,"\xd7\x90",2,&o,0,0,0,&layout)==FVM_OK);fvm_text_layout_destroy(layout);
 o.direction=FVM_TEXT_RTL;assert(fvm_text_layout_create_indented(font,"AV",2,&o,0,0,1,&layout)==FVM_UNSUPPORTED && !layout);o.direction=FVM_TEXT_LTR;
 memset(&o.alignment,127,sizeof(o.alignment));assert(fvm_text_layout_create_indented(font,"AV",2,&o,0,0,1,&layout)==FVM_INVALID_ARGUMENT && !layout);o.alignment=FVM_TEXT_ALIGN_LEFT;o.max_width=1000000;assert(fvm_text_layout_create_indented(font,"AV",2,&o,0,0,-1,&layout)==FVM_UNSUPPORTED && !layout);fvm_font_destroy(font);
#else
 assert(fvm_text_layout_create_indented(font,"AV",2,&o,0,0,1,&layout)==FVM_UNSUPPORTED && !layout);
#endif
 return 0;
}
