/* Periodic stops are not restricted to the 64-entry custom-table representation. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
int main(void)
{
 FVM_TextLayout *l=0;FVM_Font *font=0;FVM_TextLayoutOptions o={0,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_LEFT,0};
#ifdef FVM_HAS_FONTS
 FVM_TextLayoutMetrics m;FVM_TextLayoutGlyph g;char text[100];unsigned i;
 assert(fvm_text_layout_create_periodic(font,0,0,&o,0,0,0,0,&l)==FVM_INVALID_ARGUMENT && !l);
 assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
 assert(fvm_text_layout_create_periodic(font,"A\tV",3,&o,0,0,0,40,&l)==FVM_OK);
 assert(fvm_text_layout_metrics(l,&m)==FVM_OK && m.advance_width==64);
 assert(fvm_text_layout_glyph(l,1,&g)==FVM_OK && g.shaped.glyph==0 && g.shaped.x_advance==16);
 fvm_text_layout_destroy(l);
 assert(fvm_text_layout_create_periodic(font,"A\tV",3,&o,0,0,16,40,&l)==FVM_OK);
 assert(fvm_text_layout_glyph(l,2,&g)==FVM_OK && g.x==80);fvm_text_layout_destroy(l);
 for(i=0;i<100;++i)text[i]='\t';
 assert(fvm_text_layout_create_periodic(font,text,100,&o,0,0,0,10,&l)==FVM_OK);
 assert(fvm_text_layout_metrics(l,&m)==FVM_OK && m.glyph_count==100 && m.advance_width==1000);
 fvm_text_layout_destroy(l);fvm_font_destroy(font);
#else
 assert(fvm_text_layout_create_periodic(font,0,0,&o,0,0,0,40,&l)==FVM_UNSUPPORTED && !l);
#endif
 return 0;
}
