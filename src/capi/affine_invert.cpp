/* FlashVM: raw double matrix inversion preserving host point-conversion arithmetic. */
#include "flashvm/affine.h"
/* Reject zero/NaN determinants; stage every field before publishing an aliased inverse. */
extern "C" FVM_Result fvm_affine_invert(const FVM_Affine *matrix, FVM_Affine *out)
{
    if (!matrix || !out)
        return FVM_INVALID_ARGUMENT;
    double determinant = matrix->xx * matrix->yy - matrix->xy * matrix->yx;
    if (!determinant || determinant != determinant)
        return FVM_INVALID_ARGUMENT;
    FVM_Affine result;
    result.xx = matrix->yy / determinant;
    result.xy = -matrix->xy / determinant;
    result.yx = -matrix->yx / determinant;
    result.yy = matrix->xx / determinant;
    result.x0 = -(result.xx * matrix->x0 + result.xy * matrix->y0);
    result.y0 = -(result.yx * matrix->x0 + result.yy * matrix->y0);
    *out = result;
    return FVM_OK;
}
