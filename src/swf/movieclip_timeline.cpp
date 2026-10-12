/* FlashVM: bounded SWF root/sprite action timelines and linkage resources for MovieClip.
   Unimplemented scene/resource tags remain unsupported instead of being silently discarded. */
#include "decode.hpp"
#include "../core/native_call_guard.hpp"
#include "../standard/base_state.hpp"
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

/* Parsed timeline data contains offsets only; it owns no interpreter objects. */
struct Timeline
{
    unsigned int id;     /* Zero identifies the root; nonzero IDs identify sprites. */
    unsigned int frames; /* Declared number of frames. */
    fvm_detail::vector<ActionRange> actions;       /* Independent DoAction byte ranges. */
    fvm_detail::vector<fvm_detail::string> labels; /* Optional label at each frame. */
    /* Initialize a parsed timeline without allocating managed state. */
    Timeline() : id(0), frames(0)
    {
    }
};

/* Exported linkage name for a previously defined sprite. */
struct Export
{
    unsigned int id;         /* Sprite character ID. */
    fvm_detail::string name; /* NUL-terminated linkage name copied from SWF. */
};

/* Validate one root or sprite control stream, including every nested tag boundary. */
FVM_Result inspect_timeline(const fvm_detail::vector<unsigned char> &bytes, size_t at, size_t end,
                            unsigned int version, Timeline &timeline,
                            fvm_detail::vector<Timeline> *sprites,
                            fvm_detail::vector<Export> *exports)
{
    if (!timeline.frames)
        return FVM_INVALID_ARGUMENT;
    timeline.labels.resize(timeline.frames);
    fvm_detail::vector<unsigned char> labelled(timeline.frames);
    const unsigned char *p = bytes.begin();
    unsigned int frame = 1;
    while (at < end)
    {
        if (end - at < 2)
            return FVM_INVALID_ARGUMENT;
        unsigned int header = (unsigned int)little(p + at, 2);
        unsigned int tag = header >> 6;
        size_t count = header & 63;
        at += 2;
        if (count == 63)
        {
            if (end - at < 4)
                return FVM_INVALID_ARGUMENT;
            count = (size_t)little(p + at, 4);
            at += 4;
        }
        if (count > end - at)
            return FVM_INVALID_ARGUMENT;
        if (tag == 0)
            return count == 0 && at == end && frame == timeline.frames + 1 ? FVM_OK
                                                                           : FVM_INVALID_ARGUMENT;
        if (!sprites && (tag == 39 || tag == 56))
            return FVM_INVALID_ARGUMENT;
        if (tag == 39 && sprites)
        {
            if (version < 3 || count < 6)
                return FVM_INVALID_ARGUMENT;
            Timeline sprite;
            sprite.id = (unsigned int)little(p + at, 2);
            sprite.frames = (unsigned int)little(p + at + 2, 2);
            if (!sprite.id)
                return FVM_INVALID_ARGUMENT;
            for (size_t i = 0; i < sprites->size(); ++i)
                if ((*sprites)[i].id == sprite.id)
                    return FVM_INVALID_ARGUMENT;
            FVM_Result r = inspect_timeline(bytes, at + 4, at + count, version, sprite, 0, 0);
            if (r != FVM_OK)
                return r;
            sprites->push_back(sprite);
        }
        else if (tag == 56 && exports)
        {
            if (version < 5 || count < 2)
                return FVM_INVALID_ARGUMENT;
            unsigned int entries = (unsigned int)little(p + at, 2);
            size_t pos = at + 2;
            for (unsigned int i = 0; i < entries; ++i)
            {
                if (at + count - pos < 3)
                    return FVM_INVALID_ARGUMENT;
                Export asset;
                asset.id = (unsigned int)little(p + pos, 2);
                pos += 2;
                const unsigned char *zero =
                    (const unsigned char *)std::memchr(p + pos, 0, at + count - pos);
                if (!zero || zero == p + pos)
                    return FVM_INVALID_ARGUMENT;
                asset.name = (const char *)(p + pos);
                pos = (size_t)(zero - p) + 1;
                bool found = false;
                for (size_t j = 0; j < sprites->size(); ++j)
                    if ((*sprites)[j].id == asset.id)
                        found = true;
                if (!found)
                    return FVM_INVALID_ARGUMENT;
                exports->push_back(asset);
            }
            if (pos != at + count)
                return FVM_INVALID_ARGUMENT;
        }
        else
        {
            if (frame > timeline.frames)
                return FVM_INVALID_ARGUMENT;
            if (tag == 1)
            {
                if (count)
                    return FVM_INVALID_ARGUMENT;
                ++frame;
            }
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
                timeline.actions.push_back(range);
            }
            else if (tag == 43)
            {
                const unsigned char *zero = (const unsigned char *)std::memchr(p + at, 0, count);
                if (!zero || labelled[frame - 1])
                    return FVM_INVALID_ARGUMENT;
                size_t length = (size_t)(zero - (p + at));
                if (count != length + 1 && !(version >= 6 && count == length + 2 && zero[1] <= 1))
                    return FVM_INVALID_ARGUMENT;
                labelled[frame - 1] = 1;
                timeline.labels[frame - 1] = (const char *)(p + at);
            }
            else
                return FVM_UNSUPPORTED;
        }
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
/* [GC:] Represent an owned managed object as an edge in a frame or resource table. */
FVM_Value managed(FVM_Object *object)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    return value;
}

