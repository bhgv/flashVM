#include "public_round_scale_fixture.hpp"


int main()
{
    for (int e = -1000; e <= -500; e += 10)
    {
        pr_edit(e, true);
    }
    return 0;
}
