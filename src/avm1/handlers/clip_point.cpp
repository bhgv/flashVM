/* FlashVM: existing native point conversion shared by C/generated callers and MovieClip methods. */
#include "support.hpp"
#include "flashvm/standard.h"
#include "flashvm/matrix.h"
#include "clip_point_constants.h"
#include "../scope_keywords.hpp"
namespace
{
/* Stored own property map; raw lookup never evaluates accessors or prototypes. */
typedef fvm_detail::map<fvm_detail::string, FVM_Value> PointProperties;
/* Match stored own coordinate data without invoking getters or traversing prototypes.
 * Keep exact spelling first, as in the existing stored class-prototype lookup policy. */
PointProperties::iterator stored_coordinate(FVM_Context *c, FVM_Object *point, const char *name)
{
    PointProperties::iterator found = point->properties.find(name);
    if (found != point->properties.end() || c->swf_info.version >= FVM_SWF_VERSION_7)
        return found;
    for (found = point->properties.begin(); found != point->properties.end(); ++found)
        if (fvm_avm1_keyword_name(found->first, name, FVM_CLIP_POINT_FIELD_UNITS, false))
            return found;
    return point->properties.end();
}
/* Reproduce loaded point input conversion: NaN to zero, saturation, then truncation to twips.
 * Check the signed-32 bounds before the final safe conversion to a native long. */
long loaded_twips(double pixels)
{
    if (pixels != pixels)
        return 0;
    /* Clamp pixels first so even DBL_MAX and infinities never overflow the product. */
    if (pixels >= FVM_CLIP_POINT_MAX_TWIPS / FVM_CLIP_POINT_TWIPS_PER_PIXEL)
        return FVM_FIXED32_MAX;
    if (pixels <= FVM_CLIP_POINT_MIN_TWIPS / FVM_CLIP_POINT_TWIPS_PER_PIXEL)
        return FVM_FIXED32_MIN;
    double twips = pixels * FVM_CLIP_POINT_TWIPS_PER_PIXEL;
    if (twips >= FVM_CLIP_POINT_MAX_TWIPS)
        twips = FVM_CLIP_POINT_MAX_TWIPS;
    else if (twips <= FVM_CLIP_POINT_MIN_TWIPS)
        twips = FVM_CLIP_POINT_MIN_TWIPS;
    else
        twips = twips < 0 ? std::ceil(twips) : std::floor(twips);
    return (long)twips;
}
/* Convert a native double coefficient to binary32 without out-of-range float conversions. */
float scene_coefficient(double value)
{
    if (value != value)
        return std::numeric_limits<float>::quiet_NaN();
    double limit = std::numeric_limits<float>::max();
    if (value > limit)
        return std::numeric_limits<float>::infinity();
    if (value < -limit)
        return -std::numeric_limits<float>::infinity();
    return (float)value;
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
        long input_x = 0, input_y = 0;
        /* Copy field spelling before geometry callbacks can mutate the point's property map. */
        fvm_detail::string x_name(FVM_CLIP_POINT_X_NAME), y_name(FVM_CLIP_POINT_Y_NAME);
        FVM_Result status = FVM_OK;
        if (c->swf_loaded)
        {
            /* Loaded AVM1 reads only stored own numeric fields; no getters or coercion. */
            PointProperties::iterator own_x = stored_coordinate(c, target, FVM_CLIP_POINT_X_NAME);
            PointProperties::iterator own_y = stored_coordinate(c, target, FVM_CLIP_POINT_Y_NAME);
            if (own_x == target->properties.end() || own_y == target->properties.end() ||
                own_x->second.type != FVM_VALUE_NUMBER || own_y->second.type != FVM_VALUE_NUMBER)
                return FVM_OK;
            x_name = own_x->first;
            y_name = own_y->first;
            input_x = loaded_twips(own_x->second.number);
            input_y = loaded_twips(own_y->second.number);
        }
        else
        {
            status = coordinate(c, target, FVM_CLIP_POINT_X_NAME, x);
            if (status == FVM_OK)
                status = coordinate(c, target, FVM_CLIP_POINT_Y_NAME, y);
        }
        FVM_Affine matrix;
        if (status == FVM_OK)
            status = fvm_movieclip_world_matrix(c, clip, matrix);
        if (status != FVM_OK)
            return status;
        if (reverse && !c->swf_loaded)
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
        double result_x = 0, result_y = 0;
        if (c->swf_loaded)
        {
            /* Apply binary32 linear terms in integer twips, then modular translation. */
            FVM_TwipMatrix scene;
            scene.a = scene_coefficient(matrix.xx);
            scene.b = scene_coefficient(matrix.yx);
            scene.c = scene_coefficient(matrix.xy);
            scene.d = scene_coefficient(matrix.yy);
            scene.tx = loaded_twips(matrix.x0);
            scene.ty = loaded_twips(matrix.y0);
            long output_x = 0, output_y = 0;
            if (reverse)
                status = fvm_matrix_inverse_twips(&scene, &scene);
            if (reverse && status == FVM_INVALID_ARGUMENT)
            {
                /* A failed loaded inverse preserves the original integer point, without
                 * passing it through binary32 conversion a second time. */
                output_x = input_x;
                output_y = input_y;
                status = FVM_OK;
            }
            else if (status == FVM_OK)
                status = fvm_matrix_point_twips(&scene, input_x, input_y, &output_x, &output_y);
            if (status != FVM_OK)
                return status;
            result_x = (double)output_x / FVM_CLIP_POINT_TWIPS_PER_PIXEL;
            result_y = (double)output_y / FVM_CLIP_POINT_TWIPS_PER_PIXEL;
        }
        else
        {
            result_x = matrix.xx * x + matrix.xy * y + matrix.x0;
            result_y = matrix.yx * x + matrix.yy * y + matrix.y0;
        }
        FVM_Value value;
        fvm_value_number(&value, result_x);
        status = fvm_object_set(c, target, x_name.c_str(), &value);
        if (status != FVM_OK)
            return status;
        fvm_value_number(&value, result_y);
        return fvm_object_set(c, target, y_name.c_str(), &value);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
