/* FlashVM: backend-independent fill and selected cap/join point hits and straight/round-curve bounds. */
#include "flashvm/path.h"
#include "path_geometry_constants.h"
#include "../render/quality_internal.hpp"
#include "../render/curve_bounds.hpp"
#include <cmath>
#include <cfloat>
#include <climits>
#include <cstring>

namespace
{
/* Ray-crossing and distance state for one query across all path contours. */
struct Query
{
    double x, y;   /* Query coordinates in the same space as path commands. */
    double radius; /* Half the requested stroke width; zero selects filled contours. */
    bool styled; /* Explicit private cap/join policy; public queries retain legacy distances. */
    unsigned cap, join; /* Validated stroke selectors. */
    bool segment; /* Whether this contour has a nonzero tangent. */
    double first_x, first_y, first_dx, first_dy; /* Initial point and unit tangent. */
    double last_x, last_y, last_dx, last_dy; /* Last endpoint and unit tangent. */
    bool tracking; /* Whether this walk also measures exact straight-stroke extents. */
    bool have_extent; /* Whether a nonempty stroke primitive was accumulated. */
    FVM_PathBounds extent; /* Union of primitive endpoints and sector extrema. */
    long winding;  /* Signed half-open ray intersections across all contours. */
    bool edge;     /* Whether a boundary or stroked segment already contains the point. */
    /* Initialize an empty query with no intersections. */
    Query(double px, double py, double width, bool selected = false,
          unsigned selected_cap = FVM_STROKE_CAP_ROUND,
          unsigned selected_join = FVM_STROKE_JOIN_ROUND, bool measure = false)
        : x(px), y(py), radius(width / 2), styled(selected), cap(selected_cap), join(selected_join),
          segment(false), first_x(0), first_y(0), first_dx(0), first_dy(0),
          last_x(0), last_y(0), last_dx(0), last_dy(0), tracking(measure),
          have_extent(false), winding(0), edge(false)
    {
        std::memset(&extent, 0, sizeof(extent));
    }
    /* Include one primitive extremum without allocation or a graphics backend. */
    void point(double px, double py)
    {
        if (!tracking)
            return;
        if (!have_extent)
        {
            extent.x_min = extent.x_max = px;
            extent.y_min = extent.y_max = py;
            have_extent = true;
        }
        else
        {
            if (px < extent.x_min) extent.x_min = px;
            if (py < extent.y_min) extent.y_min = py;
            if (px > extent.x_max) extent.x_max = px;
            if (py > extent.y_max) extent.y_max = py;
        }
    }
    /* Include a circular round cap or join, retaining boundary points. */
    void disk(double ax, double ay)
    {
        double dx = x - ax, dy = y - ay;
        if (radius * radius < DBL_MIN && radius > 0)
        {
            /* Reject distant components before division, then compare bounded ratios. */
            if (std::fabs(dx) > radius || std::fabs(dy) > radius)
                return;
            const double sx = dx / radius, sy = dy / radius;
            if (sx * sx + sy * sy <= 1)
                edge = true;
        }
        else if (dx * dx + dy * dy <= radius * radius)
            edge = true;
    }
    /* Include a bevel wedge using orientation signs, independent of raster backends. */
    void triangle(double ax, double ay, double bx, double by, double cx, double cy)
    {
        double a = (bx - ax) * (y - ay) - (by - ay) * (x - ax);
        double b = (cx - bx) * (y - by) - (cy - by) * (x - bx);
        double c = (ax - cx) * (y - cy) - (ay - cy) * (x - cx);
        double area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
        if (area && ((a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0)))
            edge = true;
    }
    /* Complete a turn between normalized tangents; inner wedges are already in the bodies. */
    void turn(double ax, double ay, double ux, double uy, double vx, double vy)
    {
        double cross = ux * vy - uy * vx;
        if (!cross && ux * vx + uy * vy > 0)
            return;
        if (join == FVM_STROKE_JOIN_ROUND)
        {
            if (tracking)
            {
                /* Sector endpoints already belong to the segment bodies. Only cardinal
                 * directions can add another axis-aligned extremum. */
                const double dx[] = {1, 0, -1, 0}, dy[] = {0, 1, 0, -1};
                for (unsigned i = 0; i < 4; ++i)
                    if (dx[i] * ux + dy[i] * uy >= 0 && dx[i] * vx + dy[i] * vy <= 0)
                        point(ax + radius * dx[i], ay + radius * dy[i]);
            }
            double qx = x - ax, qy = y - ay;
            /* Restrict to the outer sector: a whole disk would extend short butt-ended edges. */
            if (qx * ux + qy * uy >= 0 && qx * vx + qy * vy <= 0)
                disk(ax, ay);
        }
        else
        {
            double side = cross > 0 ? -1 : 1;
            triangle(ax, ay, ax - side * uy * radius, ay + side * ux * radius,
                     ax - side * vy * radius, ay + side * vx * radius);
        }
    }
    /* Include only the outward half-width square extension at an open endpoint. */
    void endpoint(double ax, double ay, double ux, double uy)
    {
        if (cap == FVM_STROKE_CAP_ROUND)
        {
            point(ax - radius, ay - radius);
            point(ax + radius, ay + radius);
            disk(ax, ay);
        }
        else if (cap == FVM_STROKE_CAP_SQUARE)
        {
            point(ax + radius * (ux - uy), ay + radius * (uy + ux));
            point(ax + radius * (ux + uy), ay + radius * (uy - ux));
            double along = (x - ax) * ux + (y - ay) * uy;
            double across = (x - ax) * uy - (y - ay) * ux;
            if (along >= 0 && along <= radius && std::fabs(across) <= radius)
                edge = true;
        }
    }
    /* Finish one explicit contour; closed contours join the final and initial tangents. */
    void finish(bool closed)
    {
        if (styled && radius && segment)
        {
            if (closed)
                turn(first_x, first_y, last_dx, last_dy, first_dx, first_dy);
            else
            {
                endpoint(first_x, first_y, -first_dx, -first_dy);
                endpoint(last_x, last_y, last_dx, last_dy);
            }
        }
        segment = false;
    }
    /* Accumulate a fill crossing, a selected stroke body, or legacy round distance. */
    void line(double ax, double ay, double bx, double by)
    {
        double dx = bx - ax, dy = by - ay, length = dx * dx + dy * dy;
        if (styled && radius)
        {
            if (!dx && !dy)
                return;
            double size = std::sqrt(length), ux, uy;
            if (length >= DBL_MIN)
            {
                ux = dx / size;
                uy = dy / size;
            }
            else
            {
                /* Subnormal squared norms have reduced relative precision, even when nonzero.
                 * Scale before squaring throughout that range; preserve normal norms. */
                const double span = std::fabs(dx) > std::fabs(dy) ? std::fabs(dx) : std::fabs(dy);
                const double sx = dx / span, sy = dy / span;
                const double norm = std::sqrt(sx * sx + sy * sy);
                ux = sx / norm;
                uy = sy / norm;
                size = span * norm;
            }
            point(ax - uy * radius, ay + ux * radius);
            point(ax + uy * radius, ay - ux * radius);
            point(bx - uy * radius, by + ux * radius);
            point(bx + uy * radius, by - ux * radius);
            double along = (x - ax) * ux + (y - ay) * uy;
            double across = (x - ax) * uy - (y - ay) * ux;
            if (along >= 0 && along <= size && std::fabs(across) <= radius)
                edge = true;
            if (segment)
                turn(ax, ay, last_dx, last_dy, ux, uy);
            else
            {
                first_x = ax; first_y = ay; first_dx = ux; first_dy = uy;
                segment = true;
            }
            last_x = bx; last_y = by; last_dx = ux; last_dy = uy;
            return;
        }
        if (!length && !radius)
            return;
        double t = length ? ((x - ax) * dx + (y - ay) * dy) / length : 0;
        if (t < 0)
            t = 0;
        if (t > 1)
            t = 1;
        double ex = x - ax - t * dx, ey = y - ay - t * dy;
        double limit = radius ? radius : 0.000001;
        if (ex * ex + ey * ey <= limit * limit)
            edge = true;
        if (!radius && ((ay <= y && by > y) || (by <= y && ay > y)))
        {
            double at = ax + (y - ay) * dx / dy;
            if (at > x)
                winding += by > ay ? 1 : -1;
        }
    }
};
/* Measure control-point distance from the finite endpoint segment, including cusps. */
double distance(double x, double y, double ax, double ay, double bx, double by)
{
    double dx = bx - ax, dy = by - ay, length = dx * dx + dy * dy;
    double t = length ? ((x - ax) * dx + (y - ay) * dy) / length : 0;
    if (t < 0)
        t = 0;
    if (t > 1)
        t = 1;
    dx = x - ax - t * dx;
    dy = y - ay - t * dy;
    return dx * dx + dy * dy;
}
/* Subdivide quadratic/cubic Beziers until their chord error is bounded or depth reaches 20. */
void curve(Query &query, const double *x, const double *y, unsigned degree, unsigned depth)
{
    bool flat = true;
    for (unsigned i = 1; i < degree; ++i)
        if (distance(x[i], y[i], x[0], y[0], x[degree], y[degree]) > 0.00000001)
            flat = false;
    if (flat || depth == 20)
    {
        query.line(x[0], y[0], x[degree], y[degree]);
        return;
    }
    double tx[4], ty[4], lx[4], ly[4], rx[4], ry[4];
    for (unsigned i = 0; i <= degree; ++i)
    {
        tx[i] = x[i];
        ty[i] = y[i];
    }
    lx[0] = tx[0];
    ly[0] = ty[0];
    rx[degree] = tx[degree];
    ry[degree] = ty[degree];
    for (unsigned level = 1; level <= degree; ++level)
    {
        for (unsigned i = 0; i <= degree - level; ++i)
        {
            tx[i] = (tx[i] + tx[i + 1]) / 2;
            ty[i] = (ty[i] + ty[i + 1]) / 2;
        }
        lx[level] = tx[0];
        ly[level] = ty[0];
        rx[degree - level] = tx[degree - level];
        ry[degree - level] = ty[degree - level];
    }
    curve(query, lx, ly, degree, depth + 1);
    curve(query, rx, ry, degree, depth + 1);
}
} // namespace
/* Two-Diff residual: force each intermediate to double storage, including on
 * extended-register targets. Reject subtraction that loses any low component.
 * Construction: Shewchuk, Adaptive Precision Floating-Point Arithmetic (1997). */
