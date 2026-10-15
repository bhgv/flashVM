/* FlashVM: strict double inverse projection shared by pointer and drag handlers. */
#include "flashvm/affine.h"
#include <limits>
/* Stage both results before publication so aliased matrix fields and errors are safe. */
extern "C" FVM_Result fvm_affine_inverse_point(const FVM_Affine *matrix, int mode, double x,
                                               double y, double *out_x, double *out_y)
{
    if (!matrix || !out_x || !out_y || out_x == out_y ||
        (mode != FVM_AFFINE_POINT && mode != FVM_AFFINE_VECTOR))
        return FVM_INVALID_ARGUMENT;
    double determinant = matrix->xx * matrix->yy - matrix->xy * matrix->yx;
    double limit = std::numeric_limits<double>::max();
    if (!determinant || !(determinant >= -limit && determinant <= limit))
        return FVM_INVALID_ARGUMENT;
    double dx = mode == FVM_AFFINE_POINT ? x - matrix->x0 : x;
    double dy = mode == FVM_AFFINE_POINT ? y - matrix->y0 : y;
    double result_x = (matrix->yy * dx - matrix->xy * dy) / determinant;
    double result_y = (matrix->xx * dy - matrix->yx * dx) / determinant;
    if (!(result_x >= -limit && result_x <= limit && result_y >= -limit && result_y <= limit))
        return FVM_INVALID_ARGUMENT;
    *out_x = result_x;
    *out_y = result_y;
    return FVM_OK;
}
