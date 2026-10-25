#include "public_round_scale_fixture.hpp"


int main()
{
    for (int e = -1000; e <= 10; e += 19)
    {
        for (unsigned a = 0; a < 3; ++a)
        {
            for (unsigned sign = 0; sign < 2; ++sign)
            {
                for (unsigned z = 0; z < 2; ++z)
                {
                    pr_check(e, a, sign != 0, z != 0);
                }
            }
        }
    }
    return 0;
}
