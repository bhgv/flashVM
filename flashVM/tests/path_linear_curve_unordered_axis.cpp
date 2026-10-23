#include "linear_curve_fixture.hpp"
int main() { for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) { FVM_Path *p=ac_path(3); assert(fvm_path_set_point(p,1,0,14,8)==FVM_OK && fvm_path_set_point(p,1,1,2,8)==FVM_OK); ac_expected(stroke_bounds(p,c,j),c); fvm_path_destroy(p); } return 0; }
