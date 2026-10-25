#include "translated_curve_fixture.hpp"


int main()
{
    for (unsigned k = 0; k < 8; ++k)
    {
        tc_check(k, 19, 2, true, true, false);
    }
    return 0;
}
