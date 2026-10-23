#include "linear_curve_fixture.hpp"
int main() { for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) { FVM_Path *p=lc_path(3); FVM_Affine m={.5,0,0,.5,.0625,.0625}; assert(fvm_path_transform(p,&m)==FVM_OK); lc_expected(stroke_bounds(p,c,j),c,4,1.0625,2.0625,7.0625,5.0625); fvm_path_destroy(p); } return 0; }
