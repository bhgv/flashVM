#include <stdio.h>
#include <stdlib.h>
#include "cubic_scale_fixture.hpp"


int main()
{
    for (unsigned k = 0; k < 7; ++k)
    {
        FVM_Path *p = cb_path(k, -600);
        FVM_PathBounds b = cb_normalized(p, -600);
        double points[4];
        cb_values(k, points);
        for (unsigned i = 0; i <= 1024; ++i)
        {
            long double v = cb_evaluate(points, (long double)i / 1024, k != 6);
            if (!(v >= b.y_min - 1e-10 && v <= b.y_max + 1e-10))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_path_destroy(p);
    }
    return 0;
}
