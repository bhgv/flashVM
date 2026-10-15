/* FlashVM: intrinsic native root-lock state with legacy graph defaults. */
#include "movieclip_lockroot.hpp"
#include "movieclip_storage.hpp"
#include <new>
/* Read stored Boolean state; missing fields in new/legacy graphs mean unlocked. */
FVM_Result fvm_movieclip_root_locked(FVM_Context *c, FVM_Object *clip, bool *out)
{
    if (!c || !clip || clip->owner != c || !out)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_Object *data = 0;
        FVM_Result status = fvm_movieclip_storage::state(clip, data);
        if (status != FVM_OK)
            return status;
        fvm_movieclip_storage::Properties::iterator flag = data->properties.find("lock_root");
        if (flag == data->properties.end())
        {
            *out = false;
            return FVM_OK;
        }
        if (flag->second.type != FVM_VALUE_BOOLEAN)
            return FVM_INVALID_ARGUMENT;
        *out = flag->second.boolean != 0;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
