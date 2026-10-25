#include "public_round_scale_fixture.hpp"


int main()
{
    for (unsigned a = 0; a < 3; ++a)
    {
        pr_check(-600, a, true);
    }
    return 0;
}
