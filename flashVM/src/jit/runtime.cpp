/* FlashVM: native helper bridges to shared C services; temporaries are always rooted. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"
#include "flashvm/movieclip.h"
#include "../swf/record_constants.h"


/* Charge one source action, preserving dynamic polling cadence through loops. */
FVM_Result fvm_jit_checkpoint(FVM_JitInvocation *s, unsigned int offset)
{
    ++s->steps;
    return fvm_exec_checkpoint(
        s->context, 1, s->source_base + offset, (s->steps & FVM_JIT_POLL_MASK) == 1UL ? FVM_EXEC_POLL : 0);
}


/* Adopt immutable literal text before publishing it into managed state. */
FVM_Result fvm_jit_push(FVM_JitInvocation *s, unsigned int index)
{
    if (s->stack->size() >= FVM_JIT_MAX_STACK)
    {
        return FVM_UNSUPPORTED;
    }
    const FVM_JitConstant &constant = s->code->constants[index];
    FVM_Value value = constant.value;
    if (value.type == FVM_VALUE_STRING)
    {
        fvm_value_string_bytes(&value, constant.text.c_str(), constant.text.size());
        FVM_Result status = fvm_native_result(s->context, FVM_OK, &value);
        if (status != FVM_OK)
        {
            return status;
        }
    }
    s->stack->push_back(value);
    return FVM_OK;
}


/* Reuse the AVM1 numeric service, including coercion, metamethods and version rules. */
FVM_Result fvm_jit_numeric(FVM_JitInvocation *s, unsigned int operation)
{
    FVM_Result status = fvm_avm1_require_operands(s->context->swf_loaded, *s->stack, FVM_JIT_BINARY_VALUES);
    if (status != FVM_OK)
    {
        return status;
    }
    size_t first = s->stack->size() - FVM_JIT_BINARY_VALUES;
    FVM_Value result;
    status =
        fvm_exec_numeric(s->context, (FVM_ExecutionNumeric)operation, &(*s->stack)[first], &s->stack->back(), &result);
    if (status == FVM_OK)
    {
        (*s->stack)[first] = result;
        s->stack->pop_back();
    }
    return status;
}


/* Dynamic comparison/addition retains the same primitive conversion order as interpretation. */
FVM_Result fvm_jit_compare(FVM_JitInvocation *s, unsigned int operation)
{
    FVM_Result status = fvm_avm1_require_operands(s->context->swf_loaded, *s->stack, FVM_JIT_BINARY_VALUES);
    if (status != FVM_OK)
    {
        return status;
    }
    size_t first = s->stack->size() - FVM_JIT_BINARY_VALUES;
    FVM_Value result;
    status =
        fvm_exec_compare(s->context, (FVM_ExecutionCompare)operation, &(*s->stack)[first], &s->stack->back(), &result);
    if (status == FVM_OK)
    {
        (*s->stack)[first] = result;
        s->stack->pop_back();
    }
    return status;
}


/* Resolve the prepared activation and physical target through the shared variable service. */
FVM_Result fvm_jit_get_variable(FVM_JitInvocation *s, unsigned int)
{
    FVM_Context *c = s->context;
    FVM_Result status = fvm_avm1_require_value(c->swf_loaded, *s->stack);
    if (status != FVM_OK)
    {
        return status;
    }
    if (!c->swf_loaded && s->stack->back().type != FVM_VALUE_STRING)
    {
        return FVM_ERROR;
    }
    FVM_Object *target = s->frame->receiver;
    if (c->avm1_target_active && c->avm1_base_clip)
    {
        target = c->avm1_target_clip;
        if (!target)
        {
            FVM_Value root;
            fvm_value_string(&root, "/");
            status = fvm_movieclip_resolve_path(c, c->avm1_base_clip, root, &target);
            if (status != FVM_OK)
            {
                return status;
            }
        }
    }
    FVM_Value result;
    status = fvm_exec_get_variable(c,
                                   c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back(),
                                   target,
                                   c->swf_loaded && c->swf_info.version >= FVM_SWF_VERSION_5 && c->script_this
                                       ? c->script_this
                                       : s->frame->receiver,
                                   s->frame->activation != 0,
                                   &s->stack->back(),
                                   &result);
    if (status == FVM_OK)
    {
        s->stack->back() = result;
    }
    return status;
}


/* Return retains the operand in the rooted stack until the native entry has exited. */
FVM_Result fvm_jit_return(FVM_JitInvocation *s, unsigned int)
{
    FVM_Result status = fvm_avm1_require_value(s->context->swf_loaded, *s->stack);
    if (status == FVM_OK)
    {
        s->result = s->stack->back();
        s->returned = true;
    }
    return status;
}


/* Project and pop branch truth using the versioned C operation. */
FVM_Result fvm_jit_condition(FVM_JitInvocation *s, unsigned int)
{
    FVM_Result status = fvm_avm1_require_value(s->context->swf_loaded, *s->stack);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_Value truth;
    status = fvm_exec_to_boolean(s->context, &s->stack->back(), &truth);
    if (status == FVM_OK)
    {
        s->condition = truth.boolean != 0;
        s->stack->pop_back();
    }
    return status;
}


/* Return the prepared branch predicate; this helper cannot allocate or throw. */
FVM_Result fvm_jit_truth(FVM_JitInvocation *s, unsigned int)
{
    return (FVM_Result)(s->condition ? 1 : 0);
}


/* Implement small stack operations without exposing a bytecode format to native blocks. */
FVM_Result fvm_jit_stack(FVM_JitInvocation *s, unsigned int operation)
{
    if (operation == FVM_JIT_POP)
    {
        if (s->stack->empty())
        {
            return s->context->swf_loaded ? FVM_OK : FVM_ERROR;
        }
        s->stack->pop_back();
        return FVM_OK;
    }
    FVM_Result status = fvm_avm1_require_operands(
        s->context->swf_loaded, *s->stack, operation == FVM_JIT_SWAP ? FVM_JIT_BINARY_VALUES : FVM_JIT_UNARY_VALUES);
    if (status != FVM_OK)
    {
        return status;
    }
    if (operation == FVM_JIT_DUPLICATE)
    {
        if (s->stack->size() >= FVM_JIT_MAX_STACK)
        {
            return FVM_UNSUPPORTED;
        }
        FVM_Value value = s->stack->back();
        s->stack->push_back(value);
    }
    else
    {
        size_t first = s->stack->size() - FVM_JIT_BINARY_VALUES;
        FVM_Value value = (*s->stack)[first];
        (*s->stack)[first] = s->stack->back();
        s->stack->back() = value;
    }
    return FVM_OK;
}


/* Synthetic exits clear the helper status register without charging a nonexistent source Action. */
FVM_Result fvm_jit_finish(FVM_JitInvocation *, unsigned int)
{
    return FVM_OK;
}
