/* FlashVM: value-returning native construction for loaded AVM1 NewObject/NewMethod. */
#include "construction.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/value_vector_root_scope.hpp"
#include "../core/native_call_guard.hpp"
#include "../core/exception_internal.hpp"
#include "flashvm/function.h"
#include <new>


/* [GC:] Preserve constructor/arguments until callback results and string views are validated. */
FVM_Result fvm_function_construct_value(
    FVM_Context *c, FVM_Object *function, unsigned int count, const FVM_Value *args, FVM_Value *out)
{
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_undefined(out);
    if (!c || !function || function->owner != c || (count && !args))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!c->swf_loaded || !function->constructable.callback)
    {
        FVM_Object *instance = 0;
        FVM_Result status = fvm_function_construct(c, function, count, args, &instance);
        if (status == FVM_OK)
        {
            out->type = FVM_VALUE_OBJECT;
            out->object = instance;
        }
        return status;
    }
    for (unsigned int i = 0; i < count; ++i)
    {
        if (fvm_validate_value(c, args + i) != FVM_OK)
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    try
    {


        FVM_ObjectRootScope function_root(c, function);
        fvm_detail::vector<FVM_Value> arguments;


        FVM_ValueVectorRootScope argument_roots(c, arguments);
        arguments.reserve(count);
        for (unsigned int i = 0; i < count; ++i)
        {
            arguments.push_back(args[i]);
        }
        FVM_Value result;
        fvm_value_undefined(&result);
        FVM_NativeEntry entry = function->constructable;


        FVM_NativeCallGuard guard(c);
        FVM_Result status = fvm_exception_complete(
            c,
            c->global,
            fvm_native_result(
                c,
                entry.callback(c, c->global, count, count ? arguments.begin() : 0, &result, entry.userdata),
                &result));
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
