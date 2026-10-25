#include "axis_curve_fixture.hpp"


int main()
{
    const double widths[] = {.25, 1, 4, 255, 1000000};
    for (unsigned w = 0; w < 5; ++w)
    {
        for (unsigned c = 0; c < 3; ++c)
        {
            for (unsigned j = 0; j < 2; ++j)
            {
                FVM_Path *p = ac_path();
                ac_expected(stroke_bounds(p, c, j, widths[w]), c, widths[w]);
                fvm_path_destroy(p);
            }
        }
    }
    return 0;
}
