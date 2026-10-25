#include "cubic_scale_fixture.hpp"


int main()
{
    for (int e = -400; e >= -1000; e -= 100)
    {
        FVM_Path *p = cb_path(1, e);
        FVM_PathBounds b = cb_normalized(p, e);
        bounds_equal(b, 0, 0, 0, 16.0 / 3);
        fvm_path_destroy(p);
    }
    return 0;
}
