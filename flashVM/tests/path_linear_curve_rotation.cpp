#include <stdio.h>
#include <stdlib.h>
#include "linear_curve_fixture.hpp"


int main()
{
    for (unsigned c = 0; c < 3; ++c)
    {
        for (unsigned j = 0; j < 2; ++j)
        {
            FVM_Path *p = lc_path(3);
            FVM_Affine m = {0, 2, -2, 0, 30, 10};
            if (!(fvm_path_transform(p, &m) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            lc_expected(stroke_bounds(p, c, j, 8), c, 8, 22, 14, 10, 38);
            fvm_path_destroy(p);
        }
    }
    return 0;
}
