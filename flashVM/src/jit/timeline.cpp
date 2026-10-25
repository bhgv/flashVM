/* FlashVM: native timeline controls preserve selected targets and shared frame semantics. */
#include "internal.hpp"
#include "timeline_constants.h"
#include "../avm1/operand_stack.hpp"
#include "../avm1/operand_constants.h"
#include "flashvm/movieclip.h"

namespace
{ /* Missing selections are no-ops within a timeline; a context without a timeline is unsupported. */


FVM_Result
apply_frame(FVM_Context *c, FVM_Object *target, FVM_ExecutionFrame command, const FVM_Value *frame, bool playing)
{
    if (!target || target->resource_kind != FVM_RESOURCE_MOVIECLIP)
    {
        return c->avm1_base_clip ? FVM_OK : FVM_UNSUPPORTED;
    }
    FVM_Result status = fvm_exec_frame_control(c, target, command, frame, playing);
    return status == FVM_NOT_FOUND ? FVM_OK : status;
}


/* Resolve a dynamic seek's physical target only after frame conversion has completed. */
FVM_Result dynamic_target(FVM_JitInvocation *s, FVM_Object **out)
{
    FVM_Context *c = s->context;
    *out = s->frame->receiver;
    if (!c->avm1_target_active || !c->avm1_base_clip)
    {
        return FVM_OK;
    }
    if (c->avm1_target_clip)
    {
        *out = c->avm1_target_clip;
        return FVM_OK;
    }
    FVM_Value root;
    FVM_Result status = fvm_value_string(&root, "/");
    return status == FVM_OK ? fvm_movieclip_resolve_path(c, c->avm1_base_clip, root, out) : status;
}
}


/* Execute a fixed next/previous/play/stop command without replaceable script method dispatch. */
FVM_Result fvm_jit_frame_control(FVM_JitInvocation *s, unsigned int command)
{
    return apply_frame(s->context, s->context->avm1_target_clip, (FVM_ExecutionFrame)command, 0, false);
}


/* [GC:] Adopt a code-owned frame/label in temporary roots without touching the operand stack. */
FVM_Result fvm_jit_frame_literal(FVM_JitInvocation *s, unsigned int index)
{
    FVM_Context *c = s->context;
    FVM_Object *target = c->avm1_target_clip;
    if (index >= s->code->constants.size())
    {
        return FVM_INVALID_ARGUMENT;
    }


    FVM_ExecutionRoots roots(c, FVM_AVM1_TIMELINE_REQUEST_ROOT_COUNT);
    FVM_Value &frame = roots.values[FVM_AVM1_TIMELINE_REQUEST_ROOT];
    const FVM_JitConstant &constant = s->code->constants[index];
    frame = constant.value;
    FVM_Result status = FVM_OK;
    if (frame.type == FVM_VALUE_STRING)
    {
        status = fvm_value_string_bytes(&frame, constant.text.c_str(), constant.text.size());
        if (status == FVM_OK)
        {
            status = fvm_native_result(c, FVM_OK, &frame);
        }
    }
    return status == FVM_OK ? apply_frame(c, target, FVM_EXEC_GOTO_FRAME, &frame, false) : status;
}


/* [GC:] Retain the input and converted request through coercion and post-conversion target lookup. */
FVM_Result fvm_jit_frame_value(FVM_JitInvocation *s, unsigned int policy)
{
    FVM_Context *c = s->context;
    FVM_Result status = fvm_avm1_require_value(c->swf_loaded, *s->stack);
    if (status != FVM_OK)
    {
        return status;
    }


    FVM_ExecutionRoots roots(c, FVM_AVM1_TIMELINE_REQUEST_ROOT_COUNT);
    FVM_Value &frame = roots.values[FVM_AVM1_TIMELINE_REQUEST_ROOT];
    status = fvm_exec_frame_request(
        c, &s->stack->back(), (policy & FVM_JIT_FRAME_WITH_BIAS) != 0, policy >> FVM_JIT_FRAME_BIAS_SHIFT, &frame);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_Object *target = 0;
    status = dynamic_target(s, &target);
    s->stack->pop_back();
    return status == FVM_OK ? apply_frame(c, target, FVM_EXEC_GOTO_FRAME, &frame, (policy & FVM_JIT_FRAME_PLAY) != 0)
                            : status;
}
