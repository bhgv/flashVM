/* FlashVM: read retained source SWF version independently from ambient execution metadata. */
#include "movieclip_state.hpp"
#include "movieclip_storage.hpp"
#include "movieclip_provenance_constants.h"
#include "../swf/format_constants.h"
#include <cmath>


/* Read optional library provenance without script lookup; older records use ambient metadata. */
FVM_Result fvm_movieclip_library_version(FVM_Context *c, FVM_Object *library, unsigned int *out)
{
    if (!c || !library || library->owner != c || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    unsigned int version = c->swf_loaded ? c->swf_info.version : 0;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator field =
        library->properties.find(FVM_MOVIECLIP_LIBRARY_VERSION_NAME);
    if (field != library->properties.end())
    {
        const FVM_Value &value = field->second;
        if (value.type != FVM_VALUE_NUMBER || !(value.number >= FVM_SWF_VERSION_1) ||
            value.number > FVM_SWF_VERSION_8 || std::floor(value.number) != value.number)
        {
            return FVM_INVALID_ARGUMENT;
        }
        version = (unsigned int)value.number;
    }
    *out = version;
    return FVM_OK;
}


/* Read the source library version; old graphs and native empty clips use the ambient policy.
 * [GC:] The caller retains the clip; this raw traversal has no callbacks or collection points. */
FVM_Result fvm_movieclip_movie_version(FVM_Context *c, FVM_Object *clip, unsigned int *out)
{
    if (!c || !out || (clip && clip->owner != c))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (clip && clip->resource_kind == FVM_RESOURCE_MOVIECLIP)
    {
        FVM_Object *data = 0;
        FVM_Result status = fvm_movieclip_storage::state(clip, data);
        if (status != FVM_OK || data->owner != c)
        {
            return FVM_INVALID_ARGUMENT;
        }
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator library = data->properties.find("library");
        if (library != data->properties.end())
        {
            const FVM_Value &edge = library->second;
            if (edge.type != FVM_VALUE_OBJECT || !edge.object || edge.object->owner != c)
            {
                return FVM_INVALID_ARGUMENT;
            }
            return fvm_movieclip_library_version(c, edge.object, out);
        }
    }
    *out = c->swf_loaded ? c->swf_info.version : 0;
    return FVM_OK;
}
