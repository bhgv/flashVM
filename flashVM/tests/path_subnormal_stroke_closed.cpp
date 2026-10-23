#include "subnormal_stroke_fixture.hpp"
int main() {
for(int e=-510;e>=-545;--e) for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) {
 double s=std::ldexp(1.0,e);FVM_Path *unit=sn_contour(1,1),*tiny=sn_contour(1,s);
 sn_equal(sn_unit(stroke_bounds(tiny,c,j,4*s),s),stroke_bounds(unit,c,j));
 fvm_path_destroy(tiny);fvm_path_destroy(unit);
}
return 0; }
