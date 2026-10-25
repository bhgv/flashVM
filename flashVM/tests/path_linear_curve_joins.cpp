#include <stdio.h>
#include <stdlib.h>
#include "linear_curve_fixture.hpp"


int main()
{
    for (unsigned c = 0; c < 3; ++c)
    {
        for (unsigned j = 0; j < 2; ++j)
        {
            FVM_Path *p = lc_path(3), *line = 0;
            if (!(fvm_path_create(&line) == FVM_OK && hit_add(line, FVM_PATH_MOVE, 2, 4) == FVM_OK &&
                  hit_add(line, FVM_PATH_LINE, 14, 10) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(hit_add(p, FVM_PATH_QUADRATIC, 11, 13, 8, 16) == FVM_OK &&
                  hit_add(line, FVM_PATH_LINE, 8, 16) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(hit_add(p, FVM_PATH_CLOSE) == FVM_OK && hit_add(line, FVM_PATH_CLOSE) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_PathBounds a = stroke_bounds(p, c, j), b = stroke_bounds(line, c, j);
            bounds_equal(a, b.x_min, b.y_min, b.x_max, b.y_max);
            fvm_path_destroy(p);
            fvm_path_destroy(line);
        }
    }
    return 0;
}
