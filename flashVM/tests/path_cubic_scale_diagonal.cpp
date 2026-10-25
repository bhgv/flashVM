#include "cubic_scale_fixture.hpp"


int main()
{
    for (unsigned k = 0; k < 3; ++k)
    {
        for (unsigned reverse = 0; reverse < 2; ++reverse)
        {
            for (int e = -400; e >= -1000; e -= 100)
            {
                cb_check(k, e, 2, reverse != 0);
            }
        }
    }
    return 0;
}
