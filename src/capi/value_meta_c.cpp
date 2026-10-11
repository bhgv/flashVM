#include "../core/runtime.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include "flashvm/function.h"
#include <new>

extern "C" FVM_Result fvm_value_set_metatable(FVM_Context *context, FVM_Value *value,
                                              FVM_Object *metatable)
{
    if (!context || fvm_validate_value(context, value) != FVM_OK ||
        (metatable && metatable->owner != context) || context->destroying || context->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    if (value->type == FVM_VALUE_OBJECT)
        return fvm_table_set_metatable(context, value->object, metatable);
    value->metatable = metatable;
    return FVM_OK;
}
extern "C" FVM_Result fvm_value_get_metatable(FVM_Context *context, const FVM_Value *value,
                                              FVM_Object **out)
{
    if (!context || !out || fvm_validate_value(context, value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    *out = value->type == FVM_VALUE_OBJECT ? value->object->metatable : value->metatable;
    return FVM_OK;
}
extern "C" FVM_Result fvm_value_meta_invoke(FVM_Context *context, const FVM_Value *value,
                                            const char *name, unsigned int count,
                                            const FVM_Value *args, FVM_Value *out)
{
    if (!context || !name || !*name || !out || (count && !args) || count > 65533 ||
        fvm_validate_value(context, value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < count; ++i)
        if (fvm_validate_value(context, args + i) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
    /* Snapshot owner before clearing out: callers may replace a value in place. */
    FVM_Value owner = *value;
    FVM_Object *meta = owner.type == FVM_VALUE_OBJECT ? owner.object->metatable : owner.metatable;
    try
    {
        for (unsigned int depth = 0; meta && depth < 1024; ++depth, meta = meta->metatable)
        {
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator hook =
                meta->metamethods.find(name);
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator script =
                meta->properties.find(name);
            FVM_Object *function = script != meta->properties.end() &&
                                           script->second.type == FVM_VALUE_OBJECT &&
                                           script->second.object->invocable.callback
                                       ? script->second.object
                                       : 0;
            if (hook == meta->metamethods.end() && !function)
                continue;
            fvm_detail::vector<FVM_Value> parameters;
            parameters.push_back(owner);
            FVM_Value defining;
            fvm_value_undefined(&defining);
            defining.type = FVM_VALUE_OBJECT;
            defining.object = meta;
            parameters.push_back(defining);
            for (unsigned int i = 0; i < count; ++i)
                parameters.push_back(args[i]);
            FVM_Value result;
            fvm_value_undefined(&result);
            FVM_Result status;
            if (hook == meta->metamethods.end())
                status = fvm_function_invoke(context, function,
                                             owner.type == FVM_VALUE_OBJECT ? owner.object : 0,
                                             count + 2, parameters.begin(), &result);
            else
            {
                FVM_NativeEntry callback = hook->second;
                FVM_NativeCallGuard guard(context);
                status =
                    callback.callback(context, owner.type == FVM_VALUE_OBJECT ? owner.object : 0,
                                      count + 2, parameters.begin(), &result, callback.userdata);
            }
            if (status == FVM_OK && fvm_validate_value(context, &result) != FVM_OK)
                status = FVM_INVALID_ARGUMENT;
            if (status == FVM_OK)
                *out = result;
            else
                fvm_value_undefined(out);
            return status;
        }
        fvm_value_undefined(out);
        return meta ? FVM_ERROR : FVM_NOT_FOUND;
    }
    catch (...)
    {
        fvm_value_undefined(out);
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" FVM_Result fvm_value_get_property(FVM_Context *context, const FVM_Value *value,
                                             const char *name, FVM_Value *out)
{
    if (!context || !name || !out || fvm_validate_value(context, value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    if (value->type == FVM_VALUE_OBJECT)
        return fvm_object_get(context, value->object, name, out);
    FVM_Value key;
    fvm_value_string(&key, name);
    FVM_Result result = fvm_value_meta_invoke(context, value, "__index", 1, &key, out);
    if (result == FVM_NOT_FOUND)
    {
        if (context->standard_library &&
            (value->type == FVM_VALUE_STRING || value->type == FVM_VALUE_NUMBER ||
             value->type == FVM_VALUE_BOOLEAN))
        {
            const char *type = value->type == FVM_VALUE_STRING    ? "String"
                               : value->type == FVM_VALUE_BOOLEAN ? "Boolean"
                                                                  : "Number";
            FVM_Object *boxed = 0;
            result = fvm_construct(context, type, 1, value, &boxed);
            return result == FVM_OK ? fvm_object_get(context, boxed, name, out) : result;
        }
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return result;
}
extern "C" FVM_Result fvm_value_set_property(FVM_Context *context, const FVM_Value *value,
                                             const char *name, const FVM_Value *assigned)
{
    if (!context || !name || fvm_validate_value(context, value) != FVM_OK ||
        fvm_validate_value(context, assigned) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    if (value->type == FVM_VALUE_OBJECT)
        return fvm_object_set(context, value->object, name, assigned);
    FVM_Value args[2], ignored;
    fvm_value_string(args, name);
    args[1] = *assigned;
    FVM_Result result = fvm_value_meta_invoke(context, value, "__newindex", 2, args, &ignored);
    return result == FVM_NOT_FOUND ? FVM_UNSUPPORTED : result;
}
