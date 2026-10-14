/* FlashVM: scoped path reads distinguish missing final members from undefined values. */
#include "support.hpp"
#include "scope_member_constants.h"
#include "../../core/object_root_scope.hpp"
#include "../../core/property_key.hpp"
/* Read only after a getter-free presence check; an undefined value still stops retries. */
static FVM_Result visit_member(FVM_Context *c, FVM_Object *start, FVM_Object *display_root,
                               FVM_Object *receiver, const FVM_Value &path,
                               const fvm_detail::string &name, bool prototypes, FVM_Value &result,
                               bool &found)
{
    FVM_Object *object = 0;
    FVM_Result status = fvm_movieclip_resolve_path(c, start, path, &object, display_root, receiver);
    if (status != FVM_OK || !object)
        return status;
    /* [GC:] The resolved object can be detached from every input graph by its final getter. */
    FVM_ObjectRootScope retained(c, object);
    unsigned int depth = 0;
    for (FVM_Object *cursor = object; cursor; cursor = prototypes ? cursor->prototype : 0)
    {
        if (++depth > FVM_PROTOTYPE_MAX_DEPTH)
            return FVM_ERROR;
        int own = 0;
        status = fvm_object_has_own_bytes(c, cursor, name.data(), name.size(), &own);
        if (status != FVM_OK)
            return status;
        if (own)
        {
            status = fvm_object_get_bytes(c, object, name.data(), name.size(), &result);
            if (status == FVM_OK)
                found = true;
            return status;
        }
    }
    return FVM_OK;
}
/* Retry complete prefix/member pairs; report presence independently from the returned value. */
extern "C" FVM_Result fvm_exec_scope_member(FVM_Context *c, FVM_Object *scope, FVM_Object *target,
                                            FVM_Object *receiver, const FVM_Value *path,
                                            const FVM_Value *name, FVM_Value *out, int *present)
{
    if (!c || !out || !present || (scope && scope->owner != c) || (target && target->owner != c) ||
        (receiver && receiver->owner != c) || fvm_validate_value(c, path) != FVM_OK ||
        path->type != FVM_VALUE_STRING || fvm_validate_value(c, name) != FVM_OK ||
        name->type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_SCOPE_MEMBER_ROOT_COUNT);
        FVM_Object *objects[] = {scope, target, receiver};
        const unsigned int slots[] = {FVM_SCOPE_MEMBER_SCOPE_ROOT, FVM_SCOPE_MEMBER_TARGET_ROOT,
                                      FVM_SCOPE_MEMBER_THIS_ROOT};
        for (size_t i = 0; i < sizeof(objects) / sizeof(objects[0]); ++i)
            if (objects[i])
            {
                roots.values[slots[i]].type = FVM_VALUE_OBJECT;
                roots.values[slots[i]].object = objects[i];
            }
        roots.values[FVM_SCOPE_MEMBER_PATH_ROOT] = *path;
        roots.values[FVM_SCOPE_MEMBER_NAME_ROOT] = *name;
        fvm_detail::string key;
        FVM_Result status = fvm_property_key(name->string, fvm_string_size(*name), key);
        if (status != FVM_OK)
            return status;
        FVM_Object *display_root = target ? target : c->global;
        FVM_Value absolute;
        fvm_value_string(&absolute, "/");
        status = fvm_movieclip_resolve_path(c, display_root, absolute, &display_root);
        if (status != FVM_OK)
            return status;
        roots.values[FVM_SCOPE_MEMBER_DISPLAY_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_SCOPE_MEMBER_DISPLAY_ROOT].object = display_root;
        FVM_Value result;
        fvm_value_undefined(&result);
        bool found = false;
        unsigned int depth = 0;
        for (FVM_Object *node = scope; node; node = node->prototype)
        {
            if (++depth > FVM_PROTOTYPE_MAX_DEPTH)
                return FVM_ERROR;
            roots.values[FVM_SCOPE_MEMBER_CURRENT_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_SCOPE_MEMBER_CURRENT_ROOT].object = node;
            FVM_Object *start = node;
            bool with = node->resource_kind == FVM_RESOURCE_WITH_SCOPE;
            if (with)
            {
                if (!node->resource_result_valid || node->resource_result.type != FVM_VALUE_OBJECT)
                    return FVM_INVALID_ARGUMENT;
                start = node->resource_result.object;
            }
            status = visit_member(
                c, start, display_root, receiver, roots.values[FVM_SCOPE_MEMBER_PATH_ROOT], key,
                with || fvm_string_size(roots.values[FVM_SCOPE_MEMBER_PATH_ROOT]) != 0, result,
                found);
            if (status != FVM_OK)
                return status;
            if (found)
                break;
        }
        if (!found && target && target != c->global)
            status =
                visit_member(c, target, display_root, receiver,
                             roots.values[FVM_SCOPE_MEMBER_PATH_ROOT], key, true, result, found);
        if (status == FVM_OK && !found)
            status =
                visit_member(c, c->global, display_root, receiver,
                             roots.values[FVM_SCOPE_MEMBER_PATH_ROOT], key, true, result, found);
        if (status != FVM_OK)
            return status;
        *out = result;
        *present = found ? 1 : 0;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
