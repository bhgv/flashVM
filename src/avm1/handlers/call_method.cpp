/* FlashVM: value-based CallMethod/NewMethod semantics for interpreted and generated bodies. */
#include "support.hpp"
#include "call_method_constants.h"
#include "script_call.hpp"
#include "../../core/property_key.hpp"
#include "flashvm/function.h"
#include "flashvm/standard.h"
/* Invoke a member or construct its value with all operands retained across callbacks. */
extern "C" FVM_Result fvm_exec_call_method(FVM_Context *c, int construct,
                                           const FVM_Value *owner_value,
                                           const FVM_Value *name_value, FVM_Object *anonymous_this,
                                           unsigned int argc, const FVM_Value *argv, FVM_Value *out)
{
    if (!c || !out || c->destroying || c->gc_releasing ||
        fvm_validate_value(c, owner_value) != FVM_OK ||
        fvm_validate_value(c, name_value) != FVM_OK ||
        (anonymous_this && anonymous_this->owner != c) || argc > FVM_EXEC_MAX_METHOD_ARGUMENTS ||
        (argc && !argv) ||
        (!c->swf_loaded && name_value->type != FVM_VALUE_STRING &&
         name_value->type != FVM_VALUE_NULL && name_value->type != FVM_VALUE_UNDEFINED))
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < argc; ++i)
        if (fvm_validate_value(c, argv + i) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
    if (owner_value->type == FVM_VALUE_NULL || owner_value->type == FVM_VALUE_UNDEFINED)
    {
        if (construct && !c->swf_loaded)
            return FVM_INVALID_ARGUMENT;
        fvm_value_undefined(out);
        return FVM_OK;
    }
    try
    {
        FVM_ExecutionRoots roots(c, (size_t)argc + FVM_METHOD_ARGUMENT_ROOT_BASE);
        roots.values[FVM_METHOD_OWNER_ROOT] = *owner_value;
        roots.values[FVM_METHOD_NAME_ROOT] = *name_value;
        if (anonymous_this)
        {
            roots.values[FVM_METHOD_THIS_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_METHOD_THIS_ROOT].object = anonymous_this;
        }
        for (unsigned int i = 0; i < argc; ++i)
            roots.values[FVM_METHOD_ARGUMENT_ROOT_BASE + i] = argv[i];
        const FVM_Value *args = argc ? roots.values.begin() + FVM_METHOD_ARGUMENT_ROOT_BASE : 0;
        const FVM_Value &owner = roots.values[FVM_METHOD_OWNER_ROOT];
        FVM_Object *receiver = owner.type == FVM_VALUE_OBJECT ? owner.object : 0;
        if (!receiver)
        {
            const char *type = owner.type == FVM_VALUE_STRING    ? "String"
                               : owner.type == FVM_VALUE_BOOLEAN ? "Boolean"
                                                                 : "Number";
            FVM_Result status = fvm_construct(c, type, 1, &owner, &receiver);
            if (status != FVM_OK)
                return status;
        }
        /* [GC:] Retain the box before method lookup or a constructor getter can execute script. */
        roots.values[FVM_METHOD_BOX_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_METHOD_BOX_ROOT].object = receiver;
        fvm_detail::string name;
        const FVM_Value &source_name = roots.values[FVM_METHOD_NAME_ROOT];
        FVM_Value &text = roots.values[FVM_METHOD_TEXT_ROOT];
        if (source_name.type != FVM_VALUE_UNDEFINED &&
            (c->swf_loaded || source_name.type != FVM_VALUE_NULL))
        {
            FVM_Result converted = fvm_exec_to_string(c, &source_name, &text);
            if (converted != FVM_OK)
                return converted;
            converted = fvm_property_key(text.string, fvm_string_size(text), name);
            if (converted != FVM_OK)
                return converted;
        }
        FVM_NativeEntry native_method;
        FVM_Result lookup = fvm_lookup_native_method(receiver, name, &native_method);
        if (lookup != FVM_OK && lookup != FVM_NOT_FOUND && lookup != FVM_UNSUPPORTED)
            return lookup;
        FVM_Value result;
        fvm_value_undefined(&result);
        FVM_Result status;
        if (construct)
        {
            FVM_Value &function = roots.values[FVM_METHOD_FUNCTION_ROOT];
            if (name.empty())
            {
                function.type = FVM_VALUE_OBJECT;
                function.object = receiver;
                status = FVM_OK;
            }
            else
                status = fvm_object_get_bytes(c, receiver, name.data(), name.size(), &function);
            if (status != FVM_OK)
                return status;
            if (function.type != FVM_VALUE_OBJECT)
            {
                if (!c->swf_loaded)
                    return FVM_INVALID_ARGUMENT;
                *out = result;
                return FVM_OK;
            }
            FVM_Object *instance = 0;
            status = fvm_function_construct(c, function.object, argc, args, &instance);
            if (status == FVM_OK)
            {
                result.type = FVM_VALUE_OBJECT;
                result.object = instance;
            }
        }
        else if (name.empty())
            status = fvm_function_invoke(c, receiver, anonymous_this, argc, args, &result);
        else
        {
            fvm_detail::map<fvm_detail::string, FVM_ScriptFunction>::const_iterator script =
                c->script_functions.find(name);
            if (receiver->resource_kind != FVM_RESOURCE_SUPER && lookup == FVM_NOT_FOUND &&
                script != c->script_functions.end())
                status = fvm_avm1_call_script(c, script->first, script->second, argc, args,
                                              receiver, &result);
            else
                status = fvm_object_call_bytes(c, receiver, name.data(), name.size(), argc, args,
                                               &result);
        }
        if (status == FVM_OK)
            *out = result;
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
