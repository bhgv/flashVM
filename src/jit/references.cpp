/* FlashVM: native reference bridges preserve interpreter scope selection and rooted operands. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"
#include "../avm1/operand_constants.h"
#include "flashvm/movieclip.h"

namespace
{
/* Match the interpreter's selected target, including root recovery after a missing SetTarget. */
FVM_Result target(FVM_JitInvocation *s, FVM_Object **out)
{
    FVM_Context *c = s->context;
    *out = s->frame->receiver;
    if (!c->avm1_target_active || !c->avm1_base_clip)
        return FVM_OK;
    if (c->avm1_target_clip)
    {
        *out = c->avm1_target_clip;
        return FVM_OK;
    }
    FVM_Value root;
    FVM_Result status = fvm_value_string(&root, "/");
    return status == FVM_OK ? fvm_movieclip_resolve_path(c, c->avm1_base_clip, root, out) : status;
}
/* Return the current lexical object; the prepared activation retains this chain. */
FVM_Object *scope(FVM_Context *c)
{
    return c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back();
}
/* Writes/declarations use global fallback for a receiver that is not a MovieClip. */
FVM_Object *destination(FVM_Context *c, FVM_Object *selected)
{
    return selected && selected->resource_kind == FVM_RESOURCE_MOVIECLIP ? selected : c->global;
}
/* Retain the interpreter's explicit script-this independently from the selected target. */
FVM_Object *receiver(FVM_JitInvocation *s)
{
    return s->context->script_this ? s->context->script_this : s->frame->receiver;
}
} // namespace

/* [GC:] Keep name/value in the invocation's rooted vector through conversions and setters. */
FVM_Result fvm_jit_set_variable(FVM_JitInvocation *s, unsigned int)
{
    FVM_Context *c = s->context;
    FVM_Result status = fvm_avm1_require_operands(c->swf_loaded, *s->stack,
                                                FVM_AVM1_VARIABLE_WRITE_OPERAND_COUNT);
    if (status != FVM_OK)
        return status;
    size_t first = s->stack->size() - FVM_AVM1_VARIABLE_WRITE_OPERAND_COUNT;
    if (!c->swf_loaded && (*s->stack)[first].type != FVM_VALUE_STRING)
        return FVM_ERROR;
    FVM_Object *selected = 0;
    status = target(s, &selected);
    if (status == FVM_OK)
        status = fvm_exec_variable_set(c, scope(c), destination(c, selected), receiver(s),
                                       &(*s->stack)[first], &s->stack->back());
    if (status == FVM_OK)
        s->stack->resize(first);
    return status;
}

/* [GC:] Prepared activation identity and operands survive collecting declaration callbacks. */
FVM_Result fvm_jit_declare_variable(FVM_JitInvocation *s, unsigned int initialized)
{
    FVM_Context *c = s->context;
    size_t count = initialized ? FVM_AVM1_DECLARE_INITIALIZED_OPERANDS
                               : FVM_AVM1_DECLARE_ONLY_OPERANDS;
    FVM_Result status = fvm_avm1_require_operands(c->swf_loaded, *s->stack, count);
    if (status != FVM_OK)
        return status;
    size_t first = s->stack->size() - count;
    FVM_Object *selected = 0;
    status = target(s, &selected);
    if (status == FVM_OK)
        status = fvm_exec_variable_declare(c, scope(c), s->frame->activation,
                                           destination(c, selected), receiver(s), initialized,
                                           &(*s->stack)[first], initialized ? &s->stack->back() : 0);
    if (status == FVM_OK)
        s->stack->resize(first);
    return status;
}

/* [GC:] Do not pop owner/key/value until the shared member service has completed its callbacks. */
FVM_Result fvm_jit_member(FVM_JitInvocation *s, unsigned int write)
{
    size_t count = write ? FVM_AVM1_SET_MEMBER_OPERANDS : FVM_AVM1_GET_MEMBER_OPERANDS;
    FVM_Result status = fvm_avm1_require_operands(s->context->swf_loaded, *s->stack, count);
    if (status != FVM_OK)
        return status;
    size_t first = s->stack->size() - count;
    FVM_Value result;
    status = fvm_exec_member(s->context, write, &(*s->stack)[first],
                             &(*s->stack)[first + FVM_AVM1_MEMBER_NAME_OFFSET],
                             write ? &(*s->stack)[first + FVM_AVM1_MEMBER_VALUE_OFFSET] : 0, &result);
    if (status == FVM_OK)
    {
        if (write)
            s->stack->resize(first);
        else
        {
            (*s->stack)[first] = result;
            s->stack->pop_back();
        }
    }
    return status;
}

/* [GC:] Preserve explicit owner/key until deletion returns its versioned Boolean result. */
FVM_Result fvm_jit_delete_member(FVM_JitInvocation *s, unsigned int)
{
    FVM_Result status = fvm_avm1_require_operands(s->context->swf_loaded, *s->stack,
                                                FVM_AVM1_DELETE_MEMBER_OPERANDS);
    if (status != FVM_OK)
        return status;
    size_t first = s->stack->size() - FVM_AVM1_DELETE_MEMBER_OPERANDS;
    FVM_Value result;
    status = fvm_exec_delete_member(s->context, &(*s->stack)[first], &s->stack->back(), &result);
    if (status == FVM_OK)
    {
        (*s->stack)[first] = result;
        s->stack->pop_back();
    }
    return status;
}

/* [GC:] Qualified reference coercion and path getters run while name/environment stay rooted. */
FVM_Result fvm_jit_delete_reference(FVM_JitInvocation *s, unsigned int)
{
    FVM_Context *c = s->context;
    FVM_Result status = fvm_avm1_require_value(c->swf_loaded, *s->stack);
    if (status != FVM_OK)
        return status;
    FVM_Object *selected = 0;
    FVM_Value result;
    status = target(s, &selected);
    if (status == FVM_OK)
        status = fvm_exec_delete_reference(c, scope(c), selected, receiver(s), &s->stack->back(),
                                           &result);
    if (status == FVM_OK)
        s->stack->back() = result;
    return status;
}
