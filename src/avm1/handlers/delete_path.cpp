/* FlashVM: deletion through counted object or display paths rooted at an explicit value. */
#include "support.hpp"
#include "delete_path_constants.h"
#include "reference_path_constants.h"
/* Traverse object-valued prefixes; only the final property is deleted, without reading it. */
extern "C" FVM_Result fvm_exec_delete_path(FVM_Context *c, const FVM_Value *base,
                                           const FVM_Value *path, FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, base) != FVM_OK ||
        fvm_validate_value(c, path) != FVM_OK || path->type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_DELETE_PATH_ROOT_COUNT);
        roots.values[FVM_DELETE_PATH_CURRENT_ROOT] = *base;
        roots.values[FVM_DELETE_PATH_BASE_ROOT] = *base;
        roots.values[FVM_DELETE_PATH_TEXT_ROOT] = *path;
        const FVM_Value &text = roots.values[FVM_DELETE_PATH_TEXT_ROOT];
        size_t size = fvm_string_size(text), last = 0;
        bool qualified = false;
        unsigned int segments = 1;
        for (size_t at = 0; at < size; ++at)
            if (text.string[at] == FVM_REFERENCE_PATH_DOT ||
                text.string[at] == FVM_REFERENCE_PATH_COLON)
            {
                if (++segments > FVM_REFERENCE_PATH_MAX_SEGMENTS)
                    return FVM_ERROR;
                last = at;
                qualified = true;
            }
        if (qualified)
            for (size_t at = 0; at < last; ++at)
                if (text.string[at] == FVM_REFERENCE_PATH_SLASH)
                {
                    FVM_Value &current = roots.values[FVM_DELETE_PATH_CURRENT_ROOT];
                    if (current.type != FVM_VALUE_OBJECT)
                    {
                        fvm_value_boolean(out, 0);
                        return FVM_OK;
                    }
                    FVM_Value &prefix = roots.values[FVM_DELETE_PATH_MEMBER_ROOT];
                    FVM_Result status = fvm_value_string_bytes(&prefix, text.string, last);
                    if (status != FVM_OK)
                        return status;
                    FVM_Object *resolved = 0;
                    status = fvm_movieclip_resolve_path(c, current.object, prefix, &resolved);
                    if (status != FVM_OK)
                        return status;
                    if (!resolved)
                    {
                        fvm_value_boolean(out, 0);
                        return FVM_OK;
                    }
                    current.object = resolved;
                    status =
                        fvm_value_string_bytes(&prefix, text.string + last + 1, size - last - 1);
                    return status == FVM_OK ? fvm_exec_delete_member(c, &current, &prefix, out)
                                            : status;
                }
        size_t first = 0;
        for (;;)
        {
            FVM_Value &current = roots.values[FVM_DELETE_PATH_CURRENT_ROOT];
            if (current.type != FVM_VALUE_OBJECT)
            {
                fvm_value_boolean(out, 0);
                return FVM_OK;
            }
            size_t end = first;
            while (end < size && text.string[end] != FVM_REFERENCE_PATH_DOT &&
                   text.string[end] != FVM_REFERENCE_PATH_COLON)
                ++end;
            FVM_Value &member = roots.values[FVM_DELETE_PATH_MEMBER_ROOT];
            FVM_Result status = fvm_value_string_bytes(&member, text.string + first, end - first);
            if (status != FVM_OK)
                return status;
            if (end == size)
                return fvm_exec_delete_member(c, &current, &member, out);
            FVM_Value &next = roots.values[FVM_DELETE_PATH_NEXT_ROOT];
            status = fvm_value_get_property_bytes(c, &current, member.string,
                                                  fvm_string_size(member), &next);
            if (status != FVM_OK)
                return status;
            current = next;
            fvm_value_undefined(&next);
            first = end + 1;
        }
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
