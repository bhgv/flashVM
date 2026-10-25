#include "public_round_scale_fixture.hpp"


int main()
{
    for (int e = -480; e <= 10; e += 10)
    {
        for (unsigned a = 0; a < 3; ++a)
        {
            pr_check(e, a);
        }
    }
    return 0;
}
