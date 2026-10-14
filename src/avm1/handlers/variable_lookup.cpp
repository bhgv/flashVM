/* FlashVM: ordinary variable lookup and callable receiver selection over managed values. */
#include "support.hpp"
#include "variable_lookup_constants.h"
#include "../../core/property_key.hpp"
#include "../scope_keywords.hpp"
/* Preserve local/global this spelling rules across SWF5, SWF6 and case-sensitive SWF7+. */
static bool this_keyword(FVM_Context *c, bool local_scope, const fvm_detail::string &name)
{
    bool sensitive = !c->swf_loaded || c->swf_info.version < FVM_SWF_VERSION_5 ||
                     c->swf_info.version >= FVM_SWF_VERSION_7 ||
                     (c->swf_info.version <= FVM_SWF_VERSION_5 && local_scope);
    return fvm_avm1_keyword_name(name, FVM_SCOPE_KEYWORD_THIS_NAME, FVM_SCOPE_KEYWORD_THIS_UNITS,
                                 sensitive);
}
/* Resolve a plain binding with explicit this/target/default-call receivers and no local-map API. */
extern "C" FVM_Result fvm_exec_variable_lookup(FVM_Context *c, FVM_Object *scope,
                                               FVM_Object *target, FVM_Object *receiver,
                                               FVM_Object *default_receiver, int local_scope,
                                               const FVM_Value *name, FVM_Value *out,
                                               FVM_Object **call_receiver, int *present)
{
    if (!c || !out || !call_receiver || !present || (scope && scope->owner != c) ||
        (target && target->owner != c) || (receiver && receiver->owner != c) ||
        (default_receiver && default_receiver->owner != c) ||
        fvm_validate_value(c, name) != FVM_OK || name->type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_VARIABLE_LOOKUP_ROOT_COUNT);
        FVM_Object *objects[] = {scope, target, receiver, default_receiver};
        const unsigned int slots[] = {
            FVM_VARIABLE_LOOKUP_SCOPE_ROOT, FVM_VARIABLE_LOOKUP_TARGET_ROOT,
            FVM_VARIABLE_LOOKUP_THIS_ROOT, FVM_VARIABLE_LOOKUP_DEFAULT_ROOT};
        for (size_t i = 0; i < sizeof(objects) / sizeof(objects[0]); ++i)
            if (objects[i])
            {
                roots.values[slots[i]].type = FVM_VALUE_OBJECT;
                roots.values[slots[i]].object = objects[i];
            }
        roots.values[FVM_VARIABLE_LOOKUP_NAME_ROOT] = *name;
        fvm_detail::string key;
        FVM_Result status = fvm_property_key(name->string, fvm_string_size(*name), key);
        if (status != FVM_OK)
            return status;
        FVM_Value &result = roots.values[FVM_VARIABLE_LOOKUP_RESULT_ROOT];
        FVM_Object *selected_receiver = default_receiver ? default_receiver : c->global;
        bool implicit_this = receiver && this_keyword(c, local_scope != 0, key);
        int found = 0;
        if (implicit_this && c->swf_loaded && c->swf_info.version >= FVM_SWF_VERSION_5)
        {
            result.type = FVM_VALUE_OBJECT;
            result.object = receiver;
            found = 1;
        }
        else
        {
            FVM_Value binding_name;
            status = fvm_value_string_bytes(&binding_name, key.data(), key.size());
            if (status != FVM_OK)
                return status;
            FVM_Object *owner = 0;
            int with_scope = 0;
            status = fvm_exec_scope_resolve(c, scope, &binding_name, &result, &owner, &found,
                                            &with_scope);
            if (status != FVM_OK)
                return status;
            if (found && with_scope)
                selected_receiver = owner;
            /* Preserve the unloaded host namespace's historical reserved-name priority. */
            if (!found && !c->swf_loaded && fvm_avm1_global_keyword(c, key))
            {
                result.type = FVM_VALUE_OBJECT;
                result.object = c->global;
                found = 1;
            }
            if (!found && implicit_this)
            {
                result.type = FVM_VALUE_OBJECT;
                result.object = receiver;
                found = 1;
            }
            if (!found && target && target != c->global)
            {
                unsigned int depth = 0;
                for (FVM_Object *cursor = target; cursor; cursor = cursor->prototype)
                {
                    if (++depth > FVM_PROTOTYPE_MAX_DEPTH)
                        return FVM_ERROR;
                    int own = 0;
                    status = fvm_object_has_own_bytes(c, cursor, key.data(), key.size(), &own);
                    if (status != FVM_OK)
                        return status;
                    if (own)
                    {
                        status = fvm_object_get_bytes(c, target, key.data(), key.size(), &result);
                        if (status != FVM_OK)
                            return status;
                        selected_receiver = target;
                        found = 1;
                        break;
                    }
                    if (cursor == target && fvm_avm1_movieclip_global(c, target, key))
                    {
                        result.type = FVM_VALUE_OBJECT;
                        result.object = c->global;
                        selected_receiver = target;
                        found = 1;
                        break;
                    }
                }
            }
            if (!found)
            {
                status = fvm_get_global_bytes(c, key.data(), key.size(), &result);
                if (status == FVM_OK && result.type == FVM_VALUE_UNDEFINED)
                    status = fvm_object_has_own_bytes(c, c->global, key.data(), key.size(), &found);
                else if (status == FVM_OK)
                    found = 1;
                if (status != FVM_OK)
                    return status;
            }
        }
        /* Ordinary scope/target/global bindings, including undefined, precede path fallback. */
        if (!found && fvm_avm1_global_keyword(c, key))
        {
            result.type = FVM_VALUE_OBJECT;
            result.object = c->global;
            found = 1;
        }
        *out = result;
        *call_receiver = selected_receiver;
        *present = found;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
