/* FlashVM: shared analytic Bezier axis extrema; no allocation or retained state. */
#ifndef FVM_GEOMETRY_CURVE_BOUNDS_HPP
#define FVM_GEOMETRY_CURVE_BOUNDS_HPP
#include <cmath>
#include <cfloat>
namespace fvm_geometry
{
/* Extend an existing axis interval with one finite coordinate. */
inline void extent(double v, double &low, double &high)
{
    if (v < low)
        low = v;
    if (v > high)
        high = v;
}

/* Evaluate one quadratic or cubic Bezier axis in Bernstein form. */
inline double bezier(double p0, double p1, double p2, double p3, double t, bool cubic)
{
    double s = 1 - t;
    return cubic ? s * s * s * p0 + 3 * s * s * t * p1 + 3 * s * t * t * p2 + t * t * t * p3
                 : s * s * p0 + 2 * s * t * p1 + t * t * p2;
}

/* Include the endpoint and interior derivative roots in an existing axis interval.
 * The caller includes the start point; coefficient formulas are shared with public bounds. */
inline void curve_bounds(double p0, double p1, double p2, double p3, bool cubic, double &low,
                         double &high)
{
    extent(cubic ? p3 : p2, low, high);

    /* Initialization / Инициализация локального состояния. */
    double a = cubic ? -p0 + 3 * p1 - 3 * p2 + p3 : 0;
    double b = cubic ? 2 * (p0 - 2 * p1 + p2) : p0 - 2 * p1 + p2;
    double c = p1 - p0;
    double roots[2];
    unsigned int n = 0;
    if (a == 0)
    {
        if (b != 0)
            roots[n++] = -c / b;
    }
    else
    {
        /* Uniformly tiny coefficients lose their discriminant when squared. Scale
         * by an exact binary exponent before products; roots and their signs stay
         * unchanged, and ordinary coefficient arithmetic retains its old path. */
        double span = std::fabs(a);
        if (std::fabs(b) > span)
            span = std::fabs(b);
        if (std::fabs(c) > span)
            span = std::fabs(c);
        if (span * span < DBL_MIN)
        {
            int exponent = 0;
            std::frexp(span, &exponent);
            a = std::ldexp(a, -exponent);
            b = std::ldexp(b, -exponent);
            c = std::ldexp(c, -exponent);
        }
        double d = b * b - 4 * a * c;
        if (d >= 0)
        {
            double q = -0.5 * (b + (b < 0 ? -std::sqrt(d) : std::sqrt(d)));
            if (q == 0)
                roots[n++] = -b / (2 * a);
            else
            {
                roots[n++] = q / a;
                roots[n++] = c / q;
            }
        }
    }
    for (unsigned int i = 0; i < n; ++i)
        if (roots[i] > 0 && roots[i] < 1)
            extent(bezier(p0, p1, p2, p3, roots[i], cubic), low, high);
}

} /* namespace fvm_geometry */
#endif
