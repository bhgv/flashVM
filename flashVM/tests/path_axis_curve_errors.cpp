#include <stdio.h>
#include <stdlib.h>
#include "axis_curve_fixture.hpp"


int main()
{
    for (unsigned k = 0; k < 4; ++k)
    {
        FVM_Path *p = ac_path(3);
        if (!(fvm_path_set_point(p, 1, 0, k == 1 ? 1 : k == 2 ? 12 : 6, k == 0 ? 8.000000001 : 8) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (k == 2)
        {
            if (!(fvm_path_set_point(p, 1, 1, 1, 8) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (k == 3)
        {
            if (!(fvm_path_set_point(p, 1, 0, 15, 8) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        for (unsigned c = 0; c < 3; ++c)
        {
            for (unsigned j = 0; j < 2; ++j)
            {
                if (c || j)
                {
                    FVM_PathBounds before = {1, 2, 3, 4}, b = before;
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
    }
    return 0;
}
