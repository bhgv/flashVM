/* FlashVM: native cloning, removal and dragging share rooted scene execution services. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"
#include "../avm1/operand_constants.h"
#include "../standard/movieclip_scene.hpp"
#include "flashvm/movieclip.h"

namespace
{


/* Recover the physical root after a missing selected target, as ordinary AVM1 scene lookup does. */
FVM_Result scene_target(FVM_JitInvocation *s, FVM_Object **out)
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


/* [GC:] Keep source/name/depth in the rooted operand vector until native construction succeeds. */
FVM_Result fvm_jit_clone_sprite(FVM_JitInvocation *s, unsigned int)
{
    FVM_Context *c = s->context;
    FVM_Result status = fvm_avm1_require_operands(c->swf_loaded, *s->stack, FVM_AVM1_CLONE_OPERANDS);
    FVM_Object *start = 0;
    if (status == FVM_OK)
    {
        status = scene_target(s, &start);
    }
    if (status != FVM_OK)
    {
        return status;
    }
    size_t first = s->stack->size() - FVM_AVM1_CLONE_OPERANDS;
    FVM_Value ignored;
    status = fvm_exec_duplicate_target(c,
                                       start,
                                       &(*s->stack)[first + FVM_AVM1_CLONE_SOURCE_OFFSET],
                                       &(*s->stack)[first + FVM_AVM1_CLONE_NAME_OFFSET],
                                       &(*s->stack)[first + FVM_AVM1_CLONE_DEPTH_OFFSET],
                                       &ignored);
    if (status == FVM_OK)
    {
        s->stack->resize(first);
    }
    return status;
}


/* [GC:] Retain the removal input through coercion, then repair the active target after detachment. */
FVM_Result fvm_jit_remove_sprite(FVM_JitInvocation *s, unsigned int)
{
    FVM_Context *c = s->context;
    FVM_Result status = fvm_avm1_require_value(c->swf_loaded, *s->stack);
    FVM_Object *start = 0;
    if (status == FVM_OK)
    {
        status = scene_target(s, &start);
    }
    if (status == FVM_OK)
    {
        status = fvm_exec_remove_target(c, start, &s->stack->back());
    }
    if (status != FVM_OK)
    {
        return status;
    }
    s->stack->pop_back();
    if (c->avm1_target_clip)
    {
        bool removed = false;
        status = fvm_movieclip_scene_removed(c, c->avm1_target_clip, &removed);
        if (status == FVM_OK && removed)
        {
            bool base_removed = true;
            if (c->avm1_base_clip)
            {
                status = fvm_movieclip_scene_removed(c, c->avm1_base_clip, &base_removed);
            }
            if (status == FVM_OK)
            {
                c->avm1_target_clip = base_removed ? 0 : c->avm1_base_clip;
            }
        }
    }
    return status;
}


/* [GC:] Retain the resolved clip while ordered flags and optional bounds can collect or throw. */
FVM_Result fvm_jit_start_drag(FVM_JitInvocation *s, unsigned int)
{
    FVM_Context *c = s->context;
    FVM_Result status = fvm_avm1_require_operands(c->swf_loaded, *s->stack, FVM_AVM1_DRAG_FIXED_OPERANDS);
    FVM_Object *start = 0;
    if (status == FVM_OK)
    {
        status = scene_target(s, &start);
    }
    if (status != FVM_OK)
    {
        return status;
    }


    FVM_ExecutionRoots roots(c, FVM_AVM1_DRAG_ROOT_COUNT);
    FVM_Value &clip = roots.values[FVM_AVM1_DRAG_TARGET_ROOT];
    status = fvm_exec_resolve_clip(c, start, &s->stack->back(), 1, &clip);
    if (status != FVM_OK)
    {
        return status;
    }
    s->stack->pop_back();
    FVM_Value numeric;
    status = fvm_exec_to_number(c, 1, &s->stack->back(), &numeric);
    if (status != FVM_OK)
    {
        return status;
    }
    bool center = numeric.number == FVM_AVM1_DRAG_ENABLED_INTEGER;
    s->stack->pop_back();
    status = fvm_exec_to_number(c, 1, &s->stack->back(), &numeric);
    if (status != FVM_OK)
    {
        return status;
    }
    bool constrained = numeric.number == FVM_AVM1_DRAG_ENABLED_INTEGER;
    s->stack->pop_back();
    FVM_ExecutionDragBounds bounds;
    if (constrained)
    {
        status = fvm_avm1_require_operands(c->swf_loaded, *s->stack, FVM_AVM1_DRAG_BOUND_OPERANDS);
        if (status != FVM_OK)
        {
            return status;
        }
        double *limits[] = {&bounds.y_max, &bounds.x_max, &bounds.y_min, &bounds.x_min};
        for (unsigned int i = 0; i < FVM_AVM1_DRAG_BOUND_OPERANDS; ++i)
        {
            status = fvm_exec_to_number(c, 0, &s->stack->back(), &numeric);
            if (status != FVM_OK)
            {
                return status;
            }
            *limits[i] = numeric.number;
            s->stack->pop_back();
        }
    }
    return clip.type == FVM_VALUE_OBJECT ? fvm_exec_drag_begin(c, clip.object, center, constrained ? &bounds : 0)
                                         : FVM_OK;
}


/* Flush pending native motion and end the context-wide drag through its shared service. */
FVM_Result fvm_jit_end_drag(FVM_JitInvocation *s, unsigned int)
{
    return fvm_exec_drag_end(s->context);
}
