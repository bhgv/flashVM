#include "round_curve_fixture.hpp"


int main()
{
    FVM_Path *p = rc_path(1);
    bounds_equal(stroke_bounds(p, 0), 0, 0, 16, 10);
    fvm_path_destroy(p);
    p = rc_path(2);
    FVM_PathBounds b = stroke_bounds(p, 0);
    bounds_equal(b, 0, 6 - 2 * std::sqrt(3.0), 16, 10 + 2 * std::sqrt(3.0));
    fvm_path_destroy(p);
    return 0;
}
