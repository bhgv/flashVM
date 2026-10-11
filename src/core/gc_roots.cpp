#include "runtime.hpp"
#include "flashvm/gc_roots.h"
#include <new>
#include <cstring>

struct FVM_GCRootScope
{
    FVM_Context *context;
    FVM_GCRootFrame frame;
    fvm_detail::vector<FVM_Value> values;
};
extern "C"
{
    FVM_Result fvm_gc_roots_begin(FVM_Context *context, FVM_GCRootScope **out)
    {
        if (!context || !out || context->gc_collecting)
            return FVM_INVALID_ARGUMENT;
        *out = 0;
        FVM_GCRootScope *scope = new (std::nothrow) FVM_GCRootScope;
        if (!scope)
            return FVM_OUT_OF_MEMORY;
        scope->context = context;
        scope->frame.previous = context->gc_frames;
        scope->frame.stack = &scope->values;
        scope->frame.locals = 0;
        scope->frame.receiver = 0;
        context->gc_frames = &scope->frame;
        *out = scope;
        return FVM_OK;
    }
    FVM_Result fvm_gc_roots_add(FVM_GCRootScope *scope, const FVM_Value *value)
    {
        if (!scope || !value || scope->context->gc_collecting)
            return FVM_INVALID_ARGUMENT;
        if (fvm_validate_value(scope->context, value) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
        try
        {
            scope->values.push_back(*value);
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }
    FVM_Result fvm_gc_roots_end(FVM_GCRootScope *scope)
    {
        if (!scope || scope->context->gc_frames != &scope->frame || scope->context->gc_collecting)
            return FVM_INVALID_ARGUMENT;
        scope->context->gc_frames = scope->frame.previous;
        delete scope;
        return FVM_OK;
    }
}
