#include "path_style_hit_fixture.hpp"
int main() {
 for(unsigned reverse=0;reverse<2;++reverse) for(unsigned cap=0;cap<3;++cap) {
  FVM_Path *p=hit_line(4,reverse?12:4,12,4); assert(hit_add(p,FVM_PATH_LINE,12,reverse?12:12)==FVM_OK);
  if(!reverse) {
   assert(selected_hit(p,13.3,2.7,cap,0)); assert(!selected_hit(p,13.3,2.7,cap,1));
   assert(selected_hit(p,12.75,3.25,cap,1)); assert(!selected_hit(p,13.8,2.2,cap,0));
  }
  assert(selected_hit(p,12,4,cap,0)); assert(selected_hit(p,12,4,cap,1)); fvm_path_destroy(p);
 }
 FVM_Path *p=hit_line(2,8,8,8); assert(hit_add(p,FVM_PATH_LINE,2,8)==FVM_OK);
 assert(selected_hit(p,9,8,1,0)); assert(!selected_hit(p,9,8,1,1)); fvm_path_destroy(p);
 p=hit_line(0,0,.1,0); assert(hit_add(p,FVM_PATH_LINE,.1,.1)==FVM_OK);
 for(unsigned join=0;join<2;++join) { assert(!selected_hit(p,-.5,.5,1,join)); assert(selected_hit(p,.5,-.5,1,join)); }
 fvm_path_destroy(p); p=hit_line(0,0,.1,0); assert(hit_add(p,FVM_PATH_LINE,.2,0)==FVM_OK);
 assert(!selected_hit(p,-.5,0,1,0)); fvm_path_destroy(p);
 return 0;
}
