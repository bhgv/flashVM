/* FlashVM: version-aware qualified/slash variable-path phase shared by execution clients. */
#include "support.hpp"
#include "variable_path_constants.h"
#include "reference_path_constants.h"
/* Handle qualified paths; leave the value output untouched when simple lookup must follow. */
extern "C" FVM_Result fvm_exec_variable_path(FVM_Context *c, FVM_Object *scope, FVM_Object *target,
                                             FVM_Object *receiver, const FVM_Value *name,
                                             FVM_Value *out, int *handled, int *present)
{
    if (!c || !out || !handled || !present || (scope && scope->owner != c) ||
        (target && target->owner != c) || (receiver && receiver->owner != c) ||
        fvm_validate_value(c, name) != FVM_OK || name->type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    try
    {
        size_t size = fvm_string_size(*name), separator = size;
        bool modern = !c->swf_loaded || c->swf_info.version >= FVM_SWF_VERSION_5;
        bool slash = false;
        unsigned int segments = 1;
        for (size_t at = 0; at < size; ++at)
        {
            if (name->string[at] == FVM_REFERENCE_PATH_COLON ||
                (modern && name->string[at] == FVM_REFERENCE_PATH_DOT))
            {
                if (++segments > FVM_REFERENCE_PATH_MAX_SEGMENTS)
                    return FVM_ERROR;
                separator = at;
            }
            if (name->string[at] == FVM_REFERENCE_PATH_SLASH)
                slash = true;
        }
        if (separator == size && !(modern && slash))
        {
            *handled = 0;
            *present = 0;
            return FVM_OK;
        }
        FVM_ExecutionRoots roots(c, FVM_VARIABLE_PATH_ROOT_COUNT);
        FVM_Object *objects[] = {scope, target, receiver};
        const unsigned int slots[] = {FVM_VARIABLE_PATH_SCOPE_ROOT, FVM_VARIABLE_PATH_TARGET_ROOT,
                                      FVM_VARIABLE_PATH_THIS_ROOT};
        for (size_t i = 0; i < sizeof(objects) / sizeof(objects[0]); ++i)
            if (objects[i])
            {
                roots.values[slots[i]].type = FVM_VALUE_OBJECT;
                roots.values[slots[i]].object = objects[i];
            }
        roots.values[FVM_VARIABLE_PATH_NAME_ROOT] = *name;
        const FVM_Value &text = roots.values[FVM_VARIABLE_PATH_NAME_ROOT];
        FVM_Value &result = roots.values[FVM_VARIABLE_PATH_RESULT_ROOT];
        int found = 0;
        if (separator != size)
        {
            if (separator)
            {
                FVM_Value &prefix = roots.values[FVM_VARIABLE_PATH_PREFIX_ROOT];
                FVM_Value &member = roots.values[FVM_VARIABLE_PATH_MEMBER_ROOT];
                FVM_Result status = fvm_value_string_bytes(&prefix, text.string, separator);
                if (status == FVM_OK)
                    status = fvm_value_string_bytes(&member, text.string + separator + 1,
                                                    size - separator - 1);
                if (status == FVM_OK)
                    status = fvm_exec_scope_member(c, scope, target, receiver, &prefix, &member,
                                                   &result, &found);
                if (status != FVM_OK)
                    return status;
            }
            *out = result;
            *handled = 1;
            *present = found;
            return FVM_OK;
        }
        if (modern && slash)
        {
            FVM_Result status = fvm_exec_scope_target(c, scope, target, receiver, &text, &result);
            if (status != FVM_OK)
                return status;
            if (result.type == FVM_VALUE_OBJECT)
            {
                *out = result;
                *handled = 1;
                *present = 1;
                return FVM_OK;
            }
        }
        *handled = 0;
        *present = 0;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
