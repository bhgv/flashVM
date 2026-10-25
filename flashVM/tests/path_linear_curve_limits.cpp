#include <stdio.h>
#include <stdlib.h>
#include "linear_curve_fixture.hpp"


int main()
{
    for (unsigned c = 0; c < 3; ++c)
    {
        for (unsigned j = 0; j < 2; ++j)
        {
            FVM_Path *p = 0;
            if (!(fvm_path_create(&p) == FVM_OK && hit_add(p, FVM_PATH_MOVE, -1000000, -500000) == FVM_OK &&
                  hit_add(p, FVM_PATH_CUBIC, 1000000, 500000, -1000000, -500000, 1000000, 500000) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            lc_expected(stroke_bounds(p, c, j, 1000000), c, 1000000, -1000000, -500000, 1000000, 500000);
            fvm_path_destroy(p);
        }
    }
    return 0;
}
