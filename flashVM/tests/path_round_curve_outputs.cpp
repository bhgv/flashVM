#include <stdio.h>
#include <stdlib.h>
#include "round_curve_fixture.hpp"


int main()
{
    FVM_Path *p = rc_path();
    FVM_PathBounds before = {1, 2, 3, 4}, b = before;
    int any = 77;
    if (!(fvm_path_stroke_bounds_style(p, 4, 1, 0, &b, &any) == FVM_UNSUPPORTED && any == 77 &&
          std::memcmp(&b, &before, sizeof(b)) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_stroke_bounds_style(p, 4, 0, 1, &b, &any) == FVM_UNSUPPORTED && any == 77 &&
          std::memcmp(&b, &before, sizeof(b)) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_stroke_bounds_style(p, HUGE_VAL, 0, 0, &b, &any) == FVM_INVALID_ARGUMENT && any == 77 &&
          std::memcmp(&b, &before, sizeof(b)) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_stroke_bounds_style(p, 4, 0, 0, &b, &any) == FVM_OK && any == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bounds_equal(b, 0, 2, 16, 10);
    fvm_path_destroy(p);
    return 0;
}
