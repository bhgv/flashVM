/* FlashVM: native stage-quality, display-property and target-path bridges retain shared scene semantics. */
#include "internal.hpp"
#include "flashvm/stage.h"
#include "../avm1/operand_stack.hpp"
#include "../avm1/operand_constants.h"


/* [GC:] Change only scalar context policy through the shared nonallocating stage service. */
FVM_Result fvm_jit_toggle_quality(FVM_JitInvocation *s, unsigned int)
{
    return fvm_stage_toggle_quality(s->context);
}


/* [GC:] Preserve target/index/value through ordered coercion, resolution and property callbacks. */
FVM_Result fvm_jit_display_property(FVM_JitInvocation *s, unsigned int write)
{
    size_t count = write ? FVM_AVM1_DISPLAY_WRITE_OPERANDS : FVM_AVM1_DISPLAY_READ_OPERANDS;
    FVM_Result status = fvm_avm1_require_operands(s->context->swf_loaded, *s->stack, count);
    if (status != FVM_OK)
    {
        return status;
    }
    size_t first = s->stack->size() - count;
    FVM_Value result;
    status = fvm_exec_display_property(s->context,
                                       write != 0,
                                       &(*s->stack)[first],
                                       &(*s->stack)[first + FVM_AVM1_DISPLAY_INDEX_OFFSET],
                                       write ? &(*s->stack)[first + FVM_AVM1_DISPLAY_VALUE_OFFSET] : 0,
                                       &result);
    if (status == FVM_OK)
    {
        if (write)
        {
            s->stack->resize(first);
        }
        else
        {
            (*s->stack)[first] = result;
            s->stack->pop_back();
        }
    }
    return status;
}


/* [GC:] Retain the original input through boxing and native counted-path publication. */
FVM_Result fvm_jit_target_path(FVM_JitInvocation *s, unsigned int)
{
    FVM_Result status = fvm_avm1_require_value(s->context->swf_loaded, *s->stack);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_Value result;
    status = fvm_exec_target_path(s->context, &s->stack->back(), &result);
    if (status == FVM_OK)
    {
        s->stack->back() = result;
    }
    return status;
}
