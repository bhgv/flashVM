#include <stdio.h>
#include <stdlib.h>
#include "path_style_bounds_fixture.hpp"


int main()
{
    FVM_Path *p = hit_line(-4, 4, 0, 0);
    if (!(hit_add(p, FVM_PATH_LINE, 4, 4) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double q = std::sqrt(2.0);
    bounds_equal(stroke_bounds(p, 1, 0), -4 - q, -2, 4 + q, 4 + q);
    bounds_equal(stroke_bounds(p, 1, 1), -4 - q, -q, 4 + q, 4 + q);
    fvm_path_destroy(p);
    p = hit_line(0, 0, .1, 0);
    if (!(hit_add(p, FVM_PATH_LINE, .1, .1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned j = 0; j < 2; ++j)
    {
        bounds_equal(stroke_bounds(p, 1, j), -1.9, -2, 2.1, 2);
    }
    fvm_path_destroy(p);
    p = hit_line(0, 0, .1, 0);
    if (!(hit_add(p, FVM_PATH_LINE, .2, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bounds_equal(stroke_bounds(p, 1, 0), 0, -2, .2, 2);
    fvm_path_destroy(p);
    return 0;
}
