/* FlashVM: existing native point conversion shared by C/generated callers and MovieClip methods. */
#include "support.hpp"
#include "flashvm/standard.h"
#include "clip_point_constants.h"
namespace
{
/* Preserve native point-field lookup/coercion and finite-number validation. */
FVM_Result coordinate(FVM_Context *c, FVM_Object *point, const char *name, double &out)
{
    FVM_Value value;
    FVM_Result status = fvm_object_get(c, point, name, &value);
    if (status == FVM_OK)
        status = fvm_value_to_number(c, &value, &out);
    double limit = std::numeric_limits<double>::max();
    return status != FVM_OK                ? status
           : out >= -limit && out <= limit ? FVM_OK
                                           : FVM_INVALID_ARGUMENT;
}
} // namespace
/* Read x then y, snapshot world geometry and write x then y; setter side effects stay ordered. */
extern "C" FVM_Result fvm_exec_clip_point(FVM_Context *c, FVM_Object *clip, int reverse,
                                          const FVM_Value *point)
{
    if (!c || c->destroying || c->gc_releasing || !clip || clip->owner != c ||
        clip->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_validate_value(c, point) != FVM_OK ||
        point->type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Keep both explicit operands reachable through field and geometry callbacks. */
        FVM_ExecutionRoots roots(c, FVM_CLIP_POINT_ROOT_COUNT);
        roots.values[FVM_CLIP_POINT_CLIP_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_CLIP_POINT_CLIP_ROOT].object = clip;
        roots.values[FVM_CLIP_POINT_VALUE_ROOT] = *point;
        FVM_NativeCallGuard guard(c);
        FVM_Object *target = roots.values[FVM_CLIP_POINT_VALUE_ROOT].object;
        double x = 0, y = 0;
        FVM_Result status = coordinate(c, target, "x", x);
        if (status == FVM_OK)
            status = coordinate(c, target, "y", y);
        FVM_Affine matrix;
        if (status == FVM_OK)
            status = fvm_movieclip_world_matrix(c, clip, matrix);
        if (status != FVM_OK)
            return status;
        if (reverse)
        {
            double determinant = matrix.xx * matrix.yy - matrix.xy * matrix.yx;
            if (!determinant || determinant != determinant)
                return FVM_INVALID_ARGUMENT;
            FVM_Affine old = matrix;
            matrix.xx = old.yy / determinant;
            matrix.xy = -old.xy / determinant;
            matrix.yx = -old.yx / determinant;
            matrix.yy = old.xx / determinant;
            matrix.x0 = -(matrix.xx * old.x0 + matrix.xy * old.y0);
            matrix.y0 = -(matrix.yx * old.x0 + matrix.yy * old.y0);
        }
        FVM_Value value;
        fvm_value_number(&value, matrix.xx * x + matrix.xy * y + matrix.x0);
        status = fvm_object_set(c, target, "x", &value);
        if (status != FVM_OK)
            return status;
        fvm_value_number(&value, matrix.yx * x + matrix.yy * y + matrix.y0);
        return fvm_object_set(c, target, "y", &value);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
