/* Optional-ligature suppression is identical during trial wrapping and final placement. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
int main(void)
{
 FVM_TextLayout *layout=(FVM_TextLayout*)1;
 assert(fvm_text_layout_create_spaced(0,"fi",2,0,4,1,&layout)==FVM_INVALID_ARGUMENT && !layout);
#ifdef FVM_HAS_FONTS
 { FVM_Font *font=0;FVM_TextLayoutOptions o={0,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_LEFT,0};FVM_TextLayoutMetrics m;FVM_TextLayoutGlyph g;FVM_Path *path=0;
  assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
  assert(fvm_text_layout_create_spaced(font,"fifi",4,&o,0,1,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&m)==FVM_OK && m.glyph_count==4 && fabs(m.advance_width-52)<0.02);
  assert(fvm_text_layout_glyph(layout,1,&g)==FVM_OK && g.shaped.cluster==1 && fabs(g.x-15)<0.02);
  assert(fvm_text_layout_path(layout,&path)==FVM_OK);fvm_text_layout_destroy(layout);
  o.max_width=26;assert(fvm_text_layout_create_spaced(font,"fifi",4,&o,0,1,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&m)==FVM_OK && m.line_count==2 && m.glyph_count==4 && fabs(m.advance_width-26)<0.02);fvm_text_layout_destroy(layout);
  o.max_width=0;assert(fvm_text_layout_create_spaced(font,"fi",2,&o,0,-1,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&m)==FVM_OK && m.glyph_count==2 && fabs(m.advance_width-22)<0.02);fvm_text_layout_destroy(layout);
  assert(fvm_text_layout_create_spaced(font,"fi",2,&o,0,0,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&m)==FVM_OK && m.glyph_count==1 && fabs(m.advance_width-28)<0.02);fvm_text_layout_destroy(layout);
  assert(fvm_text_layout_create_configured(font,"fi",2,&o,FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&m)==FVM_OK && m.glyph_count==2 && fabs(m.advance_width-24)<0.02);fvm_text_layout_destroy(layout);
  assert(fvm_text_layout_create_spaced(font,"fi",2,&o,0,-10,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&m)==FVM_OK && m.glyph_count==2 && fabs(m.advance_width-4)<0.02);fvm_text_layout_destroy(layout);
  assert(fvm_text_layout_create_spaced(font,"fi",2,&o,0,-10.01,&layout)==FVM_UNSUPPORTED && !layout);
  assert(fvm_text_layout_create_spaced(font,"A\xcc\x81",3,&o,0,1,&layout)==FVM_UNSUPPORTED && !layout);
  fvm_font_destroy(font);assert(fvm_path_count(path)>0);fvm_path_destroy(path);
 }
#else
 assert(fvm_text_layout_create_spaced(0,"fi",2,0,FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES,1,&layout)==FVM_UNSUPPORTED && !layout);
#endif
 return 0;
}
