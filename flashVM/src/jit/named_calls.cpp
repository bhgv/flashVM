/* FlashVM: native named calls share lexical/target lookup and rooted AVM1 construction. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"
#include "../avm1/operand_constants.h"
#include "../swf/record_constants.h"
#include "flashvm/movieclip.h"

namespace
{


/* Recover the selected timeline target, retaining the caller-specific fallback receiver. */
FVM_Result selected_target(FVM_Context *c, FVM_Object *fallback, FVM_Object **out)
{
    *out = fallback;
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


/* [GC:] Convert the name before the count while all inputs remain on the rooted operand vector. */
FVM_Result fvm_jit_call_named(FVM_JitInvocation *s, unsigned int construct)
{
    FVM_Context *c = s->context;
    fvm_detail::vector<FVM_Value> &stack = *s->stack;
    size_t fixed = construct ? FVM_AVM1_NEW_OBJECT_FIXED_OPERANDS : FVM_AVM1_NAMED_CALL_FIXED_OPERANDS;
    size_t argument = construct ? FVM_AVM1_NEW_OBJECT_FIRST_ARGUMENT : FVM_AVM1_NAMED_CALL_FIRST_ARGUMENT;
    FVM_Result status = fvm_avm1_require_operands(c->swf_loaded, stack, fixed);
    if (status != FVM_OK)
    {
        return status;
    }
    if (!c->swf_loaded && stack.back().type != FVM_VALUE_STRING)
    {
        return FVM_ERROR;
    }
    if (c->swf_loaded)
    {
        FVM_Value name;
        status = fvm_exec_to_string(c, &stack.back(), &name);
        if (status != FVM_OK)
        {
            return status;
        }
        stack.back() = name;
    }
    unsigned int argc = 0;
    status = construct ? fvm_exec_constructor_count(c, &stack[stack.size() - fixed], stack.size() - fixed, &argc)
                       : fvm_exec_argument_count(c, &stack[stack.size() - fixed], stack.size() - fixed, &argc);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_detail::vector<FVM_Value> args(argc);
    for (unsigned int i = 0; i < argc; ++i)
    {
        args[i] = stack[stack.size() - argument - i];
    }

    FVM_Object *target = 0;
    status = selected_target(c, s->frame->receiver, &target);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_Object *scope = c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back();
    FVM_Object *receiver = c->swf_loaded && c->swf_info.version >= FVM_SWF_VERSION_5 && c->script_this
                               ? c->script_this
                               : s->frame->receiver;
    FVM_Value result;
    if (construct)
    {
        status = fvm_exec_new_object(c,
                                     scope,
                                     target,
                                     receiver,
                                     s->frame->activation != 0,
                                     &stack.back(),
                                     argc,
                                     argc ? args.begin() : 0,
                                     &result);
    }
    else
    {
        FVM_Object *fallback = c->global;
        if (c->swf_loaded && c->avm1_base_clip)
        {
            status = selected_target(c, c->avm1_base_clip, &fallback);
        }
        if (status == FVM_OK)
        {
            status = fvm_exec_call_function(c,
                                            scope,
                                            target,
                                            receiver,
                                            fallback,
                                            s->frame->activation != 0,
                                            &stack.back(),
                                            argc,
                                            argc ? args.begin() : 0,
                                            &result);
        }
    }
    if (status == FVM_OK)
    { /* [GC:] Replace a rooted input before shrinking, with no post-dispatch allocation. */
        size_t first = stack.size() - argc - fixed;
        stack[first] = result;
        stack.resize(first + 1);
    }
    return status;
}
