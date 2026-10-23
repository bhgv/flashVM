#include "cubic_scale_fixture.hpp"
int main(){for(unsigned k=0;k<7;++k){FVM_Path *p=cb_path(k,0);double s=std::ldexp(1.0,-600);FVM_Affine m={s,0,0,s,0,0};assert(fvm_path_transform(p,&m)==FVM_OK);FVM_PathBounds b=cb_normalized(p,-600),expected=cb_expected(k);bounds_equal(b,expected.x_min,expected.y_min,expected.x_max,expected.y_max);fvm_path_destroy(p);}return 0;}
