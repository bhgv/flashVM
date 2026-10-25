#include <stdio.h>
#include <stdlib.h>
#include "linear_curve_fixture.hpp"


int main()
{
    for (int e = -540; e >= -1000; e -= 20)
    {
        for (unsigned c = 0; c < 3; ++c)
        {
            for (unsigned j = 0; j < 2; ++j)
            {
                double scale = std::ldexp(1.0, e);
                FVM_Path *p = hit_line(0, 0, -3 * scale, -4 * scale);
                FVM_PathBounds b = stroke_bounds(p, c, j, 4 * scale);
                b.x_min /= scale;
                b.y_min /= scale;
                b.x_max /= scale;
                b.y_max /= scale;
                lc_expected(b, c, 4, -3, -4, 0, 0);
                if (!(selected_hit(p, -3 * scale / 2, -4 * scale / 2, c, j, 4 * scale)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_path_destroy(p);
            }
        }
    }
    return 0;
}
