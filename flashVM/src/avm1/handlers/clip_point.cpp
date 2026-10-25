/* FlashVM: existing native point conversion shared by C/generated callers and MovieClip methods. */
#include "support.hpp"
#include "flashvm/standard.h"
#include "flashvm/matrix.h"
#include "flashvm/affine.h"
#include "clip_point_constants.h"
#include "../scope_keywords.hpp"
#include "../../core/scene_numeric.hpp"

namespace
{ /* Stored own property map; raw lookup never evaluates accessors or prototypes. */
typedef fvm_detail::map<fvm_detail::string, FVM_Value> PointProperties;


/* Match stored own coordinate data without invoking getters or traversing prototypes.
 * Keep exact spelling first, as in the existing stored class-prototype lookup policy. */
PointProperties::iterator stored_coordinate(FVM_Context *c, FVM_Object *point, const char *name)
{
    PointProperties::iterator found = point->properties.find(name);
    if (found != point->properties.end() || c->swf_info.version >= FVM_SWF_VERSION_7)
    {
        return found;
    }
    for (found = point->properties.begin(); found != point->properties.end(); ++found)
    {
        if (fvm_avm1_keyword_name(found->first, name, FVM_CLIP_POINT_FIELD_UNITS, false))
        {
            return found;
        }
    }
    return point->properties.end();
}


/* Preserve the strict host point-field lookup/coercion and finite-number validation. */
FVM_Result coordinate(FVM_Context *c, FVM_Object *point, const char *name, double &out)
{
    FVM_Value value;
    FVM_Result status = fvm_object_get(c, point, name, &value);
    if (status == FVM_OK)
    {
        status = fvm_value_to_number(c, &value, &out);
    }
    double limit = std::numeric_limits<double>::max();
    return status != FVM_OK ? status : out >= -limit && out <= limit ? FVM_OK : FVM_INVALID_ARGUMENT;
}
} // namespace


/* Read x then y, snapshot world geometry and write x then y; setter side effects stay ordered. */
extern "C" FVM_Result fvm_exec_clip_point(FVM_Context *c, FVM_Object *clip, int reverse, const FVM_Value *point)
{
    if (!c || c->destroying || c->gc_releasing || !clip || clip->owner != c ||
        clip->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_validate_value(c, point) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    /* Keep the selected arithmetic representation stable if a geometry getter loads a SWF. */
    const bool loaded = c->swf_loaded;
    if (point->type != FVM_VALUE_OBJECT)
    {
        return loaded ? FVM_OK : FVM_INVALID_ARGUMENT;
    }
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
        if (loaded)
        { /* Loaded AVM1 reads only stored own numeric fields; no getters or coercion. */
            PointProperties::iterator own_x = stored_coordinate(c, target, FVM_CLIP_POINT_X_NAME);
            PointProperties::iterator own_y = stored_coordinate(c, target, FVM_CLIP_POINT_Y_NAME);
            if (own_x == target->properties.end() || own_y == target->properties.end() ||
                own_x->second.type != FVM_VALUE_NUMBER || own_y->second.type != FVM_VALUE_NUMBER)
            {
                return FVM_OK;
            }
            x_name = own_x->first;
            y_name = own_y->first;
            input_x = fvm_scene_detail::input_twips(own_x->second.number);
            input_y = fvm_scene_detail::input_twips(own_y->second.number);
        }
        else
        {
            status = coordinate(c, target, FVM_CLIP_POINT_X_NAME, x);
            if (status == FVM_OK)
            {
                status = coordinate(c, target, FVM_CLIP_POINT_Y_NAME, y);
            }
        }
        FVM_Affine matrix;
        FVM_TwipMatrix scene;
        if (status == FVM_OK)
        {
            status = loaded ? fvm_exec_clip_world_matrix(c, clip, &scene) : fvm_movieclip_world_matrix(c, clip, matrix);
        }
        if (status != FVM_OK)
        {
            return status;
        }
        if (reverse && !loaded)
        {
            status = fvm_affine_invert(&matrix, &matrix);
            if (status != FVM_OK)
            {
                return status;
            }
        }
        double result_x = 0, result_y = 0;
        if (loaded)
        { /* Apply binary32 linear terms in integer twips, then modular translation. */
            long output_x = 0, output_y = 0;
            if (reverse)
            {
                status = fvm_matrix_inverse_twips(&scene, &scene);
            }
            if (reverse && status == FVM_INVALID_ARGUMENT)
            {
                /* A failed loaded inverse preserves the original integer point, without
                 * passing it through binary32 conversion a second time. */
                output_x = input_x;
                output_y = input_y;
                status = FVM_OK;
            }
            else if (status == FVM_OK)
            {
                status = fvm_matrix_point_twips(&scene, input_x, input_y, &output_x, &output_y);
            }
            if (status != FVM_OK)
            {
                return status;
            }
            result_x = (double)output_x / FVM_MATRIX_TWIPS_PER_PIXEL;
            result_y = (double)output_y / FVM_MATRIX_TWIPS_PER_PIXEL;
        }
        else
        {
            status = fvm_affine_transform_point(&matrix, x, y, &result_x, &result_y);
            if (status != FVM_OK)
            {
                return status;
            }
        }
        FVM_Value value;
        fvm_value_number(&value, result_x);
        status = fvm_object_set(c, target, x_name.c_str(), &value);
        if (status != FVM_OK)
        {
            return status;
        }
        fvm_value_number(&value, result_y);
        return fvm_object_set(c, target, y_name.c_str(), &value);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
