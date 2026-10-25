#include <stdio.h>
#include <stdlib.h>
#include "round_curve_fixture.hpp"


int main()
{
    FVM_Path *p = 0;
    if (!(fvm_path_create(&p) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(hit_add(p, FVM_PATH_MOVE, 50, 60) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(hit_add(p, FVM_PATH_QUADRATIC, 50, 60, 50, 60) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(hit_add(p, FVM_PATH_CUBIC, 50, 60, 50, 60, 50, 60) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bounds_equal(stroke_bounds(p, 0, 0, 4, 0), 0, 0, 0, 0);
    fvm_path_destroy(p);
    return 0;
}
