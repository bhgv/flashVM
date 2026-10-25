#include "adaptive_curve_fixture.hpp"


int main()
{
    for (unsigned c = 0; c < 3; ++c)
    {
        for (unsigned j = 0; j < 2; ++j)
        {
            FVM_Path *p = dc_path();
            dc_expected(stroke_bounds(p, c, j, 4.0 / 256), c);
            fvm_path_destroy(p);
        }
    }
    return 0;
}
