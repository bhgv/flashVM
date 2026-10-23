#include "axis_curve_fixture.hpp"
int main() { for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) { FVM_Path *p=ac_path(); assert(hit_add(p,FVM_PATH_MOVE,100,100)==FVM_OK); ac_expected(stroke_bounds(p,c,j),c); assert(hit_add(p,FVM_PATH_QUADRATIC,100,100,100,100)==FVM_OK); ac_expected(stroke_bounds(p,c,j),c); fvm_path_destroy(p); } return 0; }
