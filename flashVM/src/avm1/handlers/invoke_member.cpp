/* FlashVM: single-read named member calls with retained receiver and defining prototype. */
#include "support.hpp"
#include "../../standard/super.hpp"
#include "../../core/property_key.hpp"
#include "../../core/property_lookup.hpp"
#include "../../core/member_call_constants.h"
#include "invoke_member_constants.h"
#include "../../core/call_scope.hpp"
#include "flashvm/function.h"


/* Invoke an explicit object's counted member; missing/noncallable values yield undefined. */
extern "C" FVM_Result fvm_exec_invoke_member(FVM_Context *c,
                                             FVM_Object *receiver,
                                             const FVM_Value *name,
                                             unsigned int argc,
                                             const FVM_Value *argv,
                                             FVM_Value *out)
{
    if (!c || !receiver || receiver->owner != c || !out || fvm_validate_value(c, name) != FVM_OK ||
        name->type != FVM_VALUE_STRING || argc > FVM_EXEC_MAX_METHOD_ARGUMENTS || (argc && !argv))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (unsigned int i = 0; i < argc; ++i)
    {
        if (fvm_validate_value(c, argv + i) != FVM_OK)
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    try
    {


        FVM_ExecutionRoots roots(c, (size_t)argc + FVM_INVOKE_MEMBER_ARGUMENT_BASE);
        roots.values[FVM_INVOKE_MEMBER_RECEIVER_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_INVOKE_MEMBER_RECEIVER_ROOT].object = receiver;
        roots.values[FVM_INVOKE_MEMBER_NAME_ROOT] = *name;
        for (unsigned int i = 0; i < argc; ++i)
        {
            roots.values[FVM_INVOKE_MEMBER_ARGUMENT_BASE + i] = argv[i];
        }
        const FVM_Value &key = roots.values[FVM_INVOKE_MEMBER_NAME_ROOT];
        const FVM_Value *args = argc ? roots.values.begin() + FVM_INVOKE_MEMBER_ARGUMENT_BASE : 0;
        if (receiver->resource_kind == FVM_RESOURCE_SUPER)
        {
            FVM_Value result;
            fvm_value_undefined(&result);
            FVM_Result status;
            if (c->swf_loaded)
            {
                fvm_detail::string property;
                status = fvm_property_key(key.string, fvm_string_size(key), property);
                if (status == FVM_OK)
                {
                    status = fvm_super_invoke(c, receiver, property, argc, args, &result);
                }
            }
            else
            {
                status = fvm_object_call_bytes(c, receiver, key.string, fvm_string_size(key), argc, args, &result);
            }
            if (status == FVM_OK)
            {
                *out = result;
            }
            return status;
        }
        fvm_detail::string stored_name;
        FVM_Result normalized = fvm_property_key(key.string, fvm_string_size(key), stored_name);
        if (normalized != FVM_OK)
        {
            return normalized;
        }
        unsigned int depth = 0;
        unsigned int super_depth = FVM_MEMBER_CALL_MIN_SUPER_DEPTH;
        for (FVM_Object *cursor = receiver; cursor; cursor = cursor->prototype)
        {
            if (++depth > FVM_PROTOTYPE_MAX_DEPTH)
            {
                return FVM_ERROR;
            }
            int own = 0;
            FVM_Result status = fvm_object_has_own_bytes(c, cursor, key.string, fvm_string_size(key), &own);
            if (status != FVM_OK)
            {
                return status;
            }
            if (own && fvm_property_available(c, cursor, stored_name))
            {
                roots.values[FVM_INVOKE_MEMBER_SOURCE_ROOT].type = FVM_VALUE_OBJECT;
                roots.values[FVM_INVOKE_MEMBER_SOURCE_ROOT].object = cursor;
                if (depth > FVM_MEMBER_CALL_MIN_SUPER_DEPTH)
                {
                    super_depth = depth - 1;
                }
                break;
            }
        }
        FVM_Value &function = roots.values[FVM_INVOKE_MEMBER_FUNCTION_ROOT];
        FVM_Result status = fvm_object_get_bytes(c, receiver, key.string, fvm_string_size(key), &function);
        if (status != FVM_OK)
        {
            return status;
        }
        if (function.type != FVM_VALUE_OBJECT || !function.object->invocable.callback)
        {
            fvm_value_undefined(out);
            return FVM_OK;
        }
        FVM_Object *source = roots.values[FVM_INVOKE_MEMBER_SOURCE_ROOT].type == FVM_VALUE_OBJECT
                                 ? roots.values[FVM_INVOKE_MEMBER_SOURCE_ROOT].object
                                 : 0;


        /* [GC:] Keep the defining owner and pre-getter lookup depth independently;
         * loaded own/dynamic methods use the same minimum superclass depth. */
        FVM_CallScope call(
            c, 0, function.object, source, c->swf_loaded ? super_depth : FVM_CALL_SCOPE_INFER_SUPER_DEPTH);
        FVM_Value result;
        fvm_value_undefined(&result);
        status = fvm_function_invoke(c, function.object, receiver, argc, args, &result);
        if (status == FVM_OK)
        {
            *out = result;
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
