/* FlashVM: strict bounded ClipAction decoding; native state contains no managed pointers. */
#ifndef FVM_SWF_CLIP_ACTIONS_HPP
#define FVM_SWF_CLIP_ACTIONS_HPP /* Guard decoded ClipAction records and parser. */
#include "reader.hpp"
#include "clip_action_policy.hpp"
#include "../standard/movieclip_constants.h"
#include "../avm1/block.hpp"
#include "flashvm/clip_actions.h"
#include "../details/vector.hpp"


/* Own one validated action range independently of the input file's borrowed storage. */
struct FVM_SWFClipAction
{
    unsigned long events;                   /* Nonzero supported source event mask. */
    unsigned int key;                       /* Raw selector used only when KEY_PRESS is present. */
    fvm_detail::vector<unsigned char> code; /* Owned, structurally inspected AVM1 bytes. */


    /* Initialize selectors before populating decoded bytes. */
    FVM_SWFClipAction() : events(0), key(0)
    {
    }
};


/* Decode the final placement field; sticky reader errors cannot escape the record boundary. */
inline FVM_Result fvm_swf_read_clip_actions(FVM_SWFReader &reader,
                                            const unsigned char *bytes,
                                            unsigned int version,
                                            fvm_detail::vector<FVM_SWFClipAction> &out)
{
    if (version < FVM_SWF_CLIP_ACTION_MIN_VERSION)
    {
        return FVM_UNSUPPORTED;
    }
    if (reader.integer(2) || !reader.good())
    {
        return FVM_INVALID_ARGUMENT;
    }
    const unsigned int width = version < FVM_SWF_CLIP_ACTION_SHORT_FLAGS_VERSION ? FVM_SWF_CLIP_ACTION_SHORT_FLAG_BYTES
                                                                                 : FVM_SWF_CLIP_ACTION_LONG_FLAG_BYTES;
    const unsigned long all = reader.integer(width), allowed = fvm_swf_clip_action_mask(version);
    if (!reader.good() || (all & ~FVM_SWF_CLIP_ACTION_KNOWN_MASK))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (all & ~allowed)
    {
        return FVM_UNSUPPORTED;
    }
    unsigned long seen = 0, total = 0;
    for (;;)
    {
        FVM_SWFClipAction action;
        action.events = reader.integer(width);
        if (!reader.good())
        {
            return FVM_INVALID_ARGUMENT;
        }
        if (!action.events)
        {
            return seen == all ? FVM_OK : FVM_INVALID_ARGUMENT;
        }
        if ((action.events & ~all) || out.size() >= FVM_SWF_CLIP_ACTION_MAX_RECORDS)
        {
            return FVM_INVALID_ARGUMENT;
        }
        unsigned long length = reader.integer(4);
        if (action.events & FVM_CLIP_EVENT_KEY_PRESS)
        {
            if (!length)
            {
                return FVM_INVALID_ARGUMENT;
            }
            action.key = (unsigned int)reader.integer(1);
            --length;
        }
        if (!reader.good() || length > FVM_MOVIECLIP_MAX_FRAME_ACTION_BYTES ||
            length > FVM_SWF_CLIP_ACTION_MAX_BYTES - total)
        {
            return FVM_INVALID_ARGUMENT;
        }
        size_t at = reader.position();
        if (!reader.skip((size_t)length))
        {
            return FVM_INVALID_ARGUMENT;
        }
        FVM_AVM1Info info;
        info.struct_size = sizeof(info);
        FVM_Result result = fvm_avm1_inspect_block(bytes + at, (size_t)length, &info);
        if (result != FVM_OK)
        {
            return result;
        }
        for (unsigned long i = 0; i < length; ++i)
        {
            action.code.push_back(bytes[at + i]);
        }
        total += length;
        seen |= action.events;
        out.push_back(action);
    }
}
#endif
