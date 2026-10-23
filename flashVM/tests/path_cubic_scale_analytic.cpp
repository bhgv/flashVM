#include "cubic_scale_fixture.hpp"
int main(){FVM_Path *p=cb_path(0,-600);FVM_PathBounds b=cb_normalized(p,-600);bounds_equal(b,0,-2*std::sqrt(3.0),0,2*std::sqrt(3.0));fvm_path_destroy(p);return 0;}
