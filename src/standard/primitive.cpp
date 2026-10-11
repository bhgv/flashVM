#include "primitive_state.hpp"
#include "../core/runtime.hpp"
#include "flashvm/function.h"
#include "flashvm/callable.h"
#include <new>
FVM_Result fvm_to_primitive(FVM_Context *c, const FVM_Value *input, bool string_hint,
                            FVM_Value *out)
{
    if (!c || !input || !out || fvm_validate_value(c, input) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    if (input->type != FVM_VALUE_OBJECT)
    {
        *out = *input;
        return FVM_OK;
    }
    if (c->primitive_depth >= 64)
        return FVM_UNSUPPORTED;
    try
    {
        fvm_detail::vector<FVM_Value> values;
        values.push_back(*input);
        FVM_GCRootFrame frame;
        frame.previous = c->gc_frames;
        frame.stack = &values;
        frame.locals = 0;
        frame.receiver = input->object;
        c->gc_frames = &frame;
        ++c->primitive_depth;
        struct Pop
        {
            FVM_Context *c;
            FVM_GCRootFrame *previous;
            ~Pop()
            {
                --c->primitive_depth;
                c->gc_frames = previous;
            }
        } pop;
        pop.c = c;
        pop.previous = frame.previous;
        const char *names[2] = {string_hint ? "toString" : "valueOf",
                                string_hint ? "valueOf" : "toString"};
        bool callable = false;
        for (unsigned int i = 0; i < 2; ++i)
        {
            FVM_Value method;
            FVM_Result r = fvm_object_get(c, input->object, names[i], &method);
            if (r != FVM_OK)
                return r;
            if (method.type != FVM_VALUE_OBJECT)
                continue;
            FVM_Value result;
            if (method.object->invocable.callback)
                r = fvm_function_invoke(c, method.object, input->object, 0, 0, &result);
            else if (method.object->callable.callback)
                r = fvm_callable_invoke(c, method.object, 0, 0, &result);
            else
                continue;
            callable = true;
            if (r != FVM_OK)
                return r;
            if (fvm_validate_value(c, &result) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
            if (result.type != FVM_VALUE_OBJECT)
            {
                *out = result;
                return FVM_OK;
            }
        }
        if (!callable && string_hint)
            return fvm_value_string(out, "[object Object]");
        return FVM_UNSUPPORTED;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
