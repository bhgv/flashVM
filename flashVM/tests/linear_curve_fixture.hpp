#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_LINEAR_CURVE_FIXTURE_HPP
#define FVM_LINEAR_CURVE_FIXTURE_HPP
#include "axis_curve_fixture.hpp"


static FVM_Path *lc_path(unsigned degree = 2, bool reverse = false)
{
    FVM_Path *p = 0;
    if (!(fvm_path_create(&p) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(hit_add(p, FVM_PATH_MOVE, reverse ? 14 : 2, reverse ? 10 : 4) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (degree == 2)
    {
        if (!(hit_add(p, FVM_PATH_QUADRATIC, 8, 7, reverse ? 2 : 14, reverse ? 4 : 10) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(hit_add(p,
                      FVM_PATH_CUBIC,
                      reverse ? 5 : 11,
                      reverse ? 5.5 : 8.5,
                      reverse ? 11 : 5,
                      reverse ? 8.5 : 5.5,
                      reverse ? 2 : 14,
                      reverse ? 4 : 10) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return p;
}


static void lc_expected(const FVM_PathBounds &b,
                        unsigned cap,
                        double width = 4,
                        double x0 = 2,
                        double y0 = 4,
                        double x1 = 14,
                        double y1 = 10)
{
    double dx = std::fabs(x1 - x0), dy = std::fabs(y1 - y0), length = std::sqrt(dx * dx + dy * dy), r = width / 2;
    double px = cap == 0   ? r
                : cap == 1 ? r * dy / length
                           : r * (dx + dy) / length,
           py = cap == 0   ? r
                : cap == 1 ? r * dx / length
                           : r * (dx + dy) / length;
    bounds_equal(
        b, (x0 < x1 ? x0 : x1) - px, (y0 < y1 ? y0 : y1) - py, (x0 > x1 ? x0 : x1) + px, (y0 > y1 ? y0 : y1) + py);
}


static FVM_Object *lc_clip(FVM_Context *c, unsigned cap, unsigned join, double width = 4, unsigned kind = 0)
{
    FVM_Object *clip = ac_clip(c, cap, join, width, kind), *cmds = rc_part(clip)->properties["commands"].object;
    rc_number(cmds->indexed[0].object, "y0", 4);
    FVM_Object *cmd = cmds->indexed[1].object;
    rc_number(cmd, "x0", 8);
    rc_number(cmd, "y0", 7);
    rc_number(cmd, "x1", 14);
    rc_number(cmd, "y1", 10);
    return clip;
}
#endif