static bool linear_difference(double point, double origin, double &difference)
{
    volatile double high = point - origin;
    volatile double virtual_origin = point - high;
    volatile double virtual_point = high + virtual_origin;
    volatile double origin_tail = virtual_origin - origin;
    volatile double point_tail = point - virtual_point;
    volatile double residual = point_tail + origin_tail;
    difference = high;
    return residual == 0;
}

/* Power-of-two scaling is exact here and only increases magnitude. The endpoint
 * span bounds every control vector, so no scaled coordinate can overflow. */
static bool linear_vector_coordinate(double difference, int shift, double &scaled)
{
    scaled = std::ldexp(difference, shift);
    return scaled == std::floor(scaled);
}

/* Controls inside a collinear endpoint interval describe exactly its segment for
 * quadratic and cubic Beziers, even when cubic controls are not ordered. The
 * scalar cubic derivative is nonnegative at all four control-box vertices:
 * t*t, 2*t*(1-t), (1-2*t)^2 and (1-t)^2, hence throughout their convex hull.
 * Axis paths admit arbitrary finite coordinates. Other paths require exact
 * origin subtraction and integral relative vectors after adaptive binary scaling.
 * Scaled vectors stay below 2^25, products below 2^50 and cross differences below
 * 2^51; at least 53 mantissa bits make the determinant exact. */
