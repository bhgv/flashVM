#include "cubic_scale_fixture.hpp"


int main()
{
    for (unsigned k = 0; k < 3; ++k)
    {
        for (int e = -10; e >= -1000; e -= 30)
        {
            cb_check(k, e);
        }
    }
    return 0;
}
