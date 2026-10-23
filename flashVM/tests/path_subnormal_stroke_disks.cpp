#include "subnormal_stroke_fixture.hpp"
int main() { for(int e=-515;e>=-1000;e-=5) { double s=std::ldexp(1.0,e); FVM_Path *p=hit_line(0,0,3*s,4*s); assert(selected_hit(p,-.6*s,-.8*s,0,0,4*s));assert(!selected_hit(p,-1.8*s,-2.4*s,0,0,4*s));assert(!selected_hit(p,9*s,9*s,0,0,4*s));fvm_path_destroy(p); } return 0; }
