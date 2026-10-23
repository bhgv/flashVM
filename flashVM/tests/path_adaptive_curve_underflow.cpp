#include "adaptive_curve_fixture.hpp"
int main() { FVM_Path *p=dc_path(3,false,-600); for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) if(c||j) { FVM_PathBounds b={1,2,3,4},before=b; int any=77; assert(fvm_path_stroke_bounds_style(p,4,c,j,&b,&any)==FVM_UNSUPPORTED && any==77 && std::memcmp(&b,&before,sizeof(b))==0); } fvm_path_destroy(p); return 0; }
