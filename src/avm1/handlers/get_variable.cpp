/* FlashVM: complete GetVariable operation over explicit scopes and values. */
#include "support.hpp"
#include "get_variable_constants.h"
/* Coerce loaded names, resolve qualified/plain values, and hide SWF4 MovieClip results. */
extern "C" FVM_Result fvm_exec_get_variable(FVM_Context *c, FVM_Object *scope, FVM_Object *target,
                                            FVM_Object *receiver, int local_scope,
                                            const FVM_Value *name, FVM_Value *out)
{
    if (!c || !out || (scope && scope->owner != c) || (target && target->owner != c) ||
        (receiver && receiver->owner != c) || fvm_validate_value(c, name) != FVM_OK ||
        (!c->swf_loaded && name->type != FVM_VALUE_STRING))
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] A name conversion may detach scope/target/name objects and collect. */
        FVM_ExecutionRoots roots(c, FVM_GET_VARIABLE_ROOT_COUNT);
        FVM_Object *objects[] = {scope, target, receiver};
        const unsigned int slots[] = {FVM_GET_VARIABLE_SCOPE_ROOT, FVM_GET_VARIABLE_TARGET_ROOT,
                                      FVM_GET_VARIABLE_THIS_ROOT};
        for (size_t i = 0; i < sizeof(objects) / sizeof(objects[0]); ++i)
            if (objects[i])
            {
                roots.values[slots[i]].type = FVM_VALUE_OBJECT;
                roots.values[slots[i]].object = objects[i];
            }
        roots.values[FVM_GET_VARIABLE_NAME_ROOT] = *name;
        FVM_Value &text = roots.values[FVM_GET_VARIABLE_TEXT_ROOT];
        FVM_Result status = FVM_OK;
        if (name->type == FVM_VALUE_STRING)
            text = *name;
        else
            status = fvm_exec_to_string(c, &roots.values[FVM_GET_VARIABLE_NAME_ROOT], &text);
        if (status != FVM_OK)
            return status;
        FVM_Value &value = roots.values[FVM_GET_VARIABLE_VALUE_ROOT];
        int handled = 0, present = 0;
        status =
            fvm_exec_variable_path(c, scope, target, receiver, &text, &value, &handled, &present);
        if (status == FVM_OK && !handled)
        {
            FVM_Object *ignored_receiver = 0;
            status = fvm_exec_variable_lookup(c, scope, target, receiver, 0, local_scope, &text,
                                              &value, &ignored_receiver, &present);
        }
        if (status != FVM_OK)
            return status;
        if (c->swf_loaded && c->swf_info.version < FVM_SWF_VERSION_5 &&
            value.type == FVM_VALUE_OBJECT &&
            value.object->resource_kind == FVM_RESOURCE_MOVIECLIP &&
            !fvm_movieclip_is_class_prototype(value.object))
            fvm_value_undefined(&value);
        *out = value;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
