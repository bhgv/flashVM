#include <stdio.h>
#include <stdlib.h>
#include "axis_curve_fixture.hpp"


int main()
{
    for (unsigned d = 2; d <= 3; ++d)
    {
        for (unsigned c = 0; c < 3; ++c)
        {
            for (unsigned j = 0; j < 2; ++j)
            {
                FVM_Path *p = ac_path(d);
                for (unsigned n = 0; n < d; ++n)
                {
                    if (!(fvm_path_set_point(p, 1, n, 2, 8) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                bounds_equal(stroke_bounds(p, c, j, 4, 0), 0, 0, 0, 0);
                fvm_path_destroy(p);
            }
        }
    }
    return 0;
}
