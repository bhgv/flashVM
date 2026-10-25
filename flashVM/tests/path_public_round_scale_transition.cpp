#include "public_round_scale_fixture.hpp"


int main()
{
    for (int e = -560; e <= -480; ++e)
    {
        for (unsigned a = 0; a < 3; ++a)
        {
            pr_check(e, a);
        }
    }
    return 0;
}
