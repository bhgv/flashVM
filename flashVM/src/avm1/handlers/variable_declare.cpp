/* FlashVM: versioned initialized/uninitialized local declarations over managed scopes. */
#include "support.hpp"
#include "../scope_keywords.hpp"
#include "flashvm/standard.h"
#include "variable_declare_constants.h"
#include "reference_path_constants.h"
#include "../../core/property_key.hpp"
#include "../../standard/base_state.hpp"


/* Check own/prototype membership; only DefineLocal2 checks intrinsic path presence as well. */
static FVM_Result declaration_present(FVM_Context *c,
                                      FVM_Object *object,
                                      const fvm_detail::string &name,
                                      bool prototypes,
                                      bool &found,
                                      bool intrinsics = false)
{
    unsigned int depth = 0;
    for (FVM_Object *cursor = object; cursor; cursor = prototypes ? cursor->prototype : 0)
    {
        if (++depth > FVM_PROTOTYPE_MAX_DEPTH)
        {
            return FVM_ERROR;
        }
        int own = 0;
        FVM_Result status = fvm_object_has_own_bytes(c, cursor, name.data(), name.size(), &own);
        if (status != FVM_OK)
        {
            return status;
        }
        if (own || (intrinsics && fvm_avm1_movieclip_path_kind(c, cursor, name) != FVM_SCOPE_PATH_NONE))
        {
            found = true;
            return FVM_OK;
        }
    }
    return FVM_OK;
}


/* Declare in the nearest appropriate local/With object, preserving qualified top-level rules. */
extern "C" FVM_Result fvm_exec_variable_declare(FVM_Context *c,
                                                FVM_Object *scope,
                                                FVM_Object *activation,
                                                FVM_Object *target,
                                                FVM_Object *receiver,
                                                int initialized,
                                                const FVM_Value *name,
                                                const FVM_Value *value)
{
    if (!c || (scope && scope->owner != c) || (activation && activation->owner != c) ||
        (target && target->owner != c) || (receiver && receiver->owner != c) || fvm_validate_value(c, name) != FVM_OK ||
        (initialized && fvm_validate_value(c, value) != FVM_OK))
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        FVM_ExecutionRoots roots(c, FVM_VARIABLE_DECLARE_ROOT_COUNT);
        FVM_Object *objects[] = {scope, activation, target, receiver};
        const unsigned int slots[] = {FVM_VARIABLE_DECLARE_SCOPE_ROOT,
                                      FVM_VARIABLE_DECLARE_ACTIVATION_ROOT,
                                      FVM_VARIABLE_DECLARE_TARGET_ROOT,
                                      FVM_VARIABLE_DECLARE_THIS_ROOT};
        for (size_t i = 0; i < sizeof(objects) / sizeof(objects[0]); ++i)
        {
            if (objects[i])
            {
                roots.values[slots[i]].type = FVM_VALUE_OBJECT;
                roots.values[slots[i]].object = objects[i];
            }
        }
        roots.values[FVM_VARIABLE_DECLARE_NAME_ROOT] = *name;
        if (initialized)
        {
            roots.values[FVM_VARIABLE_DECLARE_VALUE_ROOT] = *value;
        }
        FVM_Value &text = roots.values[FVM_VARIABLE_DECLARE_TEXT_ROOT];
        FVM_Result status = fvm_exec_to_string(c, &roots.values[FVM_VARIABLE_DECLARE_NAME_ROOT], &text);
        if (status != FVM_OK)
        {
            return status;
        }
        if (c->swf_loaded && !fvm_string_size(text))
        {
            return FVM_OK;
        }
        FVM_Object *destination = target ? target : c->global;
        bool qualified = false;
        for (size_t at = 0; at < fvm_string_size(text); ++at)
        {
            if (text.string[at] == FVM_REFERENCE_PATH_DOT || text.string[at] == FVM_REFERENCE_PATH_COLON)
            {
                qualified = true;
            }
        }
        if (c->swf_loaded && !activation && qualified)
        {
            if (!initialized)
            {
                FVM_Value &existing = roots.values[FVM_VARIABLE_DECLARE_EXISTING_ROOT];
                status = fvm_exec_get_variable(c, scope, destination, receiver, 0, &text, &existing);
                /* Qualified DefineLocal2 tests the read value, including getters, rather
                 * than membership. An existing undefined value still receives a write. */
                if (status != FVM_OK || existing.type != FVM_VALUE_UNDEFINED)
                {
                    return status;
                }
            }
            return fvm_exec_variable_set(
                c, scope, destination, receiver, &text, &roots.values[FVM_VARIABLE_DECLARE_VALUE_ROOT]);
        }
        fvm_detail::string key;
        status = fvm_property_key(text.string, fvm_string_size(text), key);
        if (status != FVM_OK)
        {
            return status;
        }
        if (!c->swf_loaded && activation)
        {
            if (!initialized && activation->properties.find(key) != activation->properties.end())
            {
                return FVM_OK;
            }
            FVM_Value stored = roots.values[FVM_VARIABLE_DECLARE_VALUE_ROOT];
            if (stored.type == FVM_VALUE_STRING)
            {
                status = fvm_value_to_string(c, &stored, &roots.values[FVM_VARIABLE_DECLARE_VALUE_ROOT]);
                if (status != FVM_OK)
                {
                    return status;
                }
                roots.values[FVM_VARIABLE_DECLARE_VALUE_ROOT].metatable = stored.metatable;
            }
            activation->properties[key] = roots.values[FVM_VARIABLE_DECLARE_VALUE_ROOT];
            return FVM_OK;
        }
        FVM_Object *current = c->swf_loaded ? (scope ? scope : activation) : 0;
        if (!initialized)
        {
            FVM_Object *object = current ? current : destination;
            bool prototypes = !current;
            if (current && current->resource_kind == FVM_RESOURCE_WITH_SCOPE)
            {
                if (!current->resource_result_valid || current->resource_result.type != FVM_VALUE_OBJECT)
                {
                    return FVM_INVALID_ARGUMENT;
                }
                object = current->resource_result.object;
                prototypes = true;
            }
            bool found = false;
            status = declaration_present(c, object, key, prototypes, found, true);
            if (status != FVM_OK || found)
            {
                return status;
            }
        }
        bool local_destination = false;
        unsigned int depth = 0;
        for (FVM_Object *node = current; node; node = node->prototype)
        {
            if (++depth > FVM_PROTOTYPE_MAX_DEPTH)
            {
                return FVM_ERROR;
            }
            if (node->resource_kind != FVM_RESOURCE_WITH_SCOPE)
            {
                destination = node;
                local_destination = true;
                break;
            }
            if (!node->resource_result_valid || node->resource_result.type != FVM_VALUE_OBJECT)
            {
                return FVM_INVALID_ARGUMENT;
            }
            bool own = false;
            status = declaration_present(c, node->resource_result.object, key, false, own);
            if (status != FVM_OK)
            {
                return status;
            }
            if (own)
            {
                destination = node->resource_result.object;
                break;
            }
        }
        if (local_destination)
        {
            bool own = false;
            status = declaration_present(c, destination, key, false, own);
            if (status != FVM_OK)
            {
                return status;
            }
            /* Lexical parents are scope links, not inherited setters of a new local cell. */
            if (!own || key == fvm_detail::string("__proto__"))
            {
                return fvm_base_store_data(c, destination, key, roots.values[FVM_VARIABLE_DECLARE_VALUE_ROOT], false);
            }
        }
        return fvm_object_set_bytes(
            c, destination, key.data(), key.size(), &roots.values[FVM_VARIABLE_DECLARE_VALUE_ROOT]);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
