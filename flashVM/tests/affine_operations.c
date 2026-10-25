/* C89 raw affine algebra retains IEEE propagation, aliasing and host operation order. */
#include <stdio.h>
#include "flashvm/affine.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>


static void near(double a, double b)
{
    if (!(fabs(a - b) < 1e-12))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void unchanged(const FVM_Affine *m)
{
    if (!(m->xx == 71 && m->yx == 72 && m->xy == 73 && m->yy == 74 && m->x0 == 75 && m->y0 == 76))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    FVM_Affine m = {2, 1, 0.5, 3, 5, 7}, inverse, canary = {71, 72, 73, 74, 75, 76};
    double x = 73, y = 74, nan = strtod("NaN", 0);
    if (!(fvm_affine_transform_point(&m, 2, -3, &x, &y) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    near(x, 7.5);
    near(y, 0);
    if (!(fvm_affine_invert(&m, &inverse) == FVM_OK && fvm_affine_transform_point(&inverse, x, y, &x, &y) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    near(x, 2);
    near(y, -3);
    if (!(fvm_affine_invert(&m, &m) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    near(m.xx, inverse.xx);
    near(m.yx, inverse.yx);
    near(m.xy, inverse.xy);
    near(m.yy, inverse.yy);
    near(m.x0, inverse.x0);
    near(m.y0, inverse.y0);
    if (!(fvm_affine_transform_point(&m, 7.5, 0, &m.xx, &m.yy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    near(m.xx, 2);
    near(m.yy, -3);
    x = 73;
    y = 74;
    if (!(fvm_affine_transform_point(0, 1, 2, &x, &y) == FVM_INVALID_ARGUMENT && x == 73 && y == 74))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_affine_transform_point(&m, 1, 2, &x, &x) == FVM_INVALID_ARGUMENT && x == 73))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_affine_transform_point(&m, 1, 2, &x, 0) == FVM_INVALID_ARGUMENT && x == 73))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_affine_invert(0, &canary) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unchanged(&canary);
    if (!(fvm_affine_invert(&m, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    m.xx = 1;
    m.yx = 2;
    m.xy = 2;
    m.yy = 4;
    m.x0 = 5;
    m.y0 = 7;
    if (!(fvm_affine_invert(&m, &canary) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unchanged(&canary);
    m.xx = nan;
    if (!(fvm_affine_invert(&m, &canary) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unchanged(&canary);
    m.xx = DBL_MAX;
    m.yx = 0;
    m.xy = 0;
    m.yy = 2;
    m.x0 = 0;
    m.y0 = 0;
    if (!(fvm_affine_transform_point(&m, 2, 1, &x, &y) == FVM_OK && x > DBL_MAX && y == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_affine_invert(&m, &inverse) == FVM_OK && inverse.xx == 0 && inverse.yy == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    m.xx = nan;
    if (!(fvm_affine_transform_point(&m, 1, 2, &x, &y) == FVM_OK && x != x && y == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    m.xx = 3;
    m.yy = 1;
    m.x0 = 1e16;
    if (!(fvm_affine_invert(&m, &inverse) == FVM_OK &&
          fvm_affine_transform_point(&inverse, 1e16 + 2, 0, &x, &y) == FVM_OK && x == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_affine_inverse_point(&m, FVM_AFFINE_POINT, 1e16 + 2, 0, &x, &y) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    near(x, 2.0 / 3.0);
    return 0;
}
