/* FlashVM: native enumeration snapshots preserve scope lookup and operand publication. */
#include "internal.hpp"
#include "enumerate_constants.h"
#include "../avm1/operand_stack.hpp"
#include "../swf/record_constants.h"
#include "flashvm/movieclip.h"
namespace
{
/* Select the interpreter's physical target, recovering the root after a missing SetTarget. */
FVM_Result enumeration_target(FVM_Context *c, FVM_Object *fallback, FVM_Object **out)
{
    *out = fallback;
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
/* Resolve a counted binding without GetVariable's SWF4 MovieClip result suppression. */
FVM_Result enumeration_lookup(FVM_JitInvocation *s, const FVM_Value *name, FVM_Value *out)
{
    FVM_Context *c = s->context;
    FVM_Object *target = 0;
    FVM_Result status = enumeration_target(c, s->frame->receiver, &target);
    if (status != FVM_OK)
        return status;
    FVM_Object *scope = c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back();
    int handled = 0, present = 0;
    status = fvm_exec_variable_path(c, scope, target,
        c->script_this ? c->script_this : s->frame->receiver, name, out, &handled, &present);
    if (status != FVM_OK || handled)
        return status;
    FVM_Object *fallback = c->global;
    if (c->swf_loaded && c->avm1_base_clip)
        status = enumeration_target(c, c->avm1_base_clip, &fallback);
    FVM_Object *ignored = 0;
    if (status == FVM_OK)
        status = fvm_exec_variable_lookup(c, scope, target,
            c->swf_loaded && c->swf_info.version >= FVM_SWF_VERSION_5 && c->script_this
                ? c->script_this : s->frame->receiver,
            fallback, s->frame->activation != 0, name, out, &ignored, &present);
    return status;
}
}
/* [GC:] Retain input/name, resolved target and snapshot until bounded stack publication completes. */
FVM_Result fvm_jit_enumerate(FVM_JitInvocation *s, unsigned int named)
{
    FVM_Context *c = s->context;
    fvm_detail::vector<FVM_Value> &stack = *s->stack;
    FVM_Result status = fvm_avm1_require_value(c->swf_loaded, stack);
    if (status != FVM_OK)
        return status;
    FVM_ExecutionRoots roots(c, FVM_JIT_ENUMERATE_ROOT_COUNT);
    FVM_Value &name = roots.values[FVM_JIT_ENUMERATE_NAME_ROOT];
    FVM_Value &target = roots.values[FVM_JIT_ENUMERATE_TARGET_ROOT];
    FVM_Value &snapshot = roots.values[FVM_JIT_ENUMERATE_SNAPSHOT_ROOT];
    name = stack.back();
    target = name;
    if (named)
    {
        if (c->swf_loaded)
            status = fvm_exec_to_string(c, &stack.back(), &name);
        else if (name.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
        if (status == FVM_OK)
            status = enumeration_lookup(s, &name, &target);
    }
    if (status == FVM_OK)
        status = fvm_exec_enumerate(c, &target, &snapshot);
    if (status != FVM_OK)
        return status;
    size_t count = snapshot.object->indexed.size();
    if (stack.size() > FVM_JIT_MAX_STACK || count > FVM_JIT_MAX_STACK - stack.size())
        return FVM_UNSUPPORTED;
    stack.reserve(stack.size() + count);
    fvm_value_undefined(&stack.back());
    if (!c->swf_loaded)
        stack.back().type = FVM_VALUE_NULL;
    for (size_t i = count; i; --i)
        stack.push_back(snapshot.object->indexed.find((unsigned long)i - 1)->second);
    return FVM_OK;
}
