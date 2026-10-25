#include <stdio.h>
#include <stdlib.h>
#include "linear_curve_fixture.hpp"


int main()
{
    for (unsigned k = 0; k < 3; ++k)
    {
        FVM_Path *p = lc_path();
        if (!(fvm_path_set_point(p, 1, 0, k == 2 ? 15 : 8, k == 0 ? 7.0625 : k == 1 ? 7.000000001 : 10.5) == FVM_OK))
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
                    FVM_PathBounds before = {1, 2, 3, 4}, out = before;
                    int any = 77;
                    if (!(fvm_path_stroke_bounds_style(p, 4, c, j, &out, &any) == FVM_UNSUPPORTED && any == 77 &&
                          std::memcmp(&out, &before, sizeof(out)) == 0))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        fvm_path_destroy(p);
    }
    return 0;
}
