/* FlashVM: forced native display detachment shared by C and generated bodies. */
#include "support.hpp"
#include "detach_clip_constants.h"
#include "../../standard/movieclip_scene.hpp"


/* Detach a native instance without script method lookup or target-path resolution. */
extern "C" FVM_Result fvm_exec_detach_clip(FVM_Context *context, FVM_Object *clip)
{
    if (!context || context->destroying || context->gc_releasing || !clip || clip->owner != context ||
        clip->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(clip))
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Keep the native wrapper and its private state reachable during preparation. */
        FVM_ExecutionRoots roots(context, FVM_DETACH_CLIP_ROOT_COUNT);
        roots.values[FVM_DETACH_CLIP_INPUT_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_DETACH_CLIP_INPUT_ROOT].object = clip;
        return fvm_movieclip_scene_remove(context, clip);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
