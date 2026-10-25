#include <stdio.h>
#include <stdlib.h>
#include "subnormal_stroke_fixture.hpp"


int main()
{
    for (int e = -515; e >= -1000; e -= 5)
    {
        for (unsigned cap = 0; cap < 3; ++cap)
        {
            double s = std::ldexp(1.0, e);
            FVM_Path *p = sn_contour(0, s), *unit = sn_contour(0, 1);
            for (int ix = -12; ix <= 28; ++ix)
            {
                for (int iy = -12; iy <= 28; ++iy)
                {
                    double x = ix * .271 + .037, y = iy * .271 + .093;
                    if (!(selected_hit(p, x * s, y * s, cap, 1, 4 * s) == selected_hit(unit, x, y, cap, 1, 4)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            fvm_path_destroy(p);
            fvm_path_destroy(unit);
        }
    }
    return 0;
}
