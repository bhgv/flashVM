/* Nonzero fill must preserve a stem and fill its counter where the bar crosses a glyph. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include "fixtures/font_mixed_data.h"
#include <assert.h>
int main(void)
{
#ifdef FVM_HAS_FONTS
 FVM_Font *font=0;FVM_TextLayout *layout=0;FVM_Path *glyphs=0,*bar=0;int hit;
 FVM_Affine identity={1,0,0,1,0,0};
 assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK && fvm_text_layout_create(font,"A",1,0,&layout)==FVM_OK);
 assert(fvm_text_layout_path(layout,&glyphs)==FVM_OK);
 /* At y=26 the A stem contains x=5; its counter excludes x=10. */
 assert(fvm_path_hit_test(glyphs,5,26,0,0,&hit)==FVM_OK && hit);
 assert(fvm_path_hit_test(glyphs,10,26,0,0,&hit)==FVM_OK && !hit);
 assert(fvm_text_layout_line_underline_path(layout,0,-10,2,&bar)==FVM_OK);
 assert(fvm_path_append(glyphs,bar,&identity)==FVM_OK);
 assert(fvm_path_hit_test(glyphs,1,26,0,0,&hit)==FVM_OK && hit);
 assert(fvm_path_hit_test(glyphs,5,26,0,0,&hit)==FVM_OK && hit);
 assert(fvm_path_hit_test(glyphs,10,26,0,0,&hit)==FVM_OK && hit);
 assert(fvm_path_hit_test(glyphs,1,29,0,0,&hit)==FVM_OK && !hit);
 fvm_path_destroy(bar);fvm_path_destroy(glyphs);fvm_text_layout_destroy(layout);fvm_font_destroy(font);

 /* Overlapping bars on an ink line and a whitespace-only line must not cancel.
  * A one-pixel baseline step models accepted negative leading in a field. */
 {
  FVM_TextLayoutOptions options={0,1,FVM_TEXT_LTR,FVM_TEXT_ALIGN_LEFT,0};unsigned line;
  assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK && fvm_text_layout_create(font,"A\n ",3,&options,&layout)==FVM_OK);
  assert(fvm_text_layout_path(layout,&glyphs)==FVM_OK);
  for(line=0;line<2;++line){assert(fvm_text_layout_line_underline_path(layout,line,4,2,&bar)==FVM_OK && fvm_path_append(glyphs,bar,&identity)==FVM_OK);fvm_path_destroy(bar);}
  assert(fvm_path_hit_test(glyphs,1,40.5,0,0,&hit)==FVM_OK && hit);
  fvm_path_destroy(glyphs);fvm_text_layout_destroy(layout);fvm_font_destroy(font);
 }
 assert(fvm_font_read(fvm_test_font_mixed,sizeof(fvm_test_font_mixed),0,40,&font)==FVM_OK && fvm_text_layout_create(font,"AV",2,0,&layout)==FVM_OK);
 bar=(FVM_Path*)1;
 assert(fvm_text_layout_line_underline_path(layout,0,4,2,&bar)==FVM_UNSUPPORTED && !bar);
 assert(fvm_text_layout_path(layout,&glyphs)==FVM_OK);fvm_path_destroy(glyphs);
 fvm_text_layout_destroy(layout);fvm_font_destroy(font);
#endif
 return 0;
}
