#include "path_style_hit_fixture.hpp"
int main() {
 for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) {
  FVM_Path *p=hit_line();
  assert(selected_hit(p,8,8,cap,join)); assert(!selected_hit(p,8,10.01,cap,join));
  assert(selected_hit(p,1,8,cap,join)==(cap!=1)); assert(selected_hit(p,15,8,cap,join)==(cap!=1));
  assert(selected_hit(p,.1,6.1,cap,join)==(cap==2)); assert(selected_hit(p,15.9,9.9,cap,join)==(cap==2));
  assert(selected_hit(p,2,10,cap,join)); assert(!selected_hit(p,-.01,8,cap,join));
  int hit=0; assert(fvm_path_hit_test(p,.1,6.1,4,0,&hit)==FVM_OK && !hit);
  fvm_path_destroy(p);
 }
 return 0;
}
