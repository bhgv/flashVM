#include "path_style_bounds_fixture.hpp"


int main()
{
    const double directions[][2] = {{3, 4}, {-3, 4}, {3, -4}, {-3, -4}};
    const double widths[] = {.05, 1, 4, 3276.75, 1000000};
    for (unsigned d = 0; d < 4; ++d)
    {
        for (unsigned w = 0; w < 5; ++w)
        {
            for (unsigned cap = 0; cap < 3; ++cap)
            {
                for (unsigned join = 0; join < 2; ++join)
                {
                    double dx = directions[d][0], dy = directions[d][1], radius = widths[w] / 2;
                    double ex = cap == 0 ? radius : radius * (std::fabs(dy) / 5 + (cap == 2 ? std::fabs(dx) / 5 : 0));
                    double ey = cap == 0 ? radius : radius * (std::fabs(dx) / 5 + (cap == 2 ? std::fabs(dy) / 5 : 0));
                    FVM_Path *p = hit_line(0, 0, dx, dy);
                    bounds_equal(stroke_bounds(p, cap, join, widths[w]),
                                 (dx < 0 ? dx : 0) - ex,
                                 (dy < 0 ? dy : 0) - ey,
                                 (dx > 0 ? dx : 0) + ex,
                                 (dy > 0 ? dy : 0) + ey);
                    fvm_path_destroy(p);
                }
            }
        }
    }
    return 0;
}
