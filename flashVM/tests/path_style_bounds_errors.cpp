#include <stdio.h>
#include <stdlib.h>
#include "path_style_bounds_fixture.hpp"


int main()
{
    FVM_Path *p = hit_line();
    FVM_PathBounds before = {1, 2, 3, 4}, out = before;
    int any = 77;
    const double widths[] = {0, -1, 1000001, HUGE_VAL, HUGE_VAL - HUGE_VAL};
    for (unsigned i = 0; i < 5; ++i)
    {
        if (!(fvm_path_stroke_bounds_style(p, widths[i], 0, 0, &out, &any) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(std::memcmp(&out, &before, sizeof(out)) == 0 && any == 77))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    const unsigned styles[][2] = {{3, 0}, {0, 2}, {~0U, 0}, {0, ~0U}};
    for (unsigned i = 0; i < 4; ++i)
    {
        if (!(fvm_path_stroke_bounds_style(p, 4, styles[i][0], styles[i][1], &out, &any) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(std::memcmp(&out, &before, sizeof(out)) == 0 && any == 77))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_path_stroke_bounds_style(0, 4, 0, 0, &out, &any) == FVM_INVALID_ARGUMENT && any == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_stroke_bounds_style(p, 4, 0, 0, 0, &any) == FVM_INVALID_ARGUMENT && any == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_stroke_bounds_style(p, 4, 0, 0, &out, 0) == FVM_INVALID_ARGUMENT && any == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(hit_add(p, FVM_PATH_QUADRATIC, 8, 16, 14, 8) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_stroke_bounds_style(p, 4, 1, 0, &out, &any) == FVM_UNSUPPORTED && any == 77 &&
          std::memcmp(&out, &before, sizeof(out)) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(p);
    p = hit_line();
    if (!(hit_add(p, FVM_PATH_CUBIC, 2, 16, 14, 16, 14, 8) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_stroke_bounds_style(p, 4, 1, 0, &out, &any) == FVM_UNSUPPORTED && any == 77 &&
          std::memcmp(&out, &before, sizeof(out)) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(p);
    return 0;
}
