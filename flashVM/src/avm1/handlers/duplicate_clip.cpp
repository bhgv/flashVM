/* FlashVM: explicit native sibling construction for C callers and generated bodies. */
#include "support.hpp"
#include "duplicate_clip_constants.h"


/* Retain value operands and call intrinsic construction without replaceable method lookup. */
extern "C" FVM_Result fvm_exec_duplicate_clip(FVM_Context *c,
                                              FVM_Object *clip,
                                              const FVM_Value *name,
                                              const FVM_Value *depth,
                                              const FVM_Value *initial,
                                              FVM_Value *out)
{
    if (!c || c->destroying || c->gc_releasing || !out || !clip || clip->owner != c ||
        clip->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(clip) ||
        fvm_validate_value(c, name) != FVM_OK || fvm_validate_value(c, depth) != FVM_OK ||
        (initial && fvm_validate_value(c, initial) != FVM_OK))
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Root every operand before construction/coercion can invoke script callbacks. */
        FVM_ExecutionRoots roots(c, FVM_DUPLICATE_CLIP_ROOT_COUNT);
        roots.values[FVM_DUPLICATE_CLIP_RECEIVER_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_DUPLICATE_CLIP_RECEIVER_ROOT].object = clip;
        roots.values[FVM_DUPLICATE_CLIP_NAME_ROOT] = *name;
        roots.values[FVM_DUPLICATE_CLIP_DEPTH_ROOT] = *depth;
        if (initial)
        {
            roots.values[FVM_DUPLICATE_CLIP_INITIAL_ROOT] = *initial;
        }


        /* [GC:] Existing construction helpers retain unpublished geometry under a native barrier.
         */
        FVM_NativeCallGuard guard(c);
        FVM_Result status =
            fvm_movieclip_duplicate_instance(c,
                                             clip,
                                             &roots.values[FVM_DUPLICATE_CLIP_NAME_ROOT],
                                             &roots.values[FVM_DUPLICATE_CLIP_DEPTH_ROOT],
                                             initial ? &roots.values[FVM_DUPLICATE_CLIP_INITIAL_ROOT] : 0,
                                             &roots.values[FVM_DUPLICATE_CLIP_RESULT_ROOT]);
        if (status == FVM_OK)
        {
            *out = roots.values[FVM_DUPLICATE_CLIP_RESULT_ROOT];
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
