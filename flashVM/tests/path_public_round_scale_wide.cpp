#include "public_round_scale_fixture.hpp"


int main()
{
    FVM_Path *p = hit_line(0, 0, std::ldexp(10.0, -600), 0);
    pr_hit(p, .5, .5, 2, 1);
    pr_hit(p, 1, 1, 2, 0);
    fvm_path_destroy(p);
    return 0;
}
