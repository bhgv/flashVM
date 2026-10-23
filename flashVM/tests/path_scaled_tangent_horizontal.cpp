#include "linear_curve_fixture.hpp"
int main() {
 for(int e=-540;e>=-1000;e-=20) for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) {
  double scale=std::ldexp(1.0,e); FVM_Path *p=hit_line(0,0,3*scale,0*scale);
  FVM_PathBounds b=stroke_bounds(p,c,j,4*scale);
  b.x_min/=scale; b.y_min/=scale; b.x_max/=scale; b.y_max/=scale;
  lc_expected(b,c,4,0,0,3,0);
  assert(selected_hit(p,3*scale/2,0*scale/2,c,j,4*scale));
  fvm_path_destroy(p);
 } return 0;
}
