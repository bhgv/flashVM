#include "path_style_hit_fixture.hpp"
int main() {
 FVM_Path *p=hit_line(); const double bad[]={-1,HUGE_VAL,HUGE_VAL-HUGE_VAL,1000001};
 for(unsigned i=0;i<4;++i) { int out=77; assert(fvm_path_hit_test_style(p,2,8,bad[i],0,&out,0,0)==FVM_INVALID_ARGUMENT && out==77); }
 const unsigned styles[][2]={{3,0},{0,2},{~0U,0},{0,~0U}};
 for(unsigned i=0;i<4;++i) { int out=77; assert(fvm_path_hit_test_style(p,2,8,4,0,&out,styles[i][0],styles[i][1])==FVM_INVALID_ARGUMENT && out==77); }
 int out=77; assert(fvm_path_hit_test_style(0,2,8,4,0,&out,0,0)==FVM_INVALID_ARGUMENT && out==77);
 assert(fvm_path_hit_test_style(p,2,8,4,0,0,0,0)==FVM_INVALID_ARGUMENT);
 assert(fvm_path_hit_test_style(p,2,8,4,2,&out,0,0)==FVM_INVALID_ARGUMENT && out==77);
 assert(fvm_path_hit_test_style(p,HUGE_VAL,8,4,0,&out,0,0)==FVM_INVALID_ARGUMENT && out==77);
 assert(hit_add(p,FVM_PATH_LINE,14,12)==FVM_OK); assert(hit_add(p,FVM_PATH_LINE,2,12)==FVM_OK);
 for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) {
  int legacy=0,styled=0; assert(fvm_path_hit_test(p,8,10,0,0,&legacy)==FVM_OK);
  assert(fvm_path_hit_test_style(p,8,10,0,0,&styled,cap,join)==FVM_OK && legacy==styled && styled);
 }
 fvm_path_destroy(p); return 0;
}
