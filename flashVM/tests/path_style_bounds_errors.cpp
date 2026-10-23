#include "path_style_bounds_fixture.hpp"
int main() {
 FVM_Path *p=hit_line(); FVM_PathBounds before={1,2,3,4},out=before; int any=77;
 const double widths[]={0,-1,1000001,HUGE_VAL,HUGE_VAL-HUGE_VAL};
 for(unsigned i=0;i<5;++i) { assert(fvm_path_stroke_bounds_style(p,widths[i],0,0,&out,&any)==FVM_INVALID_ARGUMENT); assert(std::memcmp(&out,&before,sizeof(out))==0 && any==77); }
 const unsigned styles[][2]={{3,0},{0,2},{~0U,0},{0,~0U}};
 for(unsigned i=0;i<4;++i) { assert(fvm_path_stroke_bounds_style(p,4,styles[i][0],styles[i][1],&out,&any)==FVM_INVALID_ARGUMENT); assert(std::memcmp(&out,&before,sizeof(out))==0 && any==77); }
 assert(fvm_path_stroke_bounds_style(0,4,0,0,&out,&any)==FVM_INVALID_ARGUMENT && any==77);
 assert(fvm_path_stroke_bounds_style(p,4,0,0,0,&any)==FVM_INVALID_ARGUMENT && any==77);
 assert(fvm_path_stroke_bounds_style(p,4,0,0,&out,0)==FVM_INVALID_ARGUMENT && any==77);
 assert(hit_add(p,FVM_PATH_QUADRATIC,8,16,14,8)==FVM_OK);
 assert(fvm_path_stroke_bounds_style(p,4,1,0,&out,&any)==FVM_UNSUPPORTED && any==77 && std::memcmp(&out,&before,sizeof(out))==0);
 fvm_path_destroy(p); p=hit_line(); assert(hit_add(p,FVM_PATH_CUBIC,2,16,14,16,14,8)==FVM_OK);
 assert(fvm_path_stroke_bounds_style(p,4,1,0,&out,&any)==FVM_UNSUPPORTED && any==77 && std::memcmp(&out,&before,sizeof(out))==0);
 fvm_path_destroy(p); return 0;
}
