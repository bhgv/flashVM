#include "path_style_hit_fixture.hpp"
int main() {
 for(unsigned degree=2;degree<=3;++degree) for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) {
  FVM_Path *p=0; assert(fvm_path_create(&p)==FVM_OK); assert(hit_add(p,FVM_PATH_MOVE,2,8)==FVM_OK);
  if(degree==2) assert(hit_add(p,FVM_PATH_QUADRATIC,8,8,14,8)==FVM_OK);
  else assert(hit_add(p,FVM_PATH_CUBIC,6,8,10,8,14,8)==FVM_OK);
  assert(selected_hit(p,1,8,cap,join)==(cap!=1)); assert(selected_hit(p,.1,6.1,cap,join)==(cap==2));
  assert(selected_hit(p,8,9.9,cap,join)); fvm_path_destroy(p);
  assert(fvm_path_create(&p)==FVM_OK); assert(hit_add(p,FVM_PATH_MOVE,0,0)==FVM_OK);
  if(degree==2) assert(hit_add(p,FVM_PATH_QUADRATIC,8,16,16,0)==FVM_OK);
  else assert(hit_add(p,FVM_PATH_CUBIC,0,16,16,16,16,0)==FVM_OK);
  assert(selected_hit(p,8,degree==2?8:12,cap,join)); assert(!selected_hit(p,8,20,cap,join)); fvm_path_destroy(p);
 }
 return 0;
}
