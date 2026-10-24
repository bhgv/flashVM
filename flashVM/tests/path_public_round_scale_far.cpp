#include "public_round_scale_fixture.hpp"
int main(){FVM_Path *p=hit_line(0,0,std::ldexp(10.0,-600),0);pr_hit(p,1,1,std::ldexp(2.0,-600),0);pr_hit(p,0,0,std::ldexp(2.0,-600),1);fvm_path_destroy(p);return 0;}
