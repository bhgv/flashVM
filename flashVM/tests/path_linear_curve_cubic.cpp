#include "linear_curve_fixture.hpp"
int main() { for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) { FVM_Path *p=lc_path(3); lc_expected(stroke_bounds(p,c,j),c); fvm_path_destroy(p); } return 0; }
