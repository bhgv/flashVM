#include "cubic_scale_fixture.hpp"


int main()
{
    for (unsigned k = 0; k < 3; ++k)
    {
        for (unsigned axis = 0; axis < 3; ++axis)
        {
            for (int e = -500; e >= -1000; e -= 100)
            {
                cb_check(k, e, axis, false, true);
            }
        }
    }
    return 0;
}
