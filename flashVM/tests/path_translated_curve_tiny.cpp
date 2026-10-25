#include "translated_curve_fixture.hpp"


int main()
{
    for (int e = -900; e <= -100; e += 20)
    {
        for (unsigned k = 0; k < 8; ++k)
        {
            tc_check(k, e, 0, false, false, false);
        }
    }
    return 0;
}
