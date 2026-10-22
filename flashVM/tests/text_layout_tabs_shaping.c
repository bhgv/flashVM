/* Tracking, indentation and word ligatures preserve fixed tab anchors and source glyphs. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include "fixtures/font_ligature_data.h"
#include <assert.h>
#include <math.h>
int main(void)
{
#ifdef FVM_HAS_FONTS
 FVM_Font *font=0;FVM_TextLayout *l=0,*base=0;FVM_TextLayoutGlyph g;FVM_TextLayoutMetrics m,n;FVM_TextLayoutOptions o={0,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_LEFT,0};double stops[]={40,80};unsigned i;
 assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);for(i=0;i<2;++i){assert(fvm_text_layout_create_tabbed(font,"A\tV",3,&o,FVM_FONT_SHAPE_NO_KERNING,0.5,i?-10:10,stops,2,&l)==FVM_OK && fvm_text_layout_metrics(l,&m)==FVM_OK && m.advance_width==64.5 && fvm_text_layout_glyph(l,2,&g)==FVM_OK && g.x==40);fvm_text_layout_destroy(l);}assert(fvm_text_layout_create_tabbed(font,"A\tV",3,&o,FVM_FONT_SHAPE_NO_KERNING,0,16,stops,2,&l)==FVM_OK && fvm_text_layout_glyph(l,2,&g)==FVM_OK && g.x==80);fvm_text_layout_destroy(l);fvm_font_destroy(font);
 assert(fvm_font_read(fvm_test_font_ligatures,sizeof(fvm_test_font_ligatures),0,40,&font)==FVM_OK && fvm_text_layout_create(font,"fi",2,&o,&base)==FVM_OK && fvm_text_layout_metrics(base,&n)==FVM_OK && n.glyph_count==1);assert(fvm_text_layout_create_tabbed(font,"fi\tfi",5,&o,0,0,0,stops,2,&l)==FVM_OK && fvm_text_layout_metrics(l,&m)==FVM_OK && m.glyph_count==3 && fabs(m.advance_width-40-n.advance_width)<1e-9 && fvm_text_layout_glyph(l,2,&g)==FVM_OK && g.x==40);fvm_text_layout_destroy(l);fvm_text_layout_destroy(base);fvm_font_destroy(font);
#endif
 return 0;
}
