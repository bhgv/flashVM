/* FlashVM: getter-free binding selection and qualified variable writes for C execution clients. */
#include "support.hpp"
#include "variable_set_constants.h"
#include "reference_path_constants.h"
#include "../../core/property_key.hpp"
/* Select assignment destinations without reading the previous value or accepting interpreter state.
 */
extern "C" FVM_Result fvm_exec_variable_set(FVM_Context *c, FVM_Object *scope, FVM_Object *target,
                                            FVM_Object *receiver, const FVM_Value *name,
                                            const FVM_Value *value)
{
    if (!c || (scope && scope->owner != c) || (target && target->owner != c) ||
        (receiver && receiver->owner != c) || fvm_validate_value(c, name) != FVM_OK ||
        name->type != FVM_VALUE_STRING || fvm_validate_value(c, value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        size_t size = fvm_string_size(*name), separator = size;
        if (!size)
            return FVM_OK;
        unsigned int segments = 1;
        for (size_t at = 0; at < size; ++at)
            if (name->string[at] == FVM_REFERENCE_PATH_DOT ||
                name->string[at] == FVM_REFERENCE_PATH_COLON)
            {
                if (++segments > FVM_REFERENCE_PATH_MAX_SEGMENTS)
                    return FVM_ERROR;
                separator = at;
            }
        FVM_ExecutionRoots roots(c, FVM_VARIABLE_SET_ROOT_COUNT);
        FVM_Object *objects[] = {scope, target, receiver};
        const unsigned int slots[] = {FVM_VARIABLE_SET_SCOPE_ROOT, FVM_VARIABLE_SET_TARGET_ROOT,
                                      FVM_VARIABLE_SET_THIS_ROOT};
        for (size_t i = 0; i < sizeof(objects) / sizeof(objects[0]); ++i)
            if (objects[i])
            {
                roots.values[slots[i]].type = FVM_VALUE_OBJECT;
                roots.values[slots[i]].object = objects[i];
            }
        roots.values[FVM_VARIABLE_SET_NAME_ROOT] = *name;
        roots.values[FVM_VARIABLE_SET_VALUE_ROOT] = *value;
        const FVM_Value &text = roots.values[FVM_VARIABLE_SET_NAME_ROOT];
        if (separator != size)
        {
            FVM_Value &prefix = roots.values[FVM_VARIABLE_SET_PREFIX_ROOT];
            FVM_Value &member = roots.values[FVM_VARIABLE_SET_MEMBER_ROOT];
            FVM_Value &owner = roots.values[FVM_VARIABLE_SET_OWNER_ROOT];
            FVM_Result status = fvm_value_string_bytes(&prefix, text.string, separator);
            if (status == FVM_OK)
                status = fvm_value_string_bytes(&member, text.string + separator + 1,
                                                size - separator - 1);
            if (status == FVM_OK)
                status = fvm_exec_scope_target(c, scope, target, receiver, &prefix, &owner);
            if (status != FVM_OK || owner.type != FVM_VALUE_OBJECT)
                return status;
            return fvm_object_set_bytes(c, owner.object, member.string, fvm_string_size(member),
                                        &roots.values[FVM_VARIABLE_SET_VALUE_ROOT]);
        }
        fvm_detail::string key;
        FVM_Result status = fvm_property_key(text.string, size, key);
        if (status != FVM_OK)
            return status;
        unsigned int depth = 0;
        for (FVM_Object *node = scope; node; node = node->prototype)
        {
            if (++depth > FVM_PROTOTYPE_MAX_DEPTH)
                return FVM_ERROR;
            FVM_Object *object = node;
            bool with = node->resource_kind == FVM_RESOURCE_WITH_SCOPE;
            if (with)
            {
                if (!node->resource_result_valid || node->resource_result.type != FVM_VALUE_OBJECT)
                    return FVM_INVALID_ARGUMENT;
                object = node->resource_result.object;
            }
            unsigned int inherited = 0;
            for (FVM_Object *cursor = object; cursor; cursor = with ? cursor->prototype : 0)
            {
                if (++inherited > FVM_PROTOTYPE_MAX_DEPTH)
                    return FVM_ERROR;
                int own = 0;
                status = fvm_object_has_own_bytes(c, cursor, key.data(), key.size(), &own);
                if (status != FVM_OK)
                    return status;
                if (own)
                    return fvm_object_set_bytes(c, object, key.data(), key.size(),
                                                &roots.values[FVM_VARIABLE_SET_VALUE_ROOT]);
            }
        }
        return fvm_object_set_bytes(c, target ? target : c->global, key.data(), key.size(),
                                    &roots.values[FVM_VARIABLE_SET_VALUE_ROOT]);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
