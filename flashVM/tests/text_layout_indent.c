/* First-line indentation changes only the first wrapped span in each hard paragraph. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
int main(void)
{
 FVM_Font *font=0;FVM_TextLayout *layout=0;FVM_TextLayoutOptions o={102,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_LEFT,0};FVM_Result r;
#ifdef FVM_HAS_FONTS
 FVM_TextLayoutMetrics m;FVM_TextLayoutLine a,b,c;FVM_Path *path=0;
 assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
 r=fvm_text_layout_create_indented(font,"A V A V\nAV",10,&o,FVM_FONT_SHAPE_NO_KERNING,0,30,&layout);assert(r==FVM_OK);
 assert(fvm_text_layout_metrics(layout,&m)==FVM_OK && m.line_count==3 && m.advance_width==102);
 assert(fvm_text_layout_line(layout,0,&a)==FVM_OK && fvm_text_layout_line(layout,1,&b)==FVM_OK && fvm_text_layout_line(layout,2,&c)==FVM_OK);
 assert(a.x==30 && a.advance==72 && b.x==0 && b.advance==60 && c.x==30 && c.advance==48 && a.byte_end==4 && c.byte_start==8);
 assert(fvm_text_layout_path(layout,&path)==FVM_OK);fvm_font_destroy(font);fvm_text_layout_destroy(layout);assert(fvm_path_count(path)>0);fvm_path_destroy(path);
#else
 r=fvm_text_layout_create_indented(font,"AV",2,&o,0,0,30,&layout);assert(r==FVM_UNSUPPORTED && !layout);
#endif
 return 0;
}