static bool linear_curve_segment(double x, double y, const FVM_PathCommand &command,
                                  unsigned count)
{
    const double end_x = command.x[count - 1], end_y = command.y[count - 1];
    const double low_x = x < end_x ? x : end_x, high_x = x > end_x ? x : end_x;
    const double low_y = y < end_y ? y : end_y, high_y = y > end_y ? y : end_y;
    for (unsigned i = 0; i < count; ++i)
        if (!(command.x[i] >= low_x && command.x[i] <= high_x &&
              command.y[i] >= low_y && command.y[i] <= high_y))
            return false;
    if (x == end_x || y == end_y)
        return true;
    if (FLT_RADIX != FVM_PATH_GEOMETRY_LINEAR_RADIX ||
        DBL_MANT_DIG < FVM_PATH_GEOMETRY_LINEAR_MANTISSA_BITS)
        return false;
    double dx, dy;
    if (!linear_difference(end_x, x, dx) || !linear_difference(end_y, y, dy))
        return false;
    const double span = std::fabs(dx) > std::fabs(dy) ? std::fabs(dx) : std::fabs(dy);
    int exponent = 0;
    std::frexp(span, &exponent);
    if (exponent < FVM_PATH_GEOMETRY_LINEAR_VECTOR_BITS - INT_MAX)
        return false;
    const int shift = FVM_PATH_GEOMETRY_LINEAR_VECTOR_BITS - exponent;
    double ex, ey;
    if (!linear_vector_coordinate(dx, shift, ex) || !linear_vector_coordinate(dy, shift, ey))
        return false;
    for (unsigned i = 0; i < count; ++i)
    {
        double px, py, sx, sy;
        if (!linear_difference(command.x[i], x, px) || !linear_difference(command.y[i], y, py) ||
            !linear_vector_coordinate(px, shift, sx) || !linear_vector_coordinate(py, shift, sy) ||
            sx * ey != sy * ex)
            return false;
    }
    return true;
}

