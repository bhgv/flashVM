#include "round_curve_fixture.hpp"
int main() { FVM_Path *p=0; assert(fvm_path_create(&p)==FVM_OK); assert(hit_add(p,FVM_PATH_MOVE,-1000000,0)==FVM_OK); assert(hit_add(p,FVM_PATH_QUADRATIC,0,1000000,1000000,0)==FVM_OK); bounds_equal(stroke_bounds(p,0,0,1000000),-1500000,-500000,1500000,1000000); fvm_path_destroy(p); return 0; }
