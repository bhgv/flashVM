/* FlashVM: native legacy comparisons and logical operators retain shared coercion policies. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"
#include "../avm1/operand_constants.h"
/* [GC:] Retain both operands through metamethod lookup and ordered numeric conversions. */
FVM_Result fvm_jit_legacy_compare(FVM_JitInvocation *s, unsigned int operation)
{
    FVM_Result status = fvm_avm1_require_operands(s->context->swf_loaded, *s->stack,
                                                FVM_AVM1_LEGACY_BINARY_OPERANDS);
    if (status != FVM_OK)
        return status;
    size_t first = s->stack->size() - FVM_AVM1_LEGACY_BINARY_OPERANDS;
    FVM_Value result;
    status = fvm_exec_legacy_compare(s->context, (FVM_ExecutionLegacyCompare)operation,
                                      &(*s->stack)[first], &s->stack->back(), &result);
    if (status == FVM_OK)
    {
        (*s->stack)[first] = result;
        s->stack->pop_back();
    }
    return status;
}
/* [GC:] Convert both rooted inputs left-to-right, without short-circuiting conversion. */
FVM_Result fvm_jit_logical(FVM_JitInvocation *s, unsigned int operation)
{
    FVM_Result status = fvm_avm1_require_operands(s->context->swf_loaded, *s->stack,
                                                FVM_AVM1_LEGACY_BINARY_OPERANDS);
    if (status != FVM_OK)
        return status;
    size_t first = s->stack->size() - FVM_AVM1_LEGACY_BINARY_OPERANDS;
    FVM_Value result;
    status = fvm_exec_logical(s->context, (FVM_ExecutionLogical)operation,
                               &(*s->stack)[first], &s->stack->back(), &result);
    if (status == FVM_OK)
    {
        (*s->stack)[first] = result;
        s->stack->pop_back();
    }
    return status;
}
