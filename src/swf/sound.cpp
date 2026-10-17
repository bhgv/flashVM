/* FlashVM: allocation-free DefineSound lookup and bounded native-S16 extraction.
 * Parsing is independent of MovieClip timeline import and never opens an output. */
#include "../core/runtime.hpp"
#include "flashvm/swf_sound.h"
#include "sound_adpcm.hpp"
#include "sound_constants.h"
#include <climits>
#include <cstring>

namespace
{
/* Decode a bounded little-endian field; all call sites validate its enclosing span first. */
unsigned long sound_integer(const unsigned char *bytes, unsigned int count)
{
    unsigned long value = 0;
    for (unsigned int i = 0; i < count; ++i)
        value |= (unsigned long)bytes[i] << (CHAR_BIT * i);
    return value;
}

/* Parse one complete header; PCM geometry is exact, other codec bytes remain opaque. */
FVM_Result sound_header(const unsigned char *bytes, size_t size, unsigned int version, FVM_SWFSoundInfo &info)
{
    if (size < FVM_SWF_SOUND_HEADER_BYTES)
        return FVM_INVALID_ARGUMENT;
    std::memset(&info, 0, sizeof(info));
    info.struct_size = sizeof(info);
    info.sound_id = (unsigned int)sound_integer(bytes, FVM_SWF_SOUND_UI16_BYTES);
    if (!info.sound_id)
        return FVM_INVALID_ARGUMENT;
    unsigned int flags = bytes[FVM_SWF_SOUND_FLAGS_OFFSET];
    info.format = flags >> FVM_SWF_SOUND_FORMAT_SHIFT;
    const unsigned int rates[] = {FVM_SWF_SOUND_RATE_5K, FVM_SWF_SOUND_RATE_11K, FVM_SWF_SOUND_RATE_22K,
                                  FVM_SWF_SOUND_RATE_44K};
    info.sample_rate = rates[(flags & FVM_SWF_SOUND_RATE_MASK) >> FVM_SWF_SOUND_RATE_SHIFT];
    info.bits_per_sample = flags & FVM_SWF_SOUND_SIZE_MASK ? FVM_SWF_SOUND_BITS_16 : FVM_SWF_SOUND_BITS_8;
    info.channels = flags & FVM_SWF_SOUND_STEREO_MASK ? FVM_SWF_SOUND_STEREO_CHANNELS : FVM_SWF_SOUND_MONO_CHANNELS;
    info.sample_count = sound_integer(bytes + FVM_SWF_SOUND_COUNT_OFFSET, FVM_SWF_SOUND_UI32_BYTES);
    info.data = bytes + FVM_SWF_SOUND_HEADER_BYTES;
    info.data_size = size - FVM_SWF_SOUND_HEADER_BYTES;
    if (info.format == FVM_SWF_SOUND_FORMAT_LE && version < FVM_SWF_SOUND_LE_MIN_VERSION)
        return FVM_INVALID_ARGUMENT;
    if (info.format == FVM_SWF_SOUND_FORMAT_NATIVE || info.format == FVM_SWF_SOUND_FORMAT_LE)
    {
        size_t frame_bytes = info.channels * (info.bits_per_sample / CHAR_BIT);
        if (info.data_size % frame_bytes || info.data_size / frame_bytes != info.sample_count)
            return FVM_INVALID_ARGUMENT;
    }
    return FVM_OK;
}

/* Inspect loaded immutable bytes only on the owning VM thread outside terminal GC phases. */
bool sound_context(FVM_Context *context)
{
    return context && !context->destroying && !context->gc_collecting && !context->gc_releasing;
}
} // namespace

/* Scan the complete root stream and reject ambiguous duplicate definitions of the requested ID. */
static FVM_Result sound_lookup(FVM_Context *context, unsigned int id, size_t ordinal, bool by_index,
                               FVM_SWFSoundInfo *out, size_t *total)
{
    if (!sound_context(context))
        return FVM_INVALID_ARGUMENT;
    if (!context->swf_loaded)
        return FVM_UNSUPPORTED;
    const fvm_detail::vector<unsigned char> &bytes = context->swf_bytes;
    if (CHAR_BIT != FVM_SWF_SOUND_BYTE_BITS)
        return FVM_UNSUPPORTED;
    if (bytes.size() < FVM_SWF_SOUND_MIN_FILE_BYTES)
        return FVM_INVALID_ARGUMENT;
    const unsigned char *data = bytes.begin();
    size_t rectangle =
        (FVM_SWF_SOUND_RECT_WIDTH_BITS +
         FVM_SWF_SOUND_RECT_COORDINATES * (data[FVM_SWF_SOUND_FILE_PREFIX_BYTES] >> FVM_SWF_SOUND_RECT_WIDTH_SHIFT) +
         FVM_SWF_SOUND_BYTE_ROUNDING) /
        FVM_SWF_SOUND_BYTE_BITS;
    if (rectangle > bytes.size() - FVM_SWF_SOUND_FILE_PREFIX_BYTES ||
        bytes.size() - FVM_SWF_SOUND_FILE_PREFIX_BYTES - rectangle < FVM_SWF_SOUND_MOVIE_HEADER_BYTES)
        return FVM_INVALID_ARGUMENT;
    size_t at = FVM_SWF_SOUND_FILE_PREFIX_BYTES + rectangle + FVM_SWF_SOUND_MOVIE_HEADER_BYTES;
    bool found = false;
    size_t sounds = 0;
    FVM_SWFSoundInfo selected;
    while (at < bytes.size())
    {
        if (bytes.size() - at < FVM_SWF_SOUND_UI16_BYTES)
            return FVM_INVALID_ARGUMENT;
        unsigned int header = (unsigned int)sound_integer(data + at, FVM_SWF_SOUND_UI16_BYTES);
        at += FVM_SWF_SOUND_UI16_BYTES;
        unsigned int tag = header >> FVM_SWF_TAG_CODE_SHIFT;
        size_t count = header & FVM_SWF_TAG_LENGTH_MASK;
        if (count == FVM_SWF_TAG_LENGTH_MASK)
        {
            if (bytes.size() - at < FVM_SWF_SOUND_UI32_BYTES)
                return FVM_INVALID_ARGUMENT;
            count = (size_t)sound_integer(data + at, FVM_SWF_SOUND_UI32_BYTES);
            at += FVM_SWF_SOUND_UI32_BYTES;
        }
        if (count > bytes.size() - at)
            return FVM_INVALID_ARGUMENT;
        if (tag == FVM_SWF_TAG_DEFINE_SOUND)
        {
            FVM_SWFSoundInfo current;
            FVM_Result result = sound_header(data + at, count, context->swf_info.version, current);
            if (result != FVM_OK)
                return result;
            if (out && (by_index ? sounds == ordinal : current.sound_id == id))
            {
                if (found)
                    return FVM_INVALID_ARGUMENT;
                selected = current;
                found = true;
            }
            ++sounds;
        }
        at += count;
        if (tag == FVM_SWF_TAG_END)
        {
            if (count || at != bytes.size())
                return FVM_INVALID_ARGUMENT;
            if (out && !found)
                return FVM_NOT_FOUND;
            if (out)
                *out = selected;
            if (total)
                *total = sounds;
            return FVM_OK;
        }
    }
    return FVM_INVALID_ARGUMENT;
}

