#include "subnormal_stroke_fixture.hpp"
int main() {
for(int e=-510;e>=-545;--e) for(unsigned reverse=0;reverse<2;++reverse) for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) {
 double s=std::ldexp(1.0,e);FVM_Path *p=dc_path(3,reverse!=0,e);lc_expected(sn_unit(stroke_bounds(p,c,j,4*s),s),c);fvm_path_destroy(p);
}
return 0; }
