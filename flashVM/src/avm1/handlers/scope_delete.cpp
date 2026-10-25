/* FlashVM: getter-free deletion through managed lexical scopes and fallback objects. */
#include "support.hpp"
#include "../scope_keywords.hpp"
#include "scope_delete_constants.h"
#include "../../core/property_key.hpp"


/* Find presence without reading the value; an inherited property blocks outer deletion. */
static FVM_Result delete_binding(
    FVM_Context *c, FVM_Object *object, const fvm_detail::string &key, bool prototypes, bool &present, int &deleted)
{
    unsigned int depth = 0;
    for (FVM_Object *cursor = object; cursor; cursor = prototypes ? cursor->prototype : 0)
    {
        if (++depth > FVM_PROTOTYPE_MAX_DEPTH)
        {
            return FVM_ERROR;
        }
        int own = 0;
        FVM_Result status = fvm_object_has_own_bytes(c, cursor, key.data(), key.size(), &own);
        if (status != FVM_OK)
        {
            return status;
        }
        if (own)
        {
            present = true;
            if (cursor == object && object->resource_kind != FVM_RESOURCE_SUPER)
            {
                return fvm_object_delete_bytes(c, object, key.data(), key.size(), &deleted);
            }
            return FVM_OK;
        }
        if (fvm_avm1_movieclip_path_kind(c, cursor, key) != FVM_SCOPE_PATH_NONE)
        {
            present = true;
            return FVM_OK;
        }
    }
    return FVM_OK;
}


/* Delete a simple binding with no bytecode, operand stack or host local-map dependency. */
extern "C" FVM_Result
fvm_exec_scope_delete(FVM_Context *c, FVM_Object *scope, FVM_Object *target, const FVM_Value *name, FVM_Value *out)
{
    if (!c || !out || (scope && scope->owner != c) || (target && target->owner != c) ||
        fvm_validate_value(c, name) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        FVM_ExecutionRoots roots(c, FVM_SCOPE_DELETE_ROOT_COUNT);
        if (scope)
        {
            roots.values[FVM_SCOPE_DELETE_SCOPE_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_SCOPE_DELETE_SCOPE_ROOT].object = scope;
        }
        if (target)
        {
            roots.values[FVM_SCOPE_DELETE_TARGET_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_SCOPE_DELETE_TARGET_ROOT].object = target;
        }
        roots.values[FVM_SCOPE_DELETE_NAME_ROOT] = *name;
        FVM_Value &text = roots.values[FVM_SCOPE_DELETE_TEXT_ROOT];
        FVM_Result status = fvm_exec_to_string(c, &roots.values[FVM_SCOPE_DELETE_NAME_ROOT], &text);
        if (status != FVM_OK)
        {
            return status;
        }
        fvm_detail::string key;
        status = fvm_property_key(text.string, fvm_string_size(text), key);
        if (status != FVM_OK)
        {
            return status;
        }
        unsigned int depth = 0;
        bool present = false;
        int deleted = 0;
        for (FVM_Object *node = scope; node; node = node->prototype)
        {
            if (++depth > FVM_PROTOTYPE_MAX_DEPTH)
            {
                return FVM_ERROR;
            }
            FVM_Object *object = node;
            bool with = node->resource_kind == FVM_RESOURCE_WITH_SCOPE;
            if (with)
            {
                if (!node->resource_result_valid || node->resource_result.type != FVM_VALUE_OBJECT)
                {
                    return FVM_INVALID_ARGUMENT;
                }
                object = node->resource_result.object;
            }
            status = delete_binding(c, object, key, with, present, deleted);
            if (status != FVM_OK)
            {
                return status;
            }
            if (present)
            {
                fvm_value_boolean(out, deleted);
                return FVM_OK;
            }
        }
        if (target && target != c->global)
        {
            status = delete_binding(c, target, key, true, present, deleted);
            if (status != FVM_OK)
            {
                return status;
            }
        }
        if (!present)
        {
            status = delete_binding(c, c->global, key, true, present, deleted);
            if (status != FVM_OK)
            {
                return status;
            }
        }
        fvm_value_boolean(out, deleted);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
