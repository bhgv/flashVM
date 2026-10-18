/* FlashVM: versioned physical/retained AVM1 parent identity, without script lookup. */
#include "movieclip_state.hpp"
#include "movieclip_storage.hpp"
#include <new>
/* [GC:] Read an existing managed edge; native key lookup can allocate but never collects. */
FVM_Result fvm_movieclip_avm1_parent(FVM_Context *c, FVM_Object *clip, FVM_Object **out)
{
    if (!c || !clip || clip->owner != c || !out)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_Object *data = 0;
        FVM_Result status = fvm_movieclip_storage::state(clip, data);
        if (status != FVM_OK)
            return status;
        FVM_Value value;
        status = fvm_movieclip_storage::field(data, "parent", value);
        if (status != FVM_OK)
            return status;
        if (c->swf_loaded && value.type == FVM_VALUE_NULL)
        {
            fvm_movieclip_storage::Properties::iterator retained =
                data->properties.find("avm1_parent");
            if (retained != data->properties.end())
                value = retained->second;
        }
        FVM_Object *parent = 0;
        if (value.type == FVM_VALUE_OBJECT)
        {
            parent = value.object;
            if (!parent || parent->owner != c || parent->resource_kind != FVM_RESOURCE_MOVIECLIP)
                return FVM_INVALID_ARGUMENT;
        }
        else if (value.type != FVM_VALUE_NULL)
            return FVM_INVALID_ARGUMENT;
        *out = parent;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
