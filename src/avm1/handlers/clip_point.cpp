/* FlashVM: existing native point conversion shared by C/generated callers and MovieClip methods. */
#include "support.hpp"
#include "flashvm/standard.h"
#include "clip_point_constants.h"
namespace
{
/* Stored own property map; raw lookup never evaluates accessors or prototypes. */
typedef fvm_detail::map<fvm_detail::string, FVM_Value> PointProperties;
/* Reproduce loaded point input conversion: NaN to zero, saturation, then truncation to twips.
 * Keep the bounded twip integer in double so C++98 needs no wider-than-long integer type. */
double loaded_coordinate(double pixels)
{
    if (pixels != pixels)
        return 0;
    /* Clamp pixels first so even DBL_MAX and infinities never overflow the product. */
    if (pixels >= FVM_CLIP_POINT_MAX_TWIPS / FVM_CLIP_POINT_TWIPS_PER_PIXEL)
        return FVM_CLIP_POINT_MAX_TWIPS / FVM_CLIP_POINT_TWIPS_PER_PIXEL;
    if (pixels <= FVM_CLIP_POINT_MIN_TWIPS / FVM_CLIP_POINT_TWIPS_PER_PIXEL)
        return FVM_CLIP_POINT_MIN_TWIPS / FVM_CLIP_POINT_TWIPS_PER_PIXEL;
    double twips = pixels * FVM_CLIP_POINT_TWIPS_PER_PIXEL;
    if (twips >= FVM_CLIP_POINT_MAX_TWIPS)
        twips = FVM_CLIP_POINT_MAX_TWIPS;
    else if (twips <= FVM_CLIP_POINT_MIN_TWIPS)
        twips = FVM_CLIP_POINT_MIN_TWIPS;
    else
        twips = twips < 0 ? std::ceil(twips) : std::floor(twips);
    return twips / FVM_CLIP_POINT_TWIPS_PER_PIXEL;
}
/* Preserve the strict host point-field lookup/coercion and finite-number validation. */
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
        clip->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_validate_value(c, point) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    if (point->type != FVM_VALUE_OBJECT)
        return c->swf_loaded ? FVM_OK : FVM_INVALID_ARGUMENT;
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
        FVM_Result status = FVM_OK;
        if (c->swf_loaded)
        {
            /* Loaded AVM1 reads only stored own numeric fields; no getters or coercion. */
            PointProperties::iterator own_x = target->properties.find("x");
            PointProperties::iterator own_y = target->properties.find("y");
            if (own_x == target->properties.end() || own_y == target->properties.end() ||
                own_x->second.type != FVM_VALUE_NUMBER || own_y->second.type != FVM_VALUE_NUMBER)
                return FVM_OK;
            x = loaded_coordinate(own_x->second.number);
            y = loaded_coordinate(own_y->second.number);
        }
        else
        {
            status = coordinate(c, target, "x", x);
            if (status == FVM_OK)
                status = coordinate(c, target, "y", y);
        }
        FVM_Affine matrix;
        if (status == FVM_OK)
            status = fvm_movieclip_world_matrix(c, clip, matrix);
        if (status != FVM_OK)
            return status;
        if (reverse)
        {
            double determinant = matrix.xx * matrix.yy - matrix.xy * matrix.yx;
            if (!determinant || determinant != determinant)
            {
                if (!c->swf_loaded)
                    return FVM_INVALID_ARGUMENT;
                /* Loaded globalToLocal falls back to the unchanged world point. */
                matrix.xx = matrix.yy = 1;
                matrix.xy = matrix.yx = matrix.x0 = matrix.y0 = 0;
            }
            else
            {
                FVM_Affine old = matrix;
                matrix.xx = old.yy / determinant;
                matrix.xy = -old.xy / determinant;
                matrix.yx = -old.yx / determinant;
                matrix.yy = old.xx / determinant;
                matrix.x0 = -(matrix.xx * old.x0 + matrix.xy * old.y0);
                matrix.y0 = -(matrix.yx * old.x0 + matrix.yy * old.y0);
            }
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
