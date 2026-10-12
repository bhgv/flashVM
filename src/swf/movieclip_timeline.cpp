/* FlashVM: bounded SWF root action-timeline import into managed MovieClip frames.
   Scene/resource tags remain unsupported instead of being silently discarded. */
#include "decode.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/avm1.h"
#include "flashvm/table.h"
#include <cstring>
#include <new>

namespace
{
/* One validated action range in the canonical decoded file. */
struct ActionRange
{
    unsigned int frame; /* One-based destination frame. */
    size_t offset;      /* First action byte in the decoded SWF. */
    size_t size;        /* Validated action byte count. */
};

/* Read a little-endian unsigned integer without unaligned accesses. */
unsigned long little(const unsigned char *p, unsigned int n)
{
    unsigned long value = 0;
    for (unsigned int i = 0; i < n; ++i)
        value |= (unsigned long)p[i] << (8 * i);
    return value;
}

/* Validate every supported tag and action before allocating a MovieClip. */
FVM_Result inspect_timeline(const fvm_detail::vector<unsigned char> &bytes, const FVM_SWFInfo &info,
                            fvm_detail::vector<ActionRange> &actions,
                            fvm_detail::vector<fvm_detail::string> &labels)
{
    if (!info.frame_count)
        return FVM_INVALID_ARGUMENT;
    labels.resize(info.frame_count);
    fvm_detail::vector<unsigned char> labelled(info.frame_count);
    const unsigned char *p = bytes.begin();
    size_t at = 8 + (5 + 4 * (p[8] >> 3) + 7) / 8 + 4;
    unsigned int frame = 1;
    while (at < bytes.size())
    {
        unsigned int header = (unsigned int)little(p + at, 2);
        unsigned int tag = header >> 6;
        size_t count = header & 63;
        at += 2;
        if (count == 63)
        {
            count = (size_t)little(p + at, 4);
            at += 4;
        }
        /* fvm_swf_decode already checked all tag boundaries and the terminating End. */
        if (tag == 0)
            return frame == info.frame_count + 1 ? FVM_OK : FVM_INVALID_ARGUMENT;
        if (frame > info.frame_count)
            return FVM_INVALID_ARGUMENT;
        if (tag == 1)
            ++frame;
        else if (tag == 12)
        {
            if (!count || count > 65536)
                return FVM_INVALID_ARGUMENT;
            FVM_AVM1Info action_info;
            action_info.struct_size = sizeof(action_info);
            FVM_Result r = fvm_avm1_inspect(p + at, count, &action_info);
            if (r != FVM_OK)
                return r;
            ActionRange range;
            range.frame = frame;
            range.offset = at;
            range.size = count;
            actions.push_back(range);
        }
        else if (tag == 43)
        {
            const unsigned char *end = (const unsigned char *)std::memchr(p + at, 0, count);
            if (!end || labelled[frame - 1])
                return FVM_INVALID_ARGUMENT;
            size_t length = (size_t)(end - (p + at));
            if (count != length + 1 && !(info.version >= 6 && count == length + 2 && end[1] <= 1))
                return FVM_INVALID_ARGUMENT;
            labelled[frame - 1] = 1;
            labels[frame - 1] = (const char *)(p + at);
        }
        else
            return FVM_UNSUPPORTED;
        at += count;
    }
    return FVM_INVALID_ARGUMENT;
}

/* Encode an independent action block as a copied string owned by its managed container. */
FVM_Result store_block(FVM_Context *c, FVM_Object *blocks, unsigned long index,
                       const unsigned char *bytes, size_t size)
{
    const char *digits = "0123456789abcdef";
    fvm_detail::vector<char> hex;
    for (size_t i = 0; i < size; ++i)
    {
        hex.push_back(digits[bytes[i] >> 4]);
        hex.push_back(digits[bytes[i] & 15]);
    }
    hex.push_back(0);
    FVM_Value value;
    fvm_value_string(&value, hex.begin());
    return fvm_table_set_index(c, blocks, index, &value);
}
} // namespace

/* Decode and validate a root action timeline; publish only a fully initialized MovieClip. */
extern "C" FVM_Result fvm_movieclip_load_swf(FVM_Context *c, const void *input, size_t size,
                                             const char *name, FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || !name || !input)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::vector<unsigned char> bytes;
        FVM_SWFInfo info;
        info.struct_size = sizeof(info);
        unsigned long configured = c->runtime->config.memory_limit_bytes;
        size_t limit = configured ? (size_t)configured : (size_t)FVM_SWF_DEFAULT_LIMIT;
        FVM_Result r = fvm_swf_decode(input, size, limit, bytes, &info);
        if (r != FVM_OK)
            return r;
        fvm_detail::vector<ActionRange> actions;
        fvm_detail::vector<fvm_detail::string> labels;
        r = inspect_timeline(bytes, info, actions, labels);
        if (r != FVM_OK)
            return r;

        /* [GC:] Hold a native-call barrier until the unpublished clip owns every frame block. */
        FVM_NativeCallGuard guard(c);
        FVM_Object *clip = 0;
        r = fvm_movieclip_create(c, name, info.frame_count, &clip);
        if (r != FVM_OK)
            return r;
        /* [GC:] The clip's private state already owns its managed frame table. */
        FVM_Object *frames = clip->resource_result.object->properties["frames"].object;
        size_t next = 0;
        for (unsigned int frame = 1; frame <= info.frame_count; ++frame)
        {
            r = fvm_movieclip_define_frame(c, clip, frame, labels[frame - 1].c_str(), 0, 0);
            if (r != FVM_OK)
                return r;
            FVM_Object *blocks = 0;
            r = fvm_object_create(c, &blocks);
            if (r != FVM_OK)
                return r;
            FVM_Value value;
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = blocks;
            /* [GC:] Link the segment container into the frame before copying its action strings. */
            frames->indexed[frame].object->properties["segments"] = value;
            unsigned long index = 0;
            while (next < actions.size() && actions[next].frame == frame)
            {
                const ActionRange &range = actions[next++];
                r = store_block(c, blocks, index++, bytes.begin() + range.offset, range.size);
                if (r != FVM_OK)
                    return r;
            }
        }
        *out = clip;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
