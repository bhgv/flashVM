#include <stdio.h>
#include <stdlib.h>
#include "linear_curve_fixture.hpp"


int main()
{
    for (unsigned c = 0; c < 3; ++c)
    {
        for (unsigned j = 0; j < 2; ++j)
        {
            FVM_Path *p = ac_path(3);
            if (!(fvm_path_set_point(p, 1, 0, 14, 8) == FVM_OK && fvm_path_set_point(p, 1, 1, 2, 8) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ac_expected(stroke_bounds(p, c, j), c);
            fvm_path_destroy(p);
        }
    }
    return 0;
}
