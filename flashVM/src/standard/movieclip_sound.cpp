/* FlashVM: root StartSound notifications at real frame entry, without an implicit audio device. */
#include "movieclip_sound.hpp"
#include "movieclip_sound_constants.h"
#include "movieclip_constants.h"
#include "../core/execution_work_scope.hpp"
#include "../swf/sound_event_constants.h"
#include "../swf/sound_event_index.hpp"
#include <cmath>
#include <cstring>
#include <new>

namespace
{ /* Clear busy state even when a C++ host callback throws across the C ABI. */


struct DeliveryScope
{
    FVM_Context *context; /* Borrow the context protected by the advance native-call barrier. */


    /* Reject recursive advance/registration/source replacement until this delivery ends. */
    explicit DeliveryScope(FVM_Context *owner) : context(owner)
    {
        context->movieclip_sound_busy = true;
    }


    /* Restore ordinary host access on success, VM error or exception. */
    ~DeliveryScope()
    {
        context->movieclip_sound_busy = false;
    }
};
} // namespace


/* Register borrowed host state; never place process pointers in a transferable MovieClip graph. */
extern "C" FVM_Result
fvm_movieclip_set_sound_callback(FVM_Context *context, FVM_MovieClipSoundCallback callback, void *userdata)
{
    if (!context || context->destroying || context->gc_collecting || context->gc_releasing ||
        context->movieclip_sound_busy)
    {
        return FVM_INVALID_ARGUMENT;
    }
    context->movieclip_sound_callback = callback;
    context->movieclip_sound_userdata = callback ? userdata : 0;
    return FVM_OK;
}


/* Dispatch this entry's ordered root ordinals; replayed visual reconstruction never calls here. */
FVM_Result fvm_movieclip_sound_run(FVM_Context *context, FVM_Object *clip, FVM_Object *frames, unsigned long frame)
{
    typedef fvm_detail::map<fvm_detail::string, FVM_Value> Properties;
    typedef fvm_detail::map<unsigned long, FVM_Value> Slots;
    if (!context || !clip || clip->owner != context || !frames || frames->owner != context || frame < 1 ||
        frame > FVM_MOVIECLIP_MAX_FRAMES)
    {
        return FVM_INVALID_ARGUMENT;
    }


    FVM_ExecutionWorkScope work(context);
    if (context->movieclip_sound_busy)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *list = 0;
    Slots::iterator selected = frames->indexed.find(frame);
    if (selected != frames->indexed.end())
    {
        if (selected->second.type != FVM_VALUE_OBJECT || !selected->second.object ||
            selected->second.object->owner != context)
        {
            return FVM_INVALID_ARGUMENT;
        }
        Properties::iterator sounds = selected->second.object->properties.find("sounds");
        if (sounds != selected->second.object->properties.end())
        {
            if (sounds->second.type != FVM_VALUE_OBJECT || !sounds->second.object ||
                sounds->second.object->owner != context)
            {
                return FVM_INVALID_ARGUMENT;
            }
            list = sounds->second.object;
        }
    }
    Properties::iterator source = frames->properties.find("sound_source");
    if (source == frames->properties.end())
    {
        return list ? FVM_INVALID_ARGUMENT : FVM_OK;
    }
    if (source == frames->properties.end() || source->second.type != FVM_VALUE_STRING || !source->second.string ||
        !context->swf_loaded || fvm_value_string_size(&source->second) % FVM_MOVIECLIP_SOUND_HEX_PER_BYTE ||
        fvm_value_string_size(&source->second) / FVM_MOVIECLIP_SOUND_HEX_PER_BYTE != context->swf_bytes.size())
    {
        return FVM_INVALID_ARGUMENT;
    }
    const char *digits = "0123456789abcdef";
    for (size_t i = 0; i < context->swf_bytes.size(); ++i)
    {
        FVM_Result charged = work.step();
        if (charged != FVM_OK)
        {
            return charged;
        }
        unsigned char byte = context->swf_bytes[i];
        if (source->second.string[i * FVM_MOVIECLIP_SOUND_HEX_PER_BYTE] !=
                digits[byte >> FVM_MOVIECLIP_SOUND_NIBBLE_SHIFT] ||
            source->second.string[i * FVM_MOVIECLIP_SOUND_HEX_PER_BYTE + 1] !=
                digits[byte & FVM_MOVIECLIP_SOUND_NIBBLE_MASK])
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    try
    {
        /* [GC:] The caller's native barrier retains source values through host callbacks.
         * Capture by ordinal rather than map insertion order and validate the entire batch
         * before publishing its first external effect. */
        if (list && list->indexed.size() > FVM_MOVIECLIP_SOUND_MAX_FRAME_EVENTS)
        {
            return FVM_INVALID_ARGUMENT;
        }
        const size_t count = list ? list->indexed.size() : 0;
        fvm_detail::vector<size_t> ordinals;
        for (size_t index = 0; index < count; ++index)
        {
            FVM_Result charged = work.step();
            if (charged != FVM_OK)
            {
                return charged;
            }
            Slots::const_iterator event = list->indexed.find((unsigned long)index);
            if (event == list->indexed.end() || event->second.type != FVM_VALUE_NUMBER ||
                !(event->second.number >= 0 &&
                  event->second.number < context->swf_bytes.size() / FVM_SWF_SOUND_EVENT_MIN_BYTES) ||
                event->second.number != std::floor(event->second.number))
            {
                return FVM_INVALID_ARGUMENT;
            }
            ordinals.push_back((size_t)event->second.number);
        }
        /* Validate membership, order and completeness in one canonical scan, not K rescans.
         * Missing tables on a sound-bound root are empty candidates, not a silent bypass. */
        fvm_detail::vector<size_t> canonical;
        FVM_Result checked = fvm_swf_sound_frame_ordinals(context, (unsigned int)frame, canonical);
        if (checked != FVM_OK)
        {
            return checked;
        }
        if (canonical.size() != ordinals.size())
        {
            return FVM_INVALID_ARGUMENT;
        }
        for (size_t index = 0; index < ordinals.size(); ++index)
        {
            checked = work.step();
            if (checked != FVM_OK)
            {
                return checked;
            }
            if (ordinals[index] != canonical[index])
            {
                return FVM_INVALID_ARGUMENT;
            }
        }
        if (ordinals.empty())
        {
            return FVM_OK;
        }
        if (!context->movieclip_sound_callback)
        {
            return FVM_UNSUPPORTED;
        }
        for (size_t index = 0; index < ordinals.size(); ++index)
        {
            FVM_Result charged = work.step();
            if (charged != FVM_OK)
            {
                return charged;
            }


            DeliveryScope scope(context);
            FVM_Result result =
                context->movieclip_sound_callback(context, clip, ordinals[index], context->movieclip_sound_userdata);
            if (result != FVM_OK)
            {
                return result;
            }
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
