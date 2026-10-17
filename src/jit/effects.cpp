/* FlashVM: native audio stopping, tracing, exceptions, elapsed time and bounded random values. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"
#include "flashvm/exception.h"
#include "flashvm/multimedia.h"
/* [GC:] The shared audio service stabilizes registrations and blocks collection during callbacks. */
FVM_Result fvm_jit_stop_sounds(FVM_JitInvocation *s, unsigned int)
{
    return fvm_audio_stop_all(s->context);
}
/* [GC:] Keep the trace input rooted until conversion and the host sink finish. */
FVM_Result fvm_jit_trace(FVM_JitInvocation *s, unsigned int)
{
    FVM_Result status = fvm_avm1_require_value(s->context->swf_loaded, *s->stack);
    if (status == FVM_OK)
        status = fvm_exec_trace(s->context, &s->stack->back());
    if (status == FVM_OK)
        s->stack->pop_back();
    return status;
}
/* [GC:] Publish the thrown value in the exception roots before removing its stack root. */
FVM_Result fvm_jit_throw(FVM_JitInvocation *s, unsigned int)
{
    FVM_Result status = fvm_avm1_require_value(s->context->swf_loaded, *s->stack);
    if (status != FVM_OK)
        return status;
    status = fvm_exception_raise(s->context, &s->stack->back());
    s->stack->pop_back();
    return status;
}
/* Reserve bounded stack storage before sampling the context-relative clock. */
FVM_Result fvm_jit_get_time(FVM_JitInvocation *s, unsigned int)
{
    if (s->stack->size() >= FVM_JIT_MAX_STACK)
        return FVM_UNSUPPORTED;
    FVM_Value result;
    fvm_value_undefined(&result);
    s->stack->push_back(result);
    FVM_Result status = fvm_exec_get_time(s->context, &result);
    if (status == FVM_OK)
        s->stack->back() = result;
    else
        s->stack->pop_back();
    return status;
}
/* [GC:] Retain the bound through collecting/throwing numeric conversions. */
FVM_Result fvm_jit_random_number(FVM_JitInvocation *s, unsigned int)
{
    FVM_Result status = fvm_avm1_require_value(s->context->swf_loaded, *s->stack);
    if (status != FVM_OK)
        return status;
    FVM_Value result;
    status = fvm_exec_random_number(s->context, &s->stack->back(), &result);
    if (status == FVM_OK)
        s->stack->back() = result;
    return status;
}
