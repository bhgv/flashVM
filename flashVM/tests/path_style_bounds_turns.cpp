#include "path_style_bounds_fixture.hpp"
int main() {
 FVM_Path *p=hit_line(-4,4,0,0); assert(hit_add(p,FVM_PATH_LINE,4,4)==FVM_OK);
 double q=std::sqrt(2.0);
 bounds_equal(stroke_bounds(p,1,0),-4-q,-2,4+q,4+q);
 bounds_equal(stroke_bounds(p,1,1),-4-q,-q,4+q,4+q); fvm_path_destroy(p);
 p=hit_line(0,0,.1,0); assert(hit_add(p,FVM_PATH_LINE,.1,.1)==FVM_OK);
 for(unsigned j=0;j<2;++j) bounds_equal(stroke_bounds(p,1,j),-1.9,-2,2.1,2);
 fvm_path_destroy(p); p=hit_line(0,0,.1,0); assert(hit_add(p,FVM_PATH_LINE,.2,0)==FVM_OK);
 bounds_equal(stroke_bounds(p,1,0),0,-2,.2,2); fvm_path_destroy(p); return 0;
}