/* Count root definition occurrences; this does not certify uniqueness of every dictionary ID. */
extern "C" FVM_Result fvm_swf_sound_count(FVM_Context *context, size_t *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    size_t count = 0;
    FVM_Result result = sound_lookup(context, 0, 0, false, 0, &count);
    if (result == FVM_OK)
        *out = count;
    return result;
}

/* Inspect a selected ID transactionally, leaving all output fields unchanged on failure. */
extern "C" FVM_Result fvm_swf_sound_info(FVM_Context *context, unsigned int id, FVM_SWFSoundInfo *out)
{
    if (!out || out->struct_size != sizeof(*out) || !id || id > FVM_SWF_SOUND_ID_MAX)
        return FVM_INVALID_ARGUMENT;
    return sound_lookup(context, id, 0, false, out, 0);
}

/* Resolve an ordinal, then reuse ID lookup to reject duplicate IDs before publication. */
extern "C" FVM_Result fvm_swf_sound_at(FVM_Context *context, size_t index, FVM_SWFSoundInfo *out)
{
    if (!out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundInfo selected;
    FVM_Result result = sound_lookup(context, 0, index, true, &selected, 0);
    if (result != FVM_OK)
        return result;
    return fvm_swf_sound_info(context, selected.sound_id, out);
}

/* Publish a requested PCM range only after validating format, bounds and destination capacity. */
extern "C" FVM_Result fvm_swf_sound_read_pcm(FVM_Context *context, unsigned int id, unsigned long first, size_t frames,
                                             short *destination, size_t capacity, size_t *written)
{
    if (!written)
        return FVM_INVALID_ARGUMENT;
    *written = 0;
    FVM_SWFSoundInfo info;
    info.struct_size = sizeof(info);
    FVM_Result result = fvm_swf_sound_info(context, id, &info);
    if (result != FVM_OK)
        return result;
    if ((info.format != FVM_SWF_SOUND_ADPCM &&
         ((info.format != FVM_SWF_SOUND_FORMAT_NATIVE && info.format != FVM_SWF_SOUND_FORMAT_LE) ||
          info.bits_per_sample != FVM_SWF_SOUND_BITS_16)) ||
        sizeof(short) != FVM_SWF_SOUND_PCM_BYTES || SHRT_MAX != FVM_SWF_SOUND_SHORT_MAX)
        return FVM_UNSUPPORTED;
    if (first > info.sample_count || frames > info.sample_count - first || frames > capacity / info.channels ||
        (frames && !destination))
        return FVM_INVALID_ARGUMENT;
    size_t values = frames * info.channels;
    if (info.format == FVM_SWF_SOUND_ADPCM)
    {
        /* Validate the entire declared stream before publishing any caller samples. */
        if (!fvm_sound_adpcm(info, first, frames, 0))
            return FVM_INVALID_ARGUMENT;
        if (frames)
            fvm_sound_adpcm(info, first, frames, destination);
        *written = values;
        return FVM_OK;
    }
    const unsigned char *source = info.data + (size_t)first * info.channels * FVM_SWF_SOUND_PCM_BYTES;
    for (size_t i = 0; i < values; ++i)
    {
        unsigned long value;
        if (info.format == FVM_SWF_SOUND_FORMAT_NATIVE)
        {
            unsigned short native;
            std::memcpy(&native, source + i * FVM_SWF_SOUND_PCM_BYTES, sizeof(native));
            value = native;
        }
        else
            value = sound_integer(source + i * FVM_SWF_SOUND_PCM_BYTES, FVM_SWF_SOUND_PCM_BYTES);
        destination[i] =
            (short)(value >= FVM_SWF_SOUND_SIGN_BIT ? (long)value - FVM_SWF_SOUND_SIGN_RANGE : (long)value);
    }
    *written = values;
    return FVM_OK;
}
