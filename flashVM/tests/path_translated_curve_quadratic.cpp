#include "translated_curve_fixture.hpp"


int main()
{
    for (int e = -900; e <= 19; e += 17)
    {
        tc_check(6, e, 0, false, false, false);
    }
    return 0;
}
