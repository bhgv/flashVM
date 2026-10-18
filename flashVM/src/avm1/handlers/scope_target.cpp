/* FlashVM: object-path lookup across explicit managed lexical and display scopes. */
#include "support.hpp"
#include "scope_target_constants.h"
/* Retry complete paths, rather than committing to a partially resolved inner prefix. */
extern "C" FVM_Result fvm_exec_scope_target(FVM_Context *c, FVM_Object *scope, FVM_Object *target,
                                            FVM_Object *receiver, const FVM_Value *path,
                                            FVM_Value *out)
{
    if (!c || !out || (scope && scope->owner != c) || (target && target->owner != c) ||
        (receiver && receiver->owner != c) || fvm_validate_value(c, path) != FVM_OK ||
        path->type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_SCOPE_TARGET_ROOT_COUNT);
        if (scope)
        {
            roots.values[FVM_SCOPE_TARGET_SCOPE_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_SCOPE_TARGET_SCOPE_ROOT].object = scope;
        }
        if (target)
        {
            roots.values[FVM_SCOPE_TARGET_TARGET_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_SCOPE_TARGET_TARGET_ROOT].object = target;
        }
        roots.values[FVM_SCOPE_TARGET_PATH_ROOT] = *path;
        if (receiver)
        {
            roots.values[FVM_SCOPE_TARGET_THIS_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_SCOPE_TARGET_THIS_ROOT].object = receiver;
        }
        FVM_Object *display_root = target ? target : c->global;
        FVM_Value absolute;
        FVM_Result status = fvm_value_string(&absolute, "/");
        if (status == FVM_OK)
            status = fvm_movieclip_resolve_path(c, display_root, absolute, &display_root);
        if (status != FVM_OK)
            return status;
        roots.values[FVM_SCOPE_TARGET_DISPLAY_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_SCOPE_TARGET_DISPLAY_ROOT].object = display_root;
        unsigned int depth = 0;
        for (FVM_Object *node = scope; node; node = node->prototype)
        {
            if (++depth > FVM_PROTOTYPE_MAX_DEPTH)
                return FVM_ERROR;
            roots.values[FVM_SCOPE_TARGET_CURRENT_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_SCOPE_TARGET_CURRENT_ROOT].object = node;
            FVM_Object *start = node;
            if (node->resource_kind == FVM_RESOURCE_WITH_SCOPE)
            {
                if (!node->resource_result_valid || node->resource_result.type != FVM_VALUE_OBJECT)
                    return FVM_INVALID_ARGUMENT;
                start = node->resource_result.object;
            }
            FVM_Object *resolved = 0;
            status = fvm_movieclip_resolve_path(c, start, roots.values[FVM_SCOPE_TARGET_PATH_ROOT],
                                                &resolved, display_root, receiver);
            if (status != FVM_OK)
                return status;
            if (resolved)
            {
                fvm_value_undefined(out);
                out->type = FVM_VALUE_OBJECT;
                out->object = resolved;
                return FVM_OK;
            }
        }
        FVM_Object *resolved = 0;
        if (target && target != c->global)
            status = fvm_movieclip_resolve_path(c, target, roots.values[FVM_SCOPE_TARGET_PATH_ROOT],
                                                &resolved, display_root, receiver);
        if (status == FVM_OK && !resolved)
            status =
                fvm_movieclip_resolve_path(c, c->global, roots.values[FVM_SCOPE_TARGET_PATH_ROOT],
                                           &resolved, display_root, receiver);
        if (status != FVM_OK)
            return status;
        fvm_value_undefined(out);
        if (resolved)
        {
            out->type = FVM_VALUE_OBJECT;
            out->object = resolved;
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