/* [GC:] Build frame records whose labels and action strings belong to the managed graph. */
FVM_Result build_frames(FVM_Context *c, const fvm_detail::vector<unsigned char> &bytes,
                        const Timeline &timeline, FVM_Object **out)
{
    FVM_Object *frames = 0;
    FVM_Result r = fvm_object_create(c, &frames);
    if (r != FVM_OK)
        return r;
    size_t next = 0;
    for (unsigned int frame = 1; frame <= timeline.frames; ++frame)
    {
        FVM_Object *record = 0, *blocks = 0;
        r = fvm_object_create(c, &record);
        if (r != FVM_OK)
            return r;
        r = fvm_object_create(c, &blocks);
        if (r != FVM_OK)
            return r;
        FVM_Value value;
        fvm_value_string(&value, "00");
        r = fvm_object_set(c, record, "code", &value);
        if (r != FVM_OK)
            return r;
        fvm_value_string(&value, timeline.labels[frame - 1].c_str());
        r = fvm_object_set(c, record, "label", &value);
        if (r != FVM_OK)
            return r;
        /* [GC:] The native-call barrier protects these objects until their frame links exist. */
        record->properties["segments"] = managed(blocks);
        frames->indexed[frame] = managed(record);
        unsigned long index = 0;
        while (next < timeline.actions.size() && timeline.actions[next].frame == frame)
        {
            const ActionRange &range = timeline.actions[next++];
            r = store_block(c, blocks, index++, bytes.begin() + range.offset, range.size);
            if (r != FVM_OK)
                return r;
        }
    }
    *out = frames;
    return FVM_OK;
}

/* [GC:] Build immutable sprite definitions and a live linkage/class library for instances. */
FVM_Result build_library(FVM_Context *c, const fvm_detail::vector<unsigned char> &bytes,
                         const fvm_detail::vector<Timeline> &sprites,
                         const fvm_detail::vector<Export> &exports, FVM_Object *clip)
{
    FVM_Object *library = 0, *definitions = 0, *names = 0;
    FVM_Result r = fvm_object_create(c, &library);
    if (r != FVM_OK)
        return r;
    r = fvm_object_create(c, &definitions);
    if (r != FVM_OK)
        return r;
    r = fvm_object_create(c, &names);
    if (r != FVM_OK)
        return r;
    for (size_t i = 0; i < sprites.size(); ++i)
    {
        FVM_Object *frames = 0, *definition = 0;
        r = build_frames(c, bytes, sprites[i], &frames);
        if (r != FVM_OK)
            return r;
        r = fvm_object_create(c, &definition);
        if (r != FVM_OK)
            return r;
        FVM_Value total;
        fvm_value_number(&total, sprites[i].frames);
        definition->properties["total"] = total;
        /* [GC:] Definitions share immutable frame records, not playback state. */
        definition->properties["frames"] = managed(frames);
        definitions->indexed[sprites[i].id] = managed(definition);
    }
    fvm_detail::map<unsigned long, fvm_detail::string> previous;
    for (size_t i = 0; i < exports.size(); ++i)
    {
        fvm_detail::map<unsigned long, fvm_detail::string>::iterator old =
            previous.find(exports[i].id);
        if (old != previous.end())
        {
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator binding =
                names->properties.find(old->second);
            if (binding != names->properties.end() &&
                binding->second.object == definitions->indexed[exports[i].id].object)
                names->properties.erase(old->second);
        }
        previous[exports[i].id] = exports[i].name;
        names->properties[exports[i].name] = definitions->indexed[exports[i].id];
    }
    FVM_Object *classes = 0;
    r = fvm_base_class_registry(c, &classes);
    if (r != FVM_OK && r != FVM_NOT_FOUND)
        return r;
    /* [GC:] The library retains the live class registry, builtin prototype and all definitions. */
    if (classes)
        library->properties["classes"] = managed(classes);
    library->properties["prototype"] = managed(clip->prototype);
    library->properties["sprites"] = managed(definitions);
    library->properties["exports"] = managed(names);
    /* [GC:] Publishing this edge makes the library travel with any owning clip. */
    clip->resource_result.object->properties["library"] = managed(library);
    return FVM_OK;
}
} // namespace

/* Decode and validate root/sprite timelines and exports before publishing a MovieClip. */
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
        Timeline main_timeline;
        main_timeline.frames = info.frame_count;
        fvm_detail::vector<Timeline> sprites;
        fvm_detail::vector<Export> exports;
        size_t start = 8 + (5 + 4 * (bytes[8] >> 3) + 7) / 8 + 4;
        r = inspect_timeline(bytes, start, bytes.size(), info.version, main_timeline, &sprites,
                             &exports);
        if (r != FVM_OK)
            return r;

        /* [GC:] Hold a native-call barrier until the unpublished clip owns its entire library. */
        FVM_NativeCallGuard guard(c);
        FVM_Object *clip = 0, *frames = 0;
        r = fvm_movieclip_create(c, name, info.frame_count, &clip);
        if (r != FVM_OK)
            return r;
        r = build_frames(c, bytes, main_timeline, &frames);
        if (r != FVM_OK)
            return r;
        /* [GC:] Replace the initially empty frame table with validated managed definitions. */
        clip->resource_result.object->properties["frames"] = managed(frames);
        r = build_library(c, bytes, sprites, exports, clip);
        if (r != FVM_OK)
            return r;
        *out = clip;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
