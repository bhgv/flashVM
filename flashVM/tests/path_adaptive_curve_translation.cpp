#include "adaptive_curve_fixture.hpp"


int main()
{
    const double origins[] = {.00390625, 1000, 999000};
    for (unsigned o = 0; o < 3; ++o)
    {
        for (unsigned c = 0; c < 3; ++c)
        {
            for (unsigned j = 0; j < 2; ++j)
            {
                FVM_Path *p = dc_path(3, false, -8, origins[o]);
                dc_expected(stroke_bounds(p, c, j, 4.0 / 256), c, 4.0 / 256, -8, origins[o]);
                fvm_path_destroy(p);
            }
        }
    }
    return 0;
}