/* Test a point against implicitly closed fills or connected strokes with the selected
 * private geometry policy; public callers retain their legacy round-distance policy. */
static FVM_Result hit_test(const FVM_Path *path, double x, double y, double width,
                           int even_odd, int *out, bool styled, unsigned cap, unsigned join,
                           FVM_PathBounds *extent = 0, int *has_segments = 0)
{
    if (!path || !out || !(x >= -2000000 && x <= 2000000 && y >= -2000000 && y <= 2000000) ||
        !(width >= 0 && width <= FVM_PATH_GEOMETRY_MAX_STROKE_WIDTH) || (even_odd != 0 && even_odd != 1) ||
        cap > FVM_STROKE_CAP_SQUARE || join > FVM_STROKE_JOIN_BEVEL)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    Query query(x, y, width, styled, cap, join, extent != 0);
    double first_x = 0, first_y = 0, current_x = 0, current_y = 0;
    bool open = false;
    for (size_t i = 0; i < fvm_path_count(path); ++i)
    {
        FVM_PathCommand command;
        FVM_Result result = fvm_path_get(path, i, &command);
        if (result != FVM_OK)
            return result;
        if (command.verb == FVM_PATH_MOVE)
        {
            if (open && !width)
                query.line(current_x, current_y, first_x, first_y);
            query.finish(false);
            first_x = current_x = command.x[0];
            first_y = current_y = command.y[0];
            open = true;
        }
        else if (command.verb == FVM_PATH_CLOSE)
        {
            if (open)
                query.line(current_x, current_y, first_x, first_y);
            query.finish(true);
            current_x = first_x;
            current_y = first_y;
            open = false;
        }
        else if (command.verb == FVM_PATH_LINE)
        {
            query.line(current_x, current_y, command.x[0], command.y[0]);
            current_x = command.x[0];
            current_y = command.y[0];
        }
        else
        {
            unsigned degree = command.verb == FVM_PATH_QUADRATIC
                                  ? FVM_PATH_GEOMETRY_QUADRATIC_POINTS : FVM_PATH_GEOMETRY_CUBIC_POINTS;
            if (extent)
            {
                /* Reuse finite bodies/caps/joins only when the curve is exactly a segment.
                 * Other curves leave both outputs untouched for the managed fallback. */
                if (!linear_curve_segment(current_x, current_y, command, degree))
                    return FVM_UNSUPPORTED;
                query.line(current_x, current_y, command.x[degree - 1], command.y[degree - 1]);
                current_x = command.x[degree - 1];
                current_y = command.y[degree - 1];
                continue;
            }
            double cx[4], cy[4];
            cx[0] = current_x;
            cy[0] = current_y;
            for (unsigned k = 0; k < degree; ++k)
            {
                cx[k + 1] = command.x[k];
                cy[k + 1] = command.y[k];
            }
            curve(query, cx, cy, degree, 0);
            current_x = cx[degree];
            current_y = cy[degree];
        }
    }
    if (open && !width)
        query.line(current_x, current_y, first_x, first_y);
    query.finish(false);
    *out = query.edge || (!width && (even_odd ? query.winding % 2 != 0 : query.winding != 0));
    if (extent)
    {
        *extent = query.extent;
        *has_segments = query.have_extent ? 1 : 0;
    }
    return FVM_OK;
}

