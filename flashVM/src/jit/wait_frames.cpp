/* FlashVM: native frame-availability predicates feed compiler-resolved record-count branches. */
#include "internal.hpp"


/* Query a direct UI16 frame without changing the operand vector. */
FVM_Result fvm_jit_wait_direct(FVM_JitInvocation *s, unsigned int frame)
{
    FVM_Value request, available;
    fvm_value_number(&request, frame);
    FVM_Result status = fvm_exec_frame_loaded(s->context, FVM_EXEC_FRAME_CHECK_DIRECT, &request, &available);
    if (status == FVM_OK)
    {
        s->condition = !available.boolean;
    }
    return status;
}


/* [GC:] Keep a supplied request on the rooted operand vector through collecting/throwing coercion. */
FVM_Result fvm_jit_wait_value(FVM_JitInvocation *s, unsigned int)
{
    FVM_Value request, available;
    fvm_value_undefined(&request);
    if (s->stack->empty())
    {
        if (!s->context->swf_loaded)
        {
            return FVM_ERROR;
        }
    }
    else
    {
        request = s->stack->back();
    }
    FVM_Result status = fvm_exec_frame_loaded(s->context, FVM_EXEC_FRAME_CHECK_VALUE, &request, &available);
    if (status == FVM_OK)
    {
        if (!s->stack->empty())
        {
            s->stack->pop_back();
        }
        s->condition = !available.boolean;
    }
    return status;
}
