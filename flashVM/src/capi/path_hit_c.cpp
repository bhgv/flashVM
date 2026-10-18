/* FlashVM: backend-independent filled-contour and round-stroke point hit testing. */
#include "flashvm/path.h"
#include <cmath>
#include <cstring>

namespace
{
/* Ray-crossing and distance state for one query across all path contours. */
struct Query
{
    double x, y;   /* Query coordinates in the same space as path commands. */
    double radius; /* Half the requested stroke width; zero selects filled contours. */
    long winding;  /* Signed half-open ray intersections across all contours. */
    bool edge;     /* Whether a boundary or stroked segment already contains the point. */
    /* Initialize an empty query with no intersections. */
    Query(double px, double py, double width)
        : x(px), y(py), radius(width / 2), winding(0), edge(false)
    {
    }
    /* Accumulate a line crossing, or test its round-ended stroke distance. */
    void line(double ax, double ay, double bx, double by)
    {
        double dx = bx - ax, dy = by - ay, length = dx * dx + dy * dy;
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
/* Test a point against implicitly closed fills or explicitly connected round-cap/round-join
 * strokes. */
extern "C" FVM_Result fvm_path_hit_test(const FVM_Path *path, double x, double y, double width,
                                        int even_odd, int *out)
{
    if (!path || !out || !(x >= -2000000 && x <= 2000000 && y >= -2000000 && y <= 2000000) ||
        !(width >= 0 && width <= 1000000) || (even_odd != 0 && even_odd != 1))
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    Query query(x, y, width);
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
            first_x = current_x = command.x[0];
            first_y = current_y = command.y[0];
            open = true;
        }
        else if (command.verb == FVM_PATH_CLOSE)
        {
            if (open)
                query.line(current_x, current_y, first_x, first_y);
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
            unsigned degree = command.verb == FVM_PATH_QUADRATIC ? 2 : 3;
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
    *out = query.edge || (!width && (even_odd ? query.winding % 2 != 0 : query.winding != 0));
    return FVM_OK;
}
