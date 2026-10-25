#include "path_style_bounds_fixture.hpp"


int main()
{
    for (unsigned cap = 0; cap < 3; ++cap)
    {
        for (unsigned join = 0; join < 2; ++join)
        {
            FVM_Path *p = hit_line();
            bounds_equal(stroke_bounds(p, cap, join), cap == 1 ? 2 : 0, 6, cap == 1 ? 14 : 16, 10);
            fvm_path_destroy(p);
            p = hit_line(8, 2, 8, 14);
            bounds_equal(stroke_bounds(p, cap, join), 6, cap == 1 ? 2 : 0, 10, cap == 1 ? 14 : 16);
            fvm_path_destroy(p);
        }
    }
    return 0;
}
