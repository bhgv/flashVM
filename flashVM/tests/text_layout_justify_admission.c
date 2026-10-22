/* C enum bytes are admitted before C++ loads; unsupported directions clear owned outputs. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <string.h>
int main(void)
{
 FVM_TextLayout *layout=(FVM_TextLayout*)1;FVM_TextLayoutOptions o={90,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_JUSTIFY,0};
#ifdef FVM_HAS_FONTS
 FVM_Font *font=0;assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
 o.alignment=(FVM_TextAlignment)5;assert(fvm_text_layout_create(font,"A V",3,&o,&layout)==FVM_INVALID_ARGUMENT && !layout);
 memset(&o.alignment,127,sizeof(o.alignment));assert(fvm_text_layout_create(font,"A V",3,&o,&layout)==FVM_INVALID_ARGUMENT && !layout);o.alignment=FVM_TEXT_ALIGN_JUSTIFY;
 memset(&o.direction,127,sizeof(o.direction));assert(fvm_text_layout_create(font,"A V",3,&o,&layout)==FVM_INVALID_ARGUMENT && !layout);
 o.direction=FVM_TEXT_RTL;assert(fvm_text_layout_create(font,"A V",3,&o,&layout)==FVM_UNSUPPORTED && !layout);o.direction=FVM_TEXT_LTR;
 assert(fvm_text_layout_create(font,"A V",3,&o,&layout)==FVM_OK);fvm_text_layout_destroy(layout);fvm_font_destroy(font);
#else
 assert(fvm_text_layout_create(0,"A V",3,&o,&layout)==FVM_UNSUPPORTED && !layout);
 memset(&o.alignment,127,sizeof(o.alignment));assert(fvm_text_layout_create(0,"A V",3,&o,&layout)==FVM_UNSUPPORTED && !layout);
#endif
 return 0;
}
