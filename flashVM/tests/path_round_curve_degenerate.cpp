#include "round_curve_fixture.hpp"
int main() { FVM_Path *p=0; assert(fvm_path_create(&p)==FVM_OK); assert(hit_add(p,FVM_PATH_MOVE,50,60)==FVM_OK); assert(hit_add(p,FVM_PATH_QUADRATIC,50,60,50,60)==FVM_OK); assert(hit_add(p,FVM_PATH_CUBIC,50,60,50,60,50,60)==FVM_OK); bounds_equal(stroke_bounds(p,0,0,4,0),0,0,0,0); fvm_path_destroy(p); return 0; }
