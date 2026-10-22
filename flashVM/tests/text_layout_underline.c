/* Line-level bars use placed alignment, tracked advances and immutable owned geometry. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
int main(void)
{
 FVM_Path *path=(FVM_Path*)1;
 assert(fvm_text_layout_line_underline_path(0,0,4,2,&path)==FVM_INVALID_ARGUMENT && !path);
#ifdef FVM_HAS_FONTS
 {
  FVM_Font *font=0;FVM_TextLayout *layout=0;FVM_FontUnderlineMetrics u;
  FVM_TextLayoutOptions o={100,48,FVM_TEXT_LTR,FVM_TEXT_ALIGN_RIGHT,0};
  FVM_TextLayoutLine line;FVM_PathBounds box;FVM_PathCommand command;
  assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK && fvm_font_underline_metrics(font,&u)==FVM_OK);
  assert(fvm_text_layout_create_spaced(font,"AV\n",3,&o,FVM_FONT_SHAPE_NO_KERNING,2,&layout)==FVM_OK);
  assert(fvm_text_layout_line(layout,0,&line)==FVM_OK && fabs(line.x-48)<0.02);
  assert(fvm_text_layout_line_underline_path(layout,0,u.center_y,u.thickness,&path)==FVM_OK);
  assert(fvm_path_count(path)==5 && fvm_path_get(path,4,&command)==FVM_OK && command.verb==FVM_PATH_CLOSE);
  assert(fvm_path_bounds(path,&box)==FVM_OK && fabs(box.x_min-48)<0.02 && fabs(box.x_max-100)<0.02 && fabs(box.y_min-(line.baseline+3))<0.02 && fabs(box.y_max-(line.baseline+5))<0.02);
  fvm_path_destroy(path);
  assert(fvm_text_layout_line_underline_path(layout,1,4,2,&path)==FVM_OK && fvm_path_count(path)==0);fvm_path_destroy(path);
  path=(FVM_Path*)1;assert(fvm_text_layout_line_underline_path(layout,2,4,2,&path)==FVM_INVALID_ARGUMENT && !path);
  assert(fvm_text_layout_line_underline_path(layout,0,4,0,&path)==FVM_INVALID_ARGUMENT && !path);
  assert(fvm_text_layout_line_underline_path(layout,0,HUGE_VAL,2,&path)==FVM_INVALID_ARGUMENT && !path);
  assert(fvm_text_layout_line_underline_path(layout,0,4,2,&path)==FVM_OK);
  fvm_font_destroy(font);fvm_text_layout_destroy(layout);
  assert(fvm_path_bounds(path,&box)==FVM_OK && fabs(box.x_max-100)<0.02);fvm_path_destroy(path);
 }
#endif
 return 0;
}
