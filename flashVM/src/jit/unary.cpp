/* FlashVM: native unary semantic calls with the same missing-operand and GC policy as AVM1. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"


/* [GC:] Keep the original input on the rooted vector through every collecting coercion. */
FVM_Result fvm_jit_unary(FVM_JitInvocation *s, unsigned int operation)
{
    FVM_Result status = fvm_avm1_require_value(s->context->swf_loaded, *s->stack);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_Value result;
    const FVM_Value *input = &s->stack->back();
    switch (operation)
    {
    case FVM_JIT_UNARY_INTEGER:
    case FVM_JIT_UNARY_NUMBER:
        status = fvm_exec_to_number(s->context, operation == FVM_JIT_UNARY_INTEGER, input, &result);
        break;
    case FVM_JIT_UNARY_STRING:
        status = fvm_exec_to_string(s->context, input, &result);
        break;
    case FVM_JIT_UNARY_TYPE:
        status = fvm_exec_type_of(s->context, input, &result);
        break;
    case FVM_JIT_UNARY_INCREMENT:
    case FVM_JIT_UNARY_DECREMENT:
        status = fvm_exec_step(s->context, operation == FVM_JIT_UNARY_DECREMENT, input, &result);
        break;
    case FVM_JIT_UNARY_NOT:
        status = fvm_exec_logical(s->context, FVM_EXEC_LOGICAL_NOT, input, 0, &result);
        break;
    default:
        return FVM_INVALID_ARGUMENT;
    }
    if (status == FVM_OK)
    {
        s->stack->back() = result;
    }
    return status;
}
