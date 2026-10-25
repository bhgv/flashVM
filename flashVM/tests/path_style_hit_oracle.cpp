#include <stdio.h>
#include <stdlib.h>
#include "path_style_hit_fixture.hpp"
#include <cstdio>


int main()
{
    const double widths[] = {.05, 1, 4, 100};
    for (unsigned w = 0; w < 4; ++w)
    {
        for (unsigned cap = 0; cap < 3; ++cap)
        {
            for (unsigned join = 0; join < 2; ++join)
            {
                for (unsigned vertical = 0; vertical < 2; ++vertical)
                {
                    FVM_Path *p = vertical ? hit_line(8, 2, 8, 14) : hit_line();
                    double radius = widths[w] / 2;
                    for (int row = -12; row <= 12; ++row)
                    {
                        for (int col = -12; col <= 12; ++col)
                        {
                            double x = 8 + (col + .317) * (6 + radius) / 10, y = 8 + (row + .173) * radius / 10;
                            double along = x < 2 ? 2 - x : x > 14 ? x - 14 : 0, across = std::fabs(y - 8);
                            bool expected =
                                cap == 0 ? along * along + across * across <= radius * radius
                                         : across <= radius &&
                                               (cap == 1 ? x >= 2 && x <= 14 : x >= 2 - radius && x <= 14 + radius);
                            int got = selected_hit(p, vertical ? y : x, vertical ? x : y, cap, join, widths[w]);
                            if (got != expected)
                            {
                                std::fprintf(
                                    stderr,
                                    "w=%u cap=%u join=%u vert=%u x=%.17g y=%.17g radius=%.17g got=%d expected=%d\n",
                                    w,
                                    cap,
                                    join,
                                    vertical,
                                    x,
                                    y,
                                    radius,
                                    got,
                                    expected);
                            }
                            if (!(got == expected))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    fvm_path_destroy(p);
                }
            }
        }
    }
    return 0;
}
