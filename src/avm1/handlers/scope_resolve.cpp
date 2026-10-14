/* FlashVM: lexical resolution retains binding class before getter callbacks run. */
#include "support.hpp"
#include "scope_lookup_constants.h"
#include "../../core/property_key.hpp"
/* Resolve scopes from inner to outer, preserving the object which supplied a binding. */
extern "C" FVM_Result fvm_exec_scope_resolve(FVM_Context *c, FVM_Object *scope,
                                             const FVM_Value *name, FVM_Value *out,
                                             FVM_Object **owner, int *found, int *with_scope)
{
    if (!c || !out || !owner || !found || !with_scope || (scope && scope->owner != c) ||
        fvm_validate_value(c, name) != FVM_OK || name->type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_SCOPE_LOOKUP_ROOT_COUNT);
        roots.values[FVM_SCOPE_LOOKUP_NAME_ROOT] = *name;
        if (scope)
        {
            roots.values[FVM_SCOPE_LOOKUP_SCOPE_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_SCOPE_LOOKUP_SCOPE_ROOT].object = scope;
        }
        fvm_detail::string key;
        FVM_Result status = fvm_property_key(name->string, fvm_string_size(*name), key);
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
            for (FVM_Object *cursor = object; cursor; cursor = cursor->prototype)
            {
                if (cursor != object && ++depth > FVM_PROTOTYPE_MAX_DEPTH)
                    return FVM_ERROR;
                int own = 0;
                status = fvm_object_has_own_bytes(c, cursor, key.data(), key.size(), &own);
                if (status != FVM_OK)
                    return status;
                if (own)
                {
                    FVM_Value result;
                    status = fvm_object_get_bytes(c, object, key.data(), key.size(), &result);
                    if (status != FVM_OK)
                        return status;
                    *out = result;
                    *owner = object;
                    *found = 1;
                    *with_scope = with ? 1 : 0;
                    return FVM_OK;
                }
                /* Activation prototypes are enclosing scopes, visited by the outer loop. */
                if (!with)
                    break;
            }
        }
        fvm_value_undefined(out);
        *owner = 0;
        *found = 0;
        *with_scope = 0;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
