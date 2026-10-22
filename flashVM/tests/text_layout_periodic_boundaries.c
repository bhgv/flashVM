/* Repeating tabs reset at visual/hard lines, include tracking/indent, and retain owned paths. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
int main(void)
{
 FVM_Font *font=0;FVM_TextLayout *l=0;FVM_TextLayoutOptions o={50,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_LEFT,0};
#ifdef FVM_HAS_FONTS
 FVM_TextLayoutMetrics m;FVM_TextLayoutLine line;FVM_TextLayoutGlyph g;FVM_Path *p=0,*tab=0;unsigned i;
 assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
 assert(fvm_text_layout_create_periodic(font,"A\tV",3,&o,0,0,0,40,&l)==FVM_OK && fvm_text_layout_metrics(l,&m)==FVM_OK && m.line_count==2 && m.advance_width==40);
 assert(fvm_text_layout_line(l,0,&line)==FVM_OK && line.byte_end==2 && line.advance==40);fvm_text_layout_destroy(l);
 o.max_width=1000000;assert(fvm_text_layout_create_periodic(font,"\t\t",2,&o,0,0,0,1000000,&l)==FVM_OK && fvm_text_layout_metrics(l,&m)==FVM_OK && m.line_count==2 && m.advance_width==1000000);fvm_text_layout_destroy(l);
 o.max_width=50;assert(fvm_text_layout_create_periodic(font,"A\tV",3,&o,0,0,0,87.5,&l)==FVM_OK && fvm_text_layout_metrics(l,&m)==FVM_OK && m.line_count==3);fvm_text_layout_destroy(l);
 o.max_width=0;assert(fvm_text_layout_create_periodic(font,"A\tV\r\nA\tV",8,&o,0,0.5,-10,40,&l)==FVM_OK && fvm_text_layout_metrics(l,&m)==FVM_OK && m.line_count==2 && m.advance_width==64.5);
 assert(fvm_text_layout_glyph(l,2,&g)==FVM_OK && g.x==40 && fvm_text_layout_glyph(l,5,&g)==FVM_OK && g.x==40);fvm_text_layout_destroy(l);
 o.max_width=100;for(i=0;i<4;++i){o.alignment=(FVM_TextAlignment)i;assert(fvm_text_layout_create_periodic(font,"A\tV",3,&o,0,0,0,40,&l)==FVM_OK && fvm_text_layout_line(l,0,&line)==FVM_OK && line.x==(i==1?18:i==2?36:0));fvm_text_layout_destroy(l);}
 o.alignment=FVM_TEXT_ALIGN_LEFT;o.max_width=0;
 assert(fvm_text_layout_create_periodic(font,"A\tV",3,&o,0,0,0,40,&l)==FVM_OK && fvm_text_layout_path(l,&p)==FVM_OK && fvm_text_layout_glyph_path(l,1,&tab)==FVM_OK && fvm_path_count(tab)==0);
 fvm_font_destroy(font);fvm_text_layout_destroy(l);assert(fvm_path_count(p)>0);fvm_path_destroy(tab);fvm_path_destroy(p);
#else
 assert(fvm_text_layout_create_periodic(font,0,0,&o,0,0,0,40,&l)==FVM_UNSUPPORTED && !l);
#endif
 return 0;
}
