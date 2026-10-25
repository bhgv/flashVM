/* FlashVM: value-based TargetPath shared by interpreter and generated bodies. */
#include "support.hpp"
#include "target_path_constants.h"


/* Box the supplied value, then read a real clip's native dot path without script string lookup. */
extern "C" FVM_Result fvm_exec_target_path(FVM_Context *context, const FVM_Value *input, FVM_Value *output)
{
    if (!context || context->destroying || context->gc_releasing || !output ||
        fvm_validate_value(context, input) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Keep the original value and constructor result alive across boxing and copying. */
        FVM_ExecutionRoots roots(context, FVM_TARGET_PATH_ROOT_COUNT);
        roots.values[FVM_TARGET_PATH_INPUT_ROOT] = *input;
        FVM_Value &boxed = roots.values[FVM_TARGET_PATH_BOXED_ROOT];
        FVM_Result status = fvm_exec_to_object(context, &roots.values[FVM_TARGET_PATH_INPUT_ROOT], &boxed);
        if (status != FVM_OK)
        {
            return status;
        }
        if (boxed.type != FVM_VALUE_OBJECT)
        {
            FVM_Object *bare = 0;
            status = fvm_object_create(context, &bare);
            if (status != FVM_OK)
            {
                return status;
            }
            boxed.type = FVM_VALUE_OBJECT;
            boxed.object = bare;
        }
        if (boxed.object->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(boxed.object))
        {
            fvm_value_undefined(output);
            return FVM_OK;
        }
        FVM_Value path;
        status = fvm_movieclip_reference_string(context, boxed.object, &path);
        if (status == FVM_OK)
        {
            *output = path;
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
