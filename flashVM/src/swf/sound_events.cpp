/* FlashVM: allocation-free root StartSound/SOUNDINFO inspection.
 * These APIs preserve control records for host scheduling; they never play audio. */
#include "../core/runtime.hpp"
#include "../core/execution_work_scope.hpp"
#include "reader.hpp"
#include "sound_constants.h"
#include "sound_event_constants.h"
#include "sound_event_index.hpp"
#include <new>
#include <climits>
#include <cstring>

namespace
{
/* Parse one exact StartSound body and retain the envelope span only for this call. */
FVM_Result event_body(const unsigned char *bytes, size_t size, unsigned int frame, FVM_SWFSoundEvent &event,
                      const unsigned char *&envelope, FVM_ExecutionWorkScope &work)
{
    FVM_Result charged = work.step();
    if (charged != FVM_OK)
        return charged;
    if (size < FVM_SWF_SOUND_EVENT_MIN_BYTES)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFReader reader(bytes, size);
    std::memset(&event, 0, sizeof(event));
    event.struct_size = sizeof(event);
    event.frame = frame;
    event.sound_id = (unsigned int)reader.integer(FVM_SWF_SOUND_UI16_BYTES);
    event.flags = (unsigned int)reader.integer(1);
    if (!event.sound_id || (event.flags & FVM_SWF_SOUND_EVENT_RESERVED))
        return FVM_INVALID_ARGUMENT;
    if (event.flags & FVM_SWF_SOUND_EVENT_IN_POINT)
        event.in_point = reader.integer(FVM_SWF_SOUND_UI32_BYTES);
    if (event.flags & FVM_SWF_SOUND_EVENT_OUT_POINT)
        event.out_point = reader.integer(FVM_SWF_SOUND_UI32_BYTES);
    if (event.flags & FVM_SWF_SOUND_EVENT_LOOPS)
        event.loop_count = (unsigned int)reader.integer(FVM_SWF_SOUND_UI16_BYTES);
    if (event.flags & FVM_SWF_SOUND_EVENT_ENVELOPE)
        event.envelope_count = (unsigned int)reader.integer(1);
    envelope = bytes + reader.position();
    for (unsigned int i = 0; i < event.envelope_count; ++i)
    {
        charged = work.step();
        if (charged != FVM_OK)
            return charged;
        reader.integer(FVM_SWF_SOUND_UI32_BYTES);
        unsigned long left = reader.integer(FVM_SWF_SOUND_UI16_BYTES);
        unsigned long right = reader.integer(FVM_SWF_SOUND_UI16_BYTES);
        if (left > FVM_SWF_SOUND_ENVELOPE_LEVEL_MAX || right > FVM_SWF_SOUND_ENVELOPE_LEVEL_MAX)
            return FVM_INVALID_ARGUMENT;
    }
    return reader.finished() ? FVM_OK : FVM_INVALID_ARGUMENT;
}

/* Scan all root controls before publishing an event, count or envelope pointer. */
FVM_Result event_lookup(FVM_Context *context, size_t ordinal, FVM_SWFSoundEvent *out, const unsigned char **envelope,
                        size_t *count, unsigned int selected_frame = 0,
                        fvm_detail::vector<size_t> *frame_ordinals = 0)
{
    /* [GC:] Canonical bytes are context-owned; refuse reads during terminal collector phases. */
    if (!context || context->destroying || context->gc_collecting || context->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    if (!context->swf_loaded)
        return FVM_UNSUPPORTED;
    FVM_ExecutionWorkScope work(context);
    if (CHAR_BIT != FVM_SWF_SOUND_BYTE_BITS)
        return FVM_UNSUPPORTED;
    const fvm_detail::vector<unsigned char> &bytes = context->swf_bytes;
    if (bytes.size() < FVM_SWF_SOUND_MIN_FILE_BYTES)
        return FVM_INVALID_ARGUMENT;
    size_t rectangle =
        (FVM_SWF_SOUND_RECT_WIDTH_BITS +
         FVM_SWF_SOUND_RECT_COORDINATES * (bytes[FVM_SWF_SOUND_FILE_PREFIX_BYTES] >> FVM_SWF_SOUND_RECT_WIDTH_SHIFT) +
         FVM_SWF_SOUND_BYTE_ROUNDING) /
        FVM_SWF_SOUND_BYTE_BITS;
    if (rectangle > bytes.size() - FVM_SWF_SOUND_FILE_PREFIX_BYTES ||
        bytes.size() - FVM_SWF_SOUND_FILE_PREFIX_BYTES - rectangle < FVM_SWF_SOUND_MOVIE_HEADER_BYTES)
        return FVM_INVALID_ARGUMENT;
    size_t at = FVM_SWF_SOUND_FILE_PREFIX_BYTES + rectangle + FVM_SWF_SOUND_MOVIE_HEADER_BYTES;
    size_t events = 0;
    unsigned int frame = 1;
    bool found = false;
    FVM_SWFSoundEvent selected;
    const unsigned char *selected_envelope = 0;
    while (at < bytes.size())
    {
        FVM_Result charged = work.step();
        if (charged != FVM_OK)
            return charged;
        FVM_SWFReader reader(bytes.begin() + at, bytes.size() - at);
        unsigned int header = (unsigned int)reader.integer(FVM_SWF_SOUND_UI16_BYTES);
        unsigned int tag = header >> FVM_SWF_TAG_CODE_SHIFT;
        unsigned long length = header & FVM_SWF_TAG_LENGTH_MASK;
        if (length == FVM_SWF_TAG_LENGTH_MASK)
            length = reader.integer(FVM_SWF_SOUND_UI32_BYTES);
        if (!reader.good() || length > bytes.size() - at - reader.position())
            return FVM_INVALID_ARGUMENT;
        at += reader.position();
        if (tag == FVM_SWF_SOUND_EVENT_TAG)
        {
            if (frame > context->swf_info.frame_count)
                return FVM_INVALID_ARGUMENT;
            FVM_SWFSoundEvent current;
            const unsigned char *points = 0;
            FVM_Result result = event_body(bytes.begin() + at, (size_t)length, frame, current, points, work);
            if (result != FVM_OK)
                return result;
            if (events == ordinal && out)
            {
                selected = current;
                selected_envelope = points;
                found = true;
            }
            if (frame_ordinals && frame == selected_frame)
            {
                if (frame_ordinals->size() >= FVM_SWF_SOUND_MAX_FRAME_ORDINALS)
                    return FVM_INVALID_ARGUMENT;
                result = work.step();
                if (result != FVM_OK)
                    return result;
                frame_ordinals->push_back(events);
            }
            ++events;
        }
        else if (tag == FVM_SWF_TAG_SHOW_FRAME)
        {
            if (length || frame > context->swf_info.frame_count)
                return FVM_INVALID_ARGUMENT;
            ++frame;
        }
        at += (size_t)length;
        if (tag == FVM_SWF_TAG_END)
        {
            if (length || at != bytes.size() || frame - 1 != context->swf_info.frame_count)
                return FVM_INVALID_ARGUMENT;
            if (out && !found)
                return FVM_NOT_FOUND;
            if (out)
            {
                FVM_SWFSoundInfo info;
                info.struct_size = sizeof(info);
                FVM_Result result = fvm_swf_sound_info(context, selected.sound_id, &info);
                if (result != FVM_OK)
                    return result;
                *out = selected;
                if (envelope)
                    *envelope = selected_envelope;
            }
            if (count)
                *count = events;
            return FVM_OK;
        }
    }
    return FVM_INVALID_ARGUMENT;
}
} // namespace

/* Count structurally valid controls, without resolving every resource dictionary ID. */
extern "C" FVM_Result fvm_swf_sound_event_count(FVM_Context *context, size_t *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    size_t count = 0;
    FVM_Result result = event_lookup(context, 0, 0, 0, &count);
    if (result == FVM_OK)
        *out = count;
    return result;
}

/* Publish a copied event only after complete root-stream and selected resource validation. */
extern "C" FVM_Result fvm_swf_sound_event_at(FVM_Context *context, size_t ordinal, FVM_SWFSoundEvent *out)
{
    if (!out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    return event_lookup(context, ordinal, out, 0, 0);
}

/* Re-query the current file so no borrowed envelope address is part of the public ABI. */
extern "C" FVM_Result fvm_swf_sound_event_envelope(FVM_Context *context, size_t ordinal, size_t point,
                                                   FVM_SWFSoundEnvelope *out)
{
    if (!out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundEvent event;
    const unsigned char *bytes = 0;
    FVM_Result result = event_lookup(context, ordinal, &event, &bytes, 0);
    if (result != FVM_OK)
        return result;
    if (point >= event.envelope_count)
        return FVM_NOT_FOUND;
    FVM_SWFReader reader(bytes + point * FVM_SWF_SOUND_ENVELOPE_BYTES, FVM_SWF_SOUND_ENVELOPE_BYTES);
    FVM_SWFSoundEnvelope value;
    value.struct_size = sizeof(value);
    value.position_44 = reader.integer(FVM_SWF_SOUND_UI32_BYTES);
    value.left_level = (unsigned int)reader.integer(FVM_SWF_SOUND_UI16_BYTES);
    value.right_level = (unsigned int)reader.integer(FVM_SWF_SOUND_UI16_BYTES);
    *out = value;
    return FVM_OK;
}

/* Build one canonical frame index transactionally; parsing and collection share native work. */
FVM_Result fvm_swf_sound_frame_ordinals(FVM_Context *context, unsigned int frame,
                                        fvm_detail::vector<size_t> &out)
{
    if (!context || context->destroying || context->gc_collecting || context->gc_releasing || !frame)
        return FVM_INVALID_ARGUMENT;
    if (!context->swf_loaded)
        return FVM_UNSUPPORTED;
    if (frame > context->swf_info.frame_count)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::vector<size_t> pending;
        FVM_Result result = event_lookup(context, 0, 0, 0, 0, frame, &pending);
        if (result == FVM_OK)
            out.swap(pending);
        return result;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
