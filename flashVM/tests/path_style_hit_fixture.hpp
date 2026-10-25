#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_PATH_STYLE_HIT_FIXTURE_HPP
#define FVM_PATH_STYLE_HIT_FIXTURE_HPP
#include "../src/render/quality_internal.hpp"
#include <cmath>
#include <cstring>


static FVM_Result hit_add(FVM_Path *p,
                          unsigned verb,
                          double x0 = 0,
                          double y0 = 0,
                          double x1 = 0,
                          double y1 = 0,
                          double x2 = 0,
                          double y2 = 0)
{
    FVM_PathCommand q;
    std::memset(&q, 0, sizeof(q));
    q.verb = (FVM_PathVerb)verb;
    q.x[0] = x0;
    q.y[0] = y0;
    q.x[1] = x1;
    q.y[1] = y1;
    q.x[2] = x2;
    q.y[2] = y2;
    return fvm_path_add(p, &q);
}


static FVM_Path *hit_line(double ax = 2, double ay = 8, double bx = 14, double by = 8)
{
    FVM_Path *p = 0;
    if (!(fvm_path_create(&p) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(hit_add(p, FVM_PATH_MOVE, ax, ay) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(hit_add(p, FVM_PATH_LINE, bx, by) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return p;
}


static int selected_hit(FVM_Path *p, double x, double y, unsigned cap, unsigned join = 0, double width = 4)
{
    int hit = -1;
    if (!(fvm_path_hit_test_style(p, x, y, width, 0, &hit, cap, join) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return hit;
}
#endif
