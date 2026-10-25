#include <stdio.h>
#include <stdlib.h>
#include "cubic_scale_fixture.hpp"


int main()
{
    for (unsigned k = 0; k < 7; ++k)
    {
        FVM_Path *source = cb_path(k, 0), *target = 0;
        if (!(fvm_path_create(&target) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        double s = std::ldexp(1.0, -600);
        FVM_Affine m = {s, 0, 0, s, 0, 0};
        if (!(fvm_path_append(target, source, &m) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_PathBounds b = cb_normalized(target, -600), expected = cb_expected(k);
        bounds_equal(b, expected.x_min, expected.y_min, expected.x_max, expected.y_max);
        fvm_path_destroy(source);
        fvm_path_destroy(target);
    }
    return 0;
}
