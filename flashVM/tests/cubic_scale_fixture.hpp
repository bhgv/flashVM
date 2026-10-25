#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_CUBIC_SCALE_FIXTURE_HPP
#define FVM_CUBIC_SCALE_FIXTURE_HPP
#include "path_style_bounds_fixture.hpp"


static void cb_values(unsigned kind, double *p)
{
    const double table[][4] = {
        {0, 12, -12, 0}, {0, 0, 12, 0}, {-3, 10, -8, 5}, {0, 4, 0, 4}, {0, 4, 8, 12}, {0, 4, 0, 4.0625}, {0, 12, 0, 0}};
    for (unsigned i = 0; i < 4; ++i)
    {
        p[i] = table[kind][i];
    }
}


static FVM_Path *cb_path(unsigned kind, int exponent, unsigned axis = 0, bool reflect = false)
{
    double p[4], x[4], y[4], s = std::ldexp(reflect ? -1.0 : 1.0, exponent);
    cb_values(kind, p);
    for (unsigned i = 0; i < 4; ++i)
    {
        x[i] = axis ? p[i] * s : 0;
        y[i] = axis == 1 ? 0 : p[i] * s * (axis == 2 ? 2 : 1);
    }
    FVM_Path *path = 0;
    if (!(fvm_path_create(&path) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(hit_add(path, FVM_PATH_MOVE, x[0], y[0]) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (kind == 6)
    {
        if (!(hit_add(path, FVM_PATH_QUADRATIC, x[1], y[1], x[2], y[2]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(hit_add(path, FVM_PATH_CUBIC, x[1], y[1], x[2], y[2], x[3], y[3]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return path;
}


static long double cb_evaluate(const double *p, long double t, bool cubic)
{
    long double a[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        a[i] = p[i];
    }
    unsigned count = cubic ? 4 : 3;
    while (count > 1)
    {
        for (unsigned i = 0; i + 1 < count; ++i)
        {
            a[i] = a[i] * (1 - t) + a[i + 1] * t;
        }
        --count;
    }
    return a[0];
}


static void cb_axis_oracle(const double *p, bool cubic, double &low, double &high)
{
    low = p[0] < p[cubic ? 3 : 2] ? p[0] : p[cubic ? 3 : 2];
    high = p[0] > p[cubic ? 3 : 2] ? p[0] : p[cubic ? 3 : 2];
    long double a = cubic ? -(long double)p[0] + 3 * (long double)p[1] - 3 * (long double)p[2] + p[3] : 0;
    long double b = cubic ? 2 * ((long double)p[0] - 2 * (long double)p[1] + p[2])
                          : (long double)p[0] - 2 * (long double)p[1] + p[2];
    long double c = (long double)p[1] - p[0], roots[2];
    unsigned n = 0;
    if (a == 0)
    {
        if (b != 0)
        {
            roots[n++] = -c / b;
        }
    }
    else
    {
        long double d = b * b - 4 * a * c;
        if (d >= 0)
        {
            roots[n++] = (-b + std::sqrt(d)) / (2 * a);
            roots[n++] = (-b - std::sqrt(d)) / (2 * a);
        }
    }
    for (unsigned i = 0; i < n; ++i)
    {
        if (roots[i] > 0 && roots[i] < 1)
        {
            double v = (double)cb_evaluate(p, roots[i], cubic);
            if (v < low)
            {
                low = v;
            }
            if (v > high)
            {
                high = v;
            }
        }
    }
}


static FVM_PathBounds cb_expected(unsigned kind, unsigned axis = 0, bool reflect = false)
{
    double p[4], x[4], y[4];
    cb_values(kind, p);
    for (unsigned i = 0; i < 4; ++i)
    {
        double v = reflect ? -p[i] : p[i];
        x[i] = axis ? v : 0;
        y[i] = axis == 1 ? 0 : v * (axis == 2 ? 2 : 1);
    }
    FVM_PathBounds b;
    cb_axis_oracle(x, kind != 6, b.x_min, b.x_max);
    cb_axis_oracle(y, kind != 6, b.y_min, b.y_max);
    return b;
}


static FVM_PathBounds cb_normalized(FVM_Path *path, int exponent, bool stroke = false)
{
    FVM_PathBounds b;
    double s = std::ldexp(1.0, exponent);
    if (stroke)
    {
        b = stroke_bounds(path, 0, 0, 4 * s);
    }
    else
    {
        if (!(fvm_path_bounds(path, &b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    b.x_min /= s;
    b.y_min /= s;
    b.x_max /= s;
    b.y_max /= s;
    return b;
}


static void cb_check(unsigned kind, int exponent, unsigned axis = 0, bool reflect = false, bool stroke = false)
{
    FVM_Path *p = cb_path(kind, exponent, axis, reflect);
    FVM_PathBounds b = cb_normalized(p, exponent, stroke), expected = cb_expected(kind, axis, reflect);
    double radius = stroke ? 2 : 0;
    bounds_equal(b, expected.x_min - radius, expected.y_min - radius, expected.x_max + radius, expected.y_max + radius);
    fvm_path_destroy(p);
}
#endif
