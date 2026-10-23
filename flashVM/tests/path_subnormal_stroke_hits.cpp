#include "subnormal_stroke_fixture.hpp"
int main() {
for(int e=-510;e>=-545;--e) for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) {
 double s=std::ldexp(1.0,e); FVM_Path *p=hit_line(0,0,3*s,4*s),*unit=hit_line(0,0,3,4);
 const double queries[][2]={{1.5,2},{3.06,.83},{3.18,.74},{-.4,-.6},{-.9,-1.2},{4.4,5.8},{4.8,6.4},{9,9}};
 for(unsigned q=0;q<sizeof(queries)/sizeof(queries[0]);++q) assert(selected_hit(p,queries[q][0]*s,queries[q][1]*s,c,j,4*s)==selected_hit(unit,queries[q][0],queries[q][1],c,j));
 fvm_path_destroy(p);fvm_path_destroy(unit);
}
return 0; }
