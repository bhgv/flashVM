/* FlashVM: raw double affine application independent of execution state. */
#include "flashvm/affine.h"


/* Evaluate both coordinates before either output can overwrite an aliased coefficient. */
extern "C" FVM_Result
fvm_affine_transform_point(const FVM_Affine *matrix, double x, double y, double *out_x, double *out_y)
{
    if (!matrix || !out_x || !out_y || out_x == out_y)
    {
        return FVM_INVALID_ARGUMENT;
    }
    double result_x = matrix->xx * x + matrix->xy * y + matrix->x0;
    double result_y = matrix->yx * x + matrix->yy * y + matrix->y0;
    *out_x = result_x;
    *out_y = result_y;
    return FVM_OK;
}
