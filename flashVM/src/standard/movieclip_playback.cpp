/* Intrinsic scalar timeline observation for native hosts, bypassing all script properties. */
#include "flashvm/movieclip.h"
#include "movieclip_state.hpp"
#include "movieclip_storage.hpp"
#include <cstring>
#include <cmath>
#include <new>


/* Compare stored keys directly: no temporary owning string or allocation in an observation. */
static bool playback_integer(FVM_Object *data, const char *name, unsigned int limit, unsigned int &out)
{
    typedef fvm_movieclip_storage::Properties Properties;
    for (Properties::const_iterator i = data->properties.begin(); i != data->properties.end(); ++i)
    {
        if (i->first.size() != std::strlen(name) || std::memcmp(i->first.data(), name, i->first.size()))
        {
            continue;
        }
        const FVM_Value &value = i->second;
        if (value.type != FVM_VALUE_NUMBER || !(value.number >= 0 && value.number <= limit) ||
            value.number != std::floor(value.number))
        {
            return false;
        }
        out = (unsigned int)value.number;
        return true;
    }
    return false;
}


/* [GC:] Read retained private state on its owner; copy only validated scalars, never managed edges.
 */
extern "C" FVM_Result fvm_movieclip_get_playback_info(FVM_Context *c, FVM_Object *clip, FVM_MovieClipPlaybackInfo *out)
{
    if (!c || !clip || clip->owner != c || !out || out->struct_size != sizeof(*out) || c->destroying ||
        c->gc_collecting || c->gc_releasing || fvm_movieclip_is_class_prototype(clip))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *data = 0;
    if (fvm_movieclip_storage::state(clip, data) != FVM_OK || data->owner != c)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_MovieClipPlaybackInfo next;
    std::memset(&next, 0, sizeof(next));
    next.struct_size = sizeof(next);
    unsigned int playing = 0, entered = 0;
    if (!playback_integer(data, "frame", FVM_MOVIECLIP_MAX_FRAMES, next.frame) ||
        !playback_integer(data, "total", FVM_MOVIECLIP_MAX_FRAMES, next.total_frames) ||
        !playback_integer(data, "playing", 1, playing) || !playback_integer(data, "entered", 1, entered) ||
        !next.frame || !next.total_frames || next.frame > next.total_frames)
    {
        return FVM_INVALID_ARGUMENT;
    }
    next.playing = (int)playing;
    next.entered = (int)entered;
    *out = next;
    return FVM_OK;
}


/* Locate the optional legacy-compatible token without constructing a temporary owning key. */
static FVM_Result playback_navigation(FVM_Object *data, unsigned long &out)
{
    typedef fvm_movieclip_storage::Properties Properties;
    for (Properties::const_iterator i = data->properties.begin(); i != data->properties.end(); ++i)
    {
        if (i->first.size() != sizeof(FVM_MOVIECLIP_NAVIGATION_KEY) - 1 ||
            std::memcmp(i->first.data(), FVM_MOVIECLIP_NAVIGATION_KEY, i->first.size()))
        {
            continue;
        }
        const FVM_Value &value = i->second;
        if (value.type != FVM_VALUE_NUMBER || !(value.number >= 0 && value.number <= FVM_MOVIECLIP_NAVIGATION_MAX) ||
            value.number != std::floor(value.number))
        {
            return FVM_INVALID_ARGUMENT;
        }
        out = (unsigned long)value.number;
        return FVM_OK;
    }
    out = 0;
    return FVM_OK;
}


/* [GC:] Publish only a numeric private value; the existing enclosing native seek barrier retains
 * data. */
FVM_Result fvm_movieclip_note_navigation(FVM_Object *data)
{
    unsigned long previous = 0;
    FVM_Result result = playback_navigation(data, previous);
    if (result != FVM_OK)
    {
        return result;
    }
    try
    {
        data->properties[FVM_MOVIECLIP_NAVIGATION_KEY] =
            fvm_movieclip_storage::number(previous == FVM_MOVIECLIP_NAVIGATION_MAX ? 1 : previous + 1);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}


/* [GC:] Validate native ownership/lifecycle through the scalar query, then copy no managed edges.
 */
extern "C" FVM_Result fvm_movieclip_get_navigation(FVM_Context *context, FVM_Object *clip, unsigned long *out)
{
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_MovieClipPlaybackInfo info;
    info.struct_size = sizeof(info);
    FVM_Result result = fvm_movieclip_get_playback_info(context, clip, &info);
    if (result != FVM_OK)
    {
        return result;
    }
    FVM_Object *data = clip->resource_result.object;
    unsigned long token = 0;
    result = playback_navigation(data, token);
    if (result == FVM_OK)
    {
        *out = token;
    }
    return result;
}
