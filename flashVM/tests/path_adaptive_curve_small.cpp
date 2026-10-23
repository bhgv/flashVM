#include "adaptive_curve_fixture.hpp"
int main() { for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) { double scale=std::ldexp(1.0,-400); FVM_Path *p=dc_path(3,false,-400); FVM_PathBounds b=stroke_bounds(p,c,j,4*scale); b.x_min/=scale; b.y_min/=scale; b.x_max/=scale; b.y_max/=scale; lc_expected(b,c); fvm_path_destroy(p); } return 0; }
