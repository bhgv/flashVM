/* Named dimensions use the same geometry and rotated scaling as AVM1 numeric properties. */
#include "movieclip_dimensions.hpp"
#include "movieclip_state.hpp"
#include "../avm1/coercion.hpp"
#include "../avm1/display_property_ids.h"

/* Read without consulting a replaceable property of the same name. */
static FVM_Result dimension_get(FVM_Context *c, FVM_Object *clip, unsigned index, FVM_Value *out)
{
    if (fvm_movieclip_is_class_prototype(clip))
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return fvm_movieclip_get_action_property(c, clip, index, out);
}

/* Coercion may execute user code; publish only after it succeeds.
 * [GC:] Object accessor invocation retains the receiver and argument through conversion.
 * Null/undefined are ignored, matching the numeric display-property assignment policy. */
static FVM_Result dimension_set(FVM_Context *c, FVM_Object *clip, unsigned argc,
                                const FVM_Value *argv, FVM_Value *out, unsigned index)
{
    fvm_value_undefined(out);
    if (!argc || fvm_movieclip_is_class_prototype(clip) || argv[0].type == FVM_VALUE_NULL ||
        argv[0].type == FVM_VALUE_UNDEFINED)
        return FVM_OK;
    double number = 0;
    FVM_Result result = fvm_avm1_number(c, argv, &number);
    if (result != FVM_OK)
        return result;
    FVM_Value converted;
    fvm_value_number(&converted, number);
    return fvm_movieclip_set_action_property(c, clip, index, converted);
}

/* Width getter has its own stable callback identity; no pointer-valued userdata. */
FVM_Result fvm_movieclip_width_get(FVM_Context *c, FVM_Object *clip, unsigned, const FVM_Value *,
                                   FVM_Value *out, void *)
{
    return dimension_get(c, clip, FVM_AVM1_PROPERTY_WIDTH, out);
}
/* Width setter preserves conversion errors and intrinsic scale publication rules. */
FVM_Result fvm_movieclip_width_set(FVM_Context *c, FVM_Object *clip, unsigned argc,
                                   const FVM_Value *argv, FVM_Value *out, void *)
{
    return dimension_set(c, clip, argc, argv, out, FVM_AVM1_PROPERTY_WIDTH);
}
/* Height getter has a distinct transportable callback identity. */
FVM_Result fvm_movieclip_height_get(FVM_Context *c, FVM_Object *clip, unsigned, const FVM_Value *,
                                    FVM_Value *out, void *)
{
    return dimension_get(c, clip, FVM_AVM1_PROPERTY_HEIGHT, out);
}
/* Height setter uses the same native resize service as ActionSetProperty. */
FVM_Result fvm_movieclip_height_set(FVM_Context *c, FVM_Object *clip, unsigned argc,
                                    const FVM_Value *argv, FVM_Value *out, void *)
{
    return dimension_set(c, clip, argc, argv, out, FVM_AVM1_PROPERTY_HEIGHT);
}
