#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_SUBNORMAL_STROKE_FIXTURE_HPP
#define FVM_SUBNORMAL_STROKE_FIXTURE_HPP
#include "adaptive_curve_fixture.hpp"


static FVM_PathBounds sn_unit(FVM_PathBounds b, double s)
{
    b.x_min /= s;
    b.y_min /= s;
    b.x_max /= s;
    b.y_max /= s;
    return b;
}


static void sn_equal(const FVM_PathBounds &a, const FVM_PathBounds &b)
{
    bounds_equal(a, b.x_min, b.y_min, b.x_max, b.y_max);
}


static FVM_Path *sn_contour(unsigned kind, double s)
{
    FVM_Path *p = hit_line(0, 0, 3 * s, 4 * s);
    if (kind == 0)
    {
        if (!(hit_add(p, FVM_PATH_LINE, 7 * s, s) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (kind == 1)
    {
        if (!(hit_add(p, FVM_PATH_LINE, 7 * s, s) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(hit_add(p, FVM_PATH_CLOSE) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (kind == 2)
    {
        if (!(hit_add(p, FVM_PATH_MOVE, 9 * s, -2 * s) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(hit_add(p, FVM_PATH_LINE, 12 * s, 2 * s) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (kind == 3)
    {
        if (!(hit_add(p, FVM_PATH_LINE, 3 * s, 4 * s) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(hit_add(p, FVM_PATH_LINE, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return p;
}
#endif
