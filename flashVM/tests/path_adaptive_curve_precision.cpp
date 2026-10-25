#include <stdio.h>
#include <stdlib.h>
#include "adaptive_curve_fixture.hpp"


int main()
{
    FVM_Path *p = lc_path();
    if (!(fvm_path_set_point(p, 1, 0, 8, 7 + std::ldexp(1.0, -30)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned c = 0; c < 3; ++c)
    {
        for (unsigned j = 0; j < 2; ++j)
        {
            if (c || j)
            {
                FVM_PathBounds b = {1, 2, 3, 4}, before = b;
                int any = 77;
                if (!(fvm_path_stroke_bounds_style(p, 4, c, j, &b, &any) == FVM_UNSUPPORTED && any == 77 &&
                      std::memcmp(&b, &before, sizeof(b)) == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    fvm_path_destroy(p);
    return 0;
}
