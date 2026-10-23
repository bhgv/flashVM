#include "path_style_bounds_fixture.hpp"
int main() {
 for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) {
  FVM_Path *p=hit_line(4,4,12,4); assert(hit_add(p,FVM_PATH_LINE,12,12)==FVM_OK); assert(hit_add(p,FVM_PATH_CLOSE)==FVM_OK);
  FVM_PathBounds a=stroke_bounds(p,cap,join),b=stroke_bounds(p,0,join); bounds_equal(a,b.x_min,b.y_min,b.x_max,b.y_max);
  assert(hit_add(p,FVM_PATH_MOVE,30,8)==FVM_OK); assert(hit_add(p,FVM_PATH_LINE,40,8)==FVM_OK);
  a=stroke_bounds(p,cap,join); bound_near(a.x_max,cap==1?40:42); fvm_path_destroy(p);
  p=hit_line(2,8,2,8); bounds_equal(stroke_bounds(p,cap,join,4,0),0,0,0,0); fvm_path_destroy(p);
 }
 return 0;
}
