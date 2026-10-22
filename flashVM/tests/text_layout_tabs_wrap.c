/* Tab exhaustion participates in wrapping; table coordinates precede whole-line alignment. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
int main(void)
{
#ifdef FVM_HAS_FONTS
 FVM_Font *font=0;FVM_TextLayout *l=0;FVM_TextLayoutMetrics m;FVM_TextLayoutLine a,b;FVM_TextLayoutOptions o={50,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_LEFT,0};double stops[]={40,80};unsigned i;
 assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);assert(fvm_text_layout_create_tabbed(font,"A\tV",3,&o,0,0,0,stops,2,&l)==FVM_OK && fvm_text_layout_metrics(l,&m)==FVM_OK && m.line_count==2 && m.advance_width==40);assert(fvm_text_layout_line(l,0,&a)==FVM_OK && a.byte_end==2 && a.advance==40 && fvm_text_layout_line(l,1,&b)==FVM_OK && b.byte_start==2 && b.advance==24);fvm_text_layout_destroy(l);
 stops[0]=20;o.max_width=30;assert(fvm_text_layout_create_tabbed(font,"A\tV",3,&o,0,0,0,stops,1,&l)==FVM_OK && fvm_text_layout_metrics(l,&m)==FVM_OK && m.line_count==3);fvm_text_layout_destroy(l);o.max_width=0;assert(fvm_text_layout_create_tabbed(font,"A\tV",3,&o,0,0,0,stops,1,&l)==FVM_UNSUPPORTED && !l);
 stops[0]=40;o.max_width=100;for(i=0;i<4;++i){o.alignment=(FVM_TextAlignment)i;assert(fvm_text_layout_create_tabbed(font,"A\tV",3,&o,0,0,0,stops,2,&l)==FVM_OK && fvm_text_layout_metrics(l,&m)==FVM_OK && m.advance_width==64 && fvm_text_layout_line(l,0,&a)==FVM_OK && a.x==(i==1?18:i==2?36:0));fvm_text_layout_destroy(l);}
 o.alignment=FVM_TEXT_ALIGN_JUSTIFY;assert(fvm_text_layout_create_tabbed(font,"A\tV",3,&o,0,0,0,stops,2,&l)==FVM_UNSUPPORTED && !l);assert(fvm_text_layout_create_tabbed(font,"AV",2,&o,0,0,0,stops,2,&l)==FVM_OK);fvm_text_layout_destroy(l);fvm_font_destroy(font);
#endif
 return 0;
}
