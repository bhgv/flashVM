#include "public_round_scale_fixture.hpp"


int main()
{
    pr_check(-600, 2, true, true);
    pr_check(-600, 2, true, false);
    return 0;
}
