/* FlashVM: native detachment identity query shared by C and generated code. */
#include "support.hpp"
#include "../../standard/movieclip_storage.hpp"
/* Read committed private state without invoking script properties or changing reachability. */
extern "C" FVM_Result fvm_exec_clip_detached(FVM_Context *c, FVM_Object *clip, FVM_Value *out)
{
    if (!c || !out || c->destroying || c->gc_releasing || !clip || clip->owner != c ||
        clip->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(clip))
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_Object *data = 0;
        FVM_Result status = fvm_movieclip_storage::state(clip, data);
        if (status != FVM_OK)
            return status;
        FVM_Value marker;
        status = fvm_movieclip_storage::field(data, "detached", marker);
        if (status == FVM_INVALID_ARGUMENT)
        {
            /* Older native graphs did not record detachment independently of parent. */
            fvm_value_boolean(out, 0);
            return FVM_OK;
        }
        if (status != FVM_OK)
            return status;
        if (marker.type != FVM_VALUE_NUMBER ||
            (marker.number != FVM_MOVIECLIP_ATTACHED_STATE &&
             marker.number != FVM_MOVIECLIP_DETACHED_STATE))
            return FVM_INVALID_ARGUMENT;
        fvm_value_boolean(out, marker.number == FVM_MOVIECLIP_DETACHED_STATE);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
