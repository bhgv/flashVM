#include "axis_curve_fixture.hpp"
int main() { for(unsigned d=2;d<=3;++d) for(unsigned v=0;v<2;++v) for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) { FVM_Path *p=ac_path(d,v,true); ac_expected(stroke_bounds(p,c,j),c,4,v); fvm_path_destroy(p); } return 0; }
