/* Analytical LTR justification preserves source clusters while filling nonfinal lines. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
int main(void)
{
 FVM_TextLayout *layout=0;
#ifdef FVM_HAS_FONTS
 FVM_Font *font=0;FVM_TextLayoutMetrics m;FVM_TextLayoutLine line;FVM_TextLayoutGlyph glyph;FVM_Path *path=0;
 FVM_TextLayoutOptions o={90,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_JUSTIFY,0};
 assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
 assert(fvm_text_layout_create_configured(font,"A V A V",7,&o,FVM_FONT_SHAPE_NO_KERNING,&layout)==FVM_OK);
 assert(fvm_text_layout_metrics(layout,&m)==FVM_OK && m.line_count==2 && m.glyph_count==7 && fabs(m.advance_width-90)<0.02);
 assert(fvm_text_layout_line(layout,0,&line)==FVM_OK && line.byte_start==0 && line.byte_end==4 && fabs(line.advance-90)<0.02);
 assert(fvm_text_layout_glyph(layout,2,&glyph)==FVM_OK && glyph.shaped.cluster==2 && fabs(glyph.x-66)<0.02);
 assert(fvm_text_layout_glyph(layout,3,&glyph)==FVM_OK && glyph.shaped.cluster==3 && glyph.shaped.x_advance==0);
 assert(fvm_text_layout_line(layout,1,&line)==FVM_OK && line.byte_start==4 && line.byte_end==7 && fabs(line.advance-60)<0.02);
 assert(fvm_text_layout_path(layout,&path)==FVM_OK);fvm_text_layout_destroy(layout);fvm_font_destroy(font);assert(fvm_path_count(path)>0);fvm_path_destroy(path);
#else
 assert(fvm_text_layout_create(0,"A V",3,0,&layout)==FVM_UNSUPPORTED && !layout);
#endif
 return 0;
}
