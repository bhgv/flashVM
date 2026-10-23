#include "axis_curve_fixture.hpp"
int main() { for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) { FVM_Path *p=ac_path(3); ac_expected(stroke_bounds(p,c,j),c); fvm_path_destroy(p); } return 0; }
