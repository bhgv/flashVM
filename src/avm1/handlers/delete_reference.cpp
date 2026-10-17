/* FlashVM: complete Delete2 operand handling over explicit managed scopes and values. */
#include "support.hpp"
#include "../scope_keywords.hpp"
#include "delete_reference_constants.h"
#include "reference_path_constants.h"
#include <cstring>
/* Resolve a variable-valued prefix under SWF version rules, then delete its final member. */
extern "C" FVM_Result fvm_exec_delete_reference(FVM_Context *c, FVM_Object *scope,
                                                FVM_Object *target, FVM_Object *receiver,
                                                const FVM_Value *name, FVM_Value *out)
{
    if (!c || !out || (scope && scope->owner != c) || (target && target->owner != c) ||
        (receiver && receiver->owner != c) || fvm_validate_value(c, name) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_DELETE_REFERENCE_ROOT_COUNT);
        FVM_Object *objects[] = {scope, target, receiver};
        const unsigned int slots[] = {FVM_DELETE_REFERENCE_SCOPE_ROOT,
                                      FVM_DELETE_REFERENCE_TARGET_ROOT,
                                      FVM_DELETE_REFERENCE_THIS_ROOT};
        for (size_t i = 0; i < sizeof(objects) / sizeof(objects[0]); ++i)
            if (objects[i])
            {
                roots.values[slots[i]].type = FVM_VALUE_OBJECT;
                roots.values[slots[i]].object = objects[i];
            }
        roots.values[FVM_DELETE_REFERENCE_NAME_ROOT] = *name;
        FVM_Value &text = roots.values[FVM_DELETE_REFERENCE_TEXT_ROOT];
        FVM_Result status =
            fvm_exec_to_string(c, &roots.values[FVM_DELETE_REFERENCE_NAME_ROOT], &text);
        if (status != FVM_OK)
            return status;
        size_t size = fvm_string_size(text), last = size;
        unsigned int segments = 1;
        for (size_t at = 0; at < size; ++at)
            if (text.string[at] == FVM_REFERENCE_PATH_DOT ||
                text.string[at] == FVM_REFERENCE_PATH_COLON)
            {
                if (++segments > FVM_REFERENCE_PATH_MAX_SEGMENTS)
                    return FVM_ERROR;
                last = at;
            }
        if (last == size)
            return fvm_exec_scope_delete(c, scope, target, &text, out);
        FVM_Value &prefix = roots.values[FVM_DELETE_REFERENCE_PREFIX_ROOT];
        FVM_Value &member = roots.values[FVM_DELETE_REFERENCE_MEMBER_ROOT];
        status = fvm_value_string_bytes(&prefix, text.string, last);
        if (status == FVM_OK)
            status = fvm_value_string_bytes(&member, text.string + last + 1, size - last - 1);
        if (status != FVM_OK)
            return status;
        bool modern = !c->swf_loaded || c->swf_info.version >= FVM_SWF_VERSION_5;
        FVM_Value &base = roots.values[FVM_DELETE_REFERENCE_BASE_ROOT];
        FVM_Value &path = roots.values[FVM_DELETE_REFERENCE_PATH_ROOT];
        int handled = 0, present = 0;
        status =
            fvm_exec_variable_path(c, scope, target, receiver, &prefix, &base, &handled, &present);
        if (status == FVM_OK && !handled)
        {
            if (modern && receiver && last == sizeof("this") - 1 &&
                !std::memcmp(prefix.string, "this", sizeof("this") - 1))
            {
                /* A qualified Delete2 reads the this cell before selecting its member. */
                if (!c->script_this_undefined)
                {
                    base.type = FVM_VALUE_OBJECT;
                    base.object = receiver;
                }
            }
            else
            {
                fvm_value_string(&path, "");
                status = fvm_exec_scope_member(c, scope, target, receiver, &path, &prefix, &base,
                                               &present);
                if (status == FVM_OK && !present &&
                    fvm_avm1_global_keyword(
                        c, fvm_detail::string(prefix.string, fvm_string_size(prefix))))
                {
                    base.type = FVM_VALUE_OBJECT;
                    base.object = c->global;
                }
            }
        }
        return status == FVM_OK ? fvm_exec_delete_member(c, &base, &member, out) : status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
