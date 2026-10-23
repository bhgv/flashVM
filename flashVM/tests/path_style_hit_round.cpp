#include "path_style_hit_fixture.hpp"
int main() {
 for(unsigned closed=0;closed<2;++closed) for(unsigned curve=0;curve<3;++curve) {
  FVM_Path *p=hit_line(2,2,14,2);
  if(curve==0) assert(hit_add(p,FVM_PATH_LINE,8,14)==FVM_OK);
  if(curve==1) assert(hit_add(p,FVM_PATH_QUADRATIC,14,14,2,14)==FVM_OK);
  if(curve==2) assert(hit_add(p,FVM_PATH_CUBIC,18,6,18,12,2,14)==FVM_OK);
  if(closed) assert(hit_add(p,FVM_PATH_CLOSE)==FVM_OK);
  for(int y=-2;y<20;++y) for(int x=-2;x<20;++x) {
   int legacy=0; assert(fvm_path_hit_test(p,x+.371,y+.263,4,0,&legacy)==FVM_OK);
   assert(legacy==selected_hit(p,x+.371,y+.263,0));
  }
  fvm_path_destroy(p);
 }
 return 0;
}
