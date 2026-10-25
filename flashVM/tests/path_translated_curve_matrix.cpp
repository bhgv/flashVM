#include "translated_curve_fixture.hpp"


int main()
{
    for (int e = -900; e <= 19; e += 31)
    {
        for (unsigned k = 0; k < 8; ++k)
        {
            for (unsigned a = 0; a < 3; ++a)
            {
                for (unsigned f = 0; f < 2; ++f)
                {
                    tc_check(k, e, a, f != 0, false, false);
                }
            }
        }
    }
    return 0;
}
