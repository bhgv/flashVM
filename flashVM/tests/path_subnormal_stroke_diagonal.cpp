#include "subnormal_stroke_fixture.hpp"
int main() {
for(int e=-505;e>=-545;--e) for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) {
 double s=std::ldexp(1.0,e); FVM_Path *p=hit_line(0,0,3*s,4*s);
 FVM_PathBounds b=sn_unit(stroke_bounds(p,c,j,4*s),s);lc_expected(b,c,4,0,0,3,4); fvm_path_destroy(p);
}
return 0; }
