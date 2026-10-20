/* Committed source statistics are copied scalars, separate from live network jobs and geometry. */
#include "movieclip_content.hpp"
#include "movieclip_storage.hpp"
#include "movieclip_state.hpp"
#include "image_loading_constants.h"
#include "../swf/format_constants.h"
#include <cstring>
#include <new>
namespace
{
using namespace fvm_movieclip_storage;
/* Validate format-specific scalar consistency before exposing or storing metadata. */
bool consistent(const FVM_MovieClipContentInfo &info)
{
    if (info.format == FVM_MOVIECLIP_CONTENT_EMPTY)
        return !info.encoded_bytes && !info.decoded_bytes && !info.swf_version &&
               !info.pixel_width && !info.pixel_height;
    if (!info.encoded_bytes || info.encoded_bytes > FVM_MOVIECLIP_CONTENT_MAX_BYTES ||
        !info.decoded_bytes || info.decoded_bytes > FVM_MOVIECLIP_CONTENT_MAX_BYTES)
        return false;
    if (info.format == FVM_MOVIECLIP_CONTENT_SWF)
        return info.swf_version >= FVM_SWF_VERSION_1 && info.swf_version <= FVM_SWF_VERSION_8 &&
               !info.pixel_width && !info.pixel_height;
    return info.format >= FVM_MOVIECLIP_CONTENT_PNG && info.format <= FVM_MOVIECLIP_CONTENT_JPEG &&
           !info.swf_version && info.pixel_width && info.pixel_height &&
           info.pixel_width <= FVM_IMAGE_CONTENT_MAX_DIMENSION &&
           info.pixel_height <= FVM_IMAGE_CONTENT_MAX_DIMENSION &&
           info.decoded_bytes ==
               (size_t)info.pixel_width * info.pixel_height * FVM_IMAGE_RGBA_CHANNELS;
}
} // namespace
/* Initialize all descriptor bytes, including padding, for ordinary C89 host use. */
extern "C" FVM_Result fvm_movieclip_content_info_init(FVM_MovieClipContentInfo *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    std::memset(out, 0, sizeof(*out));
    out->struct_size = sizeof(*out);
    return FVM_OK;
}
/* [GC:] Only the unpublished state receives a new edge; failed allocation cannot affect a target.
 */
FVM_Result fvm_movieclip_record_content(FVM_Context *c, FVM_Object *data,
                                        FVM_MovieClipContentFormat format, size_t encoded,
                                        size_t decoded, unsigned version, unsigned width,
                                        unsigned height)
{
    FVM_MovieClipContentInfo info;
    fvm_movieclip_content_info_init(&info);
    info.format = format;
    info.encoded_bytes = encoded;
    info.decoded_bytes = decoded;
    info.swf_version = version;
    info.pixel_width = width;
    info.pixel_height = height;
    if (!consistent(info))
        return FVM_INVALID_ARGUMENT;
    FVM_Object *record = 0;
    FVM_Result status = fvm_object_create(c, &record);
    if (status != FVM_OK)
        return status;
    record->properties["format"] = number(format);
    record->properties["encoded"] = number((double)encoded);
    record->properties["decoded"] = number((double)decoded);
    record->properties["version"] = number(version);
    record->properties["width"] = number(width);
    record->properties["height"] = number(height);
    data->properties[FVM_MOVIECLIP_CONTENT_RECORD_NAME] = object(record);
    return FVM_OK;
}
/* [GC:] Copies share immutable metadata, just as they retain their source definitions. */
void fvm_movieclip_copy_content_record(FVM_Object *source, FVM_Object *destination)
{
    Properties::iterator found = source->properties.find(FVM_MOVIECLIP_CONTENT_RECORD_NAME);
    if (found != source->properties.end())
        destination->properties[FVM_MOVIECLIP_CONTENT_RECORD_NAME] = found->second;
}
/* Query raw private state without script getters; publish a complete local snapshot only on
 * success. */
extern "C" FVM_Result fvm_movieclip_get_content_info(FVM_Context *c, FVM_Object *clip,
                                                     FVM_MovieClipContentInfo *out)
{
    if (!c || !clip || clip->owner != c || !out || out->struct_size != sizeof(*out) ||
        c->destroying || c->gc_collecting || c->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_Object *data = 0;
        if (state(clip, data) != FVM_OK || data->owner != c ||
            fvm_movieclip_is_class_prototype(clip))
            return FVM_INVALID_ARGUMENT;
        Properties::iterator found = data->properties.find(FVM_MOVIECLIP_CONTENT_RECORD_NAME);
        if (found == data->properties.end())
            return FVM_NOT_FOUND;
        const FVM_Value &value = found->second;
        if (value.type != FVM_VALUE_OBJECT || !value.object || value.object->owner != c)
            return FVM_INVALID_ARGUMENT;
        FVM_Object *record = value.object;
        unsigned long format = 0, encoded = 0, decoded = 0, version = 0, width = 0, height = 0;
        if (integer_field(record, "format", FVM_MOVIECLIP_CONTENT_JPEG, format) != FVM_OK ||
            integer_field(record, "encoded", FVM_MOVIECLIP_CONTENT_MAX_BYTES, encoded) != FVM_OK ||
            integer_field(record, "decoded", FVM_MOVIECLIP_CONTENT_MAX_BYTES, decoded) != FVM_OK ||
            integer_field(record, "version", FVM_SWF_VERSION_8, version) != FVM_OK ||
            integer_field(record, "width", FVM_IMAGE_CONTENT_MAX_DIMENSION, width) != FVM_OK ||
            integer_field(record, "height", FVM_IMAGE_CONTENT_MAX_DIMENSION, height) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
        FVM_MovieClipContentInfo info;
        fvm_movieclip_content_info_init(&info);
        info.format = (FVM_MovieClipContentFormat)format;
        info.encoded_bytes = (size_t)encoded;
        info.decoded_bytes = (size_t)decoded;
        info.swf_version = (unsigned)version;
        info.pixel_width = (unsigned)width;
        info.pixel_height = (unsigned)height;
        if (!consistent(info))
            return FVM_INVALID_ARGUMENT;
        *out = info;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
