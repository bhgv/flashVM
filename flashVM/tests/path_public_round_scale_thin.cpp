#include "public_round_scale_fixture.hpp"
int main(){FVM_Path *p=hit_line(0,0,10,0);double w=std::ldexp(2.0,-600);pr_hit(p,5,w, w,0);pr_hit(p,5,w/4,w,1);fvm_path_destroy(p);return 0;}
