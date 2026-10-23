#include "round_curve_fixture.hpp"
int main() { FVM_Path *p=rc_path(); FVM_Affine m={0,2,-2,0,30,10}; assert(fvm_path_transform(p,&m)==FVM_OK); bounds_equal(stroke_bounds(p,0,0,8),10,10,26,42); fvm_path_destroy(p); return 0; }
