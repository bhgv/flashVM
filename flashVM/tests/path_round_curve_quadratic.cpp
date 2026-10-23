#include "round_curve_fixture.hpp"
int main() { FVM_Path *p=rc_path(); bounds_equal(stroke_bounds(p,0),0,2,16,10); FVM_PathBounds center; assert(fvm_path_bounds(p,&center)==FVM_OK); bounds_equal(center,2,4,14,8); fvm_path_destroy(p); return 0; }
