/* Completed-content ActionScript queries share C snapshot validation and graph ownership. */
#include "movieclip_content_methods.hpp"
#include "movieclip_state.hpp"
#include "flashvm/movieclip.h"

/* A complete import is available all at once; unknown internal/legacy counts remain undefined. */
static FVM_Result committed_bytes(FVM_Context *c, FVM_Object *clip, FVM_Value *out)
{
    fvm_value_undefined(out);
    if (fvm_movieclip_is_class_prototype(clip))
        return FVM_OK;
    FVM_MovieClipContentInfo info;
    fvm_movieclip_content_info_init(&info);
    FVM_Result result = fvm_movieclip_get_content_info(c, clip, &info);
    if (result == FVM_NOT_FOUND)
        return FVM_OK;
    if (result != FVM_OK)
        return result;
    size_t count =
        info.format == FVM_MOVIECLIP_CONTENT_SWF ? info.decoded_bytes : info.encoded_bytes;
    fvm_value_number(out, (double)count);
    return FVM_OK;
}
/* Separate callback identity keeps loaded/total methods independently replaceable and serializable.
 */
FVM_Result fvm_movieclip_bytes_loaded(FVM_Context *c, FVM_Object *clip, unsigned, const FVM_Value *,
                                      FVM_Value *out, void *)
{
    return committed_bytes(c, clip, out);
}
/* Return the same fully published byte count; worker progress belongs to the host monitor. */
FVM_Result fvm_movieclip_bytes_total(FVM_Context *c, FVM_Object *clip, unsigned, const FVM_Value *,
                                     FVM_Value *out, void *)
{
    return committed_bytes(c, clip, out);
}
/* Image records override the retained SWF language profile used for future decode compatibility. */
FVM_Result fvm_movieclip_swf_version(FVM_Context *c, FVM_Object *clip, unsigned, const FVM_Value *,
                                     FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (fvm_movieclip_is_class_prototype(clip))
        return FVM_OK;
    FVM_MovieClipContentInfo info;
    fvm_movieclip_content_info_init(&info);
    FVM_Result result = fvm_movieclip_get_content_info(c, clip, &info);
    int version = FVM_MOVIECLIP_UNKNOWN_SWF_VERSION;
    if (result == FVM_OK)
    {
        if (info.format == FVM_MOVIECLIP_CONTENT_SWF)
            version = (int)info.swf_version;
    }
    else if (result == FVM_NOT_FOUND)
    {
        unsigned source = 0;
        result = fvm_movieclip_movie_version(c, clip, &source);
        if (result != FVM_OK)
            return result;
        if (source)
            version = (int)source;
    }
    else
        return result;
    fvm_value_number(out, version);
    return FVM_OK;
}
