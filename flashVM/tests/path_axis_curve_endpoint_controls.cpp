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
                for (unsigned k = 0; k < 2; ++k)
                {
                    FVM_Path *p = ac_path(d);
                    for (unsigned n = 0; n < d - 1; ++n)
                    {
                        if (!(fvm_path_set_point(p, 1, n, k ? 14 : 2, 8) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    ac_expected(stroke_bounds(p, c, j), c);
                    fvm_path_destroy(p);
                }
            }
        }
    }
    return 0;
}
