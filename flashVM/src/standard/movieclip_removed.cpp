/* FlashVM: effective native detachment identity across retained parent edges. */
#include "movieclip_scene.hpp"
#include "movieclip_storage.hpp"
#include "flashvm/execution.h"
#include <new>


/* Query the clip and its ancestors without script callbacks or partial output publication. */
FVM_Result fvm_movieclip_scene_removed(FVM_Context *c, FVM_Object *clip, bool *out)
{
    if (!c || !clip || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {
        unsigned int visited = 0;
        for (FVM_Object *node = clip; node; node = fvm_movieclip_storage::parent(node))
        {
            if (++visited > FVM_MOVIECLIP_REMOVE_MAX_ANCESTORS)
            {
                return FVM_ERROR;
            }
            FVM_Value marker;
            FVM_Result status = fvm_exec_clip_detached(c, node, &marker);
            if (status != FVM_OK)
            {
                return status;
            }
            if (marker.boolean)
            {
                *out = true;
                return FVM_OK;
            }
        }
        *out = false;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