/* Preserve the public round-ended distance query and its existing C89 ABI. */
extern "C" FVM_Result fvm_path_hit_test(const FVM_Path *path, double x, double y, double width,
                                        int even_odd, int *out)
{
    return hit_test(path, x, y, width, even_odd, out, false,
                    FVM_STROKE_CAP_ROUND, FVM_STROKE_JOIN_ROUND);
}
/* Query selected private strokes independently of AGG/Cairo or a rendering surface. */
FVM_Result fvm_path_hit_test_style(const FVM_Path *path, double x, double y, double width,
                                   int even_odd, int *out, unsigned cap, unsigned join)
{
    return hit_test(path, x, y, width, even_odd, out, true, cap, join);
}

/* Measure the centerline of nonconstant round-stroked segments analytically.
 * Round caps and joins form the radius-neighborhood of this centerline, so its
 * axis extrema expand by exactly half the width. Isolated Moves are excluded.
 * All results stay local until the complete borrowed path has been validated. */
static FVM_Result round_stroke_bounds(const FVM_Path *path, double width,
                                      FVM_PathBounds *out, int *has_segments)
{
    FVM_PathBounds box = {0, 0, 0, 0};
    double x = 0, y = 0, sx = 0, sy = 0;
    bool any = false;
    for (size_t i = 0; i < fvm_path_count(path); ++i)
    {
        FVM_PathCommand command;
        FVM_Result result = fvm_path_get(path, i, &command);
        if (result != FVM_OK)
            return result;
        if (command.verb == FVM_PATH_MOVE)
        {
            x = sx = command.x[0];
            y = sy = command.y[0];
            continue;
        }
        unsigned count = command.verb == FVM_PATH_CUBIC ? FVM_PATH_GEOMETRY_CUBIC_POINTS :
                         command.verb == FVM_PATH_QUADRATIC ? FVM_PATH_GEOMETRY_QUADRATIC_POINTS : 1;
        if (command.verb == FVM_PATH_CLOSE)
        {
            command.x[0] = sx;
            command.y[0] = sy;
        }
        bool nonconstant = false;
        for (unsigned n = 0; n < count; ++n)
            if (command.x[n] != x || command.y[n] != y)
                nonconstant = true;
        if (nonconstant)
        {
            if (!any)
            {
                box.x_min = box.x_max = x;
                box.y_min = box.y_max = y;
                any = true;
            }
            else
            {
                fvm_geometry::extent(x, box.x_min, box.x_max);
                fvm_geometry::extent(y, box.y_min, box.y_max);
            }
            if (count == 1)
            {
                fvm_geometry::extent(command.x[0], box.x_min, box.x_max);
                fvm_geometry::extent(command.y[0], box.y_min, box.y_max);
            }
            else
            {
                fvm_geometry::curve_bounds(x, command.x[0], command.x[1], command.x[2],
                                           count == FVM_PATH_GEOMETRY_CUBIC_POINTS, box.x_min, box.x_max);
                fvm_geometry::curve_bounds(y, command.y[0], command.y[1], command.y[2],
                                           count == FVM_PATH_GEOMETRY_CUBIC_POINTS, box.y_min, box.y_max);
            }
        }
        x = command.x[count - 1];
        y = command.y[count - 1];
    }
    if (any)
    {
        double radius = width / 2;
        box.x_min -= radius;
        box.y_min -= radius;
        box.x_max += radius;
        box.y_max += radius;
    }
    *out = box;
    *has_segments = any ? 1 : 0;
    return FVM_OK;
}

/* Measure straight selected strokes or analytic round/round Bezier strokes. */
FVM_Result fvm_path_stroke_bounds_style(const FVM_Path *path, double width, unsigned cap,
                                        unsigned join, FVM_PathBounds *out, int *has_segments)
{
    if (!path || !out || !has_segments || !(width > 0 && width <= FVM_PATH_GEOMETRY_MAX_STROKE_WIDTH) ||
        cap > FVM_STROKE_CAP_SQUARE || join > FVM_STROKE_JOIN_BEVEL)
        return FVM_INVALID_ARGUMENT;
    if (cap == FVM_STROKE_CAP_ROUND && join == FVM_STROKE_JOIN_ROUND)
        return round_stroke_bounds(path, width, out, has_segments);
    int ignored = 0;
    return hit_test(path, 0, 0, width, 0, &ignored, true, cap, join, out, has_segments);
}
