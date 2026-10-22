/* Whitespace boundaries, hard paragraphs and word clusters are independent of gap expansion. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
int main(void)
{
#ifdef FVM_HAS_FONTS
 FVM_Font *font=0;FVM_TextLayout *layout=0;FVM_TextLayoutMetrics m;FVM_TextLayoutLine line;FVM_TextLayoutGlyph g;
 FVM_TextLayoutOptions o={90,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_JUSTIFY,0};
 assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
 assert(fvm_text_layout_create(font,"A  V A",6,&o,&layout)==FVM_OK && fvm_text_layout_line(layout,0,&line)==FVM_OK && line.advance==90);
 assert(fvm_text_layout_glyph(layout,1,&g)==FVM_OK && g.shaped.x_advance==21 && fvm_text_layout_glyph(layout,3,&g)==FVM_OK && g.x==66);fvm_text_layout_destroy(layout);
 assert(fvm_text_layout_create(font," A V A",6,&o,&layout)==FVM_OK && fvm_text_layout_glyph(layout,0,&g)==FVM_OK && g.shaped.x_advance==12 && fvm_text_layout_glyph(layout,3,&g)==FVM_OK && g.x==66);fvm_text_layout_destroy(layout);
 assert(fvm_text_layout_create(font,"A V \nA V ",9,&o,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&m)==FVM_OK && m.line_count==2 && m.advance_width==60);fvm_text_layout_destroy(layout);
 assert(fvm_text_layout_create(font,"A V \r\n",6,&o,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&m)==FVM_OK && m.line_count==2 && fvm_text_layout_line(layout,0,&line)==FVM_OK && line.byte_next==6 && line.advance==60);fvm_text_layout_destroy(layout);
 assert(fvm_text_layout_create(font,"fi fi fi",8,&o,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&m)==FVM_OK && m.glyph_count==5 && fvm_text_layout_glyph(layout,2,&g)==FVM_OK && g.shaped.cluster==3 && g.x==62);fvm_text_layout_destroy(layout);
 assert(fvm_text_layout_create_spaced(font,"                 ",17,&o,0,0.1,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&m)==FVM_OK && m.advance_width==0);fvm_text_layout_destroy(layout);
 o.max_width=50;assert(fvm_text_layout_create_configured(font,"AVAV",4,&o,FVM_FONT_SHAPE_NO_KERNING,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&m)==FVM_OK && m.advance_width==48);fvm_text_layout_destroy(layout);
 assert(fvm_text_layout_create(font,"A \xcc\x81 V A",8,&o,&layout)==FVM_UNSUPPORTED && !layout);
 assert(fvm_text_layout_create(font,"\xd7\x90 \xd7\x91",5,&o,&layout)==FVM_UNSUPPORTED && !layout);
 o.alignment=(FVM_TextAlignment)5;assert(fvm_text_layout_create(font,"A V",3,&o,&layout)==FVM_INVALID_ARGUMENT && !layout);fvm_font_destroy(font);
#endif
 return 0;
}
