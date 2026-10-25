#include "cubic_scale_fixture.hpp"


int main()
{
    for (unsigned k = 0; k < 3; ++k)
    {
        for (int e = -500; e >= -550; --e)
        {
            cb_check(k, e);
        }
    }
    return 0;
}
