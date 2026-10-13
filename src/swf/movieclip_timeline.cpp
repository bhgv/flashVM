/* FlashVM: bounded SWF root/sprite action timelines and linkage resources for MovieClip.
   Unimplemented scene/resource tags remain unsupported instead of being silently discarded. */
#include "decode.hpp"
#include "document.hpp"
#include "reader.hpp"
#include "shape_graph.hpp"
#include "bitmap.hpp"
#include "../standard/bitmap_state.hpp"
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

/* One decoded placement/removal operation, preserving optional-field presence. */
struct Placement
{
    unsigned int frame;          /* One-based frame containing this control. */
    unsigned int tag;            /* Placement/removal or SetBackgroundColor control code. */
    unsigned int flags;          /* PlaceObject2 field-presence and move flags. */
    unsigned int depth;          /* Raw positive SWF depth; public depth subtracts 16384. */
    unsigned int character;      /* Defined character ID when supplied by this operation. */
    unsigned long token;         /* Stable creation identity within this timeline. */
    unsigned int ratio;          /* Optional morph ratio, preserved for character-specific use. */
    unsigned long background;   /* RGB24 payload for SetBackgroundColor, without a display depth. */
    FVM_Affine matrix;           /* Decoded pixel-space affine transform. */
    FVM_SWFColorTransform color; /* Optional RGBA color transform. */
    fvm_detail::string name;     /* Optional instance name. */
    /* Initialize omitted placement fields without inventing presence flags. */
    Placement() : frame(0), tag(0), flags(0), depth(0), character(0), token(0), ratio(0), background(0)
    {
        matrix.xx = matrix.yy = 1;
        matrix.xy = matrix.yx = matrix.x0 = matrix.y0 = 0;
    }
};

/* Decode supported display-list fields, rejecting trailing bytes and unknown characters. */
FVM_Result inspect_placement(const unsigned char *bytes, size_t size, unsigned int tag,
                             unsigned int version,
                             const fvm_detail::map<unsigned long, bool> &known, Placement &out)
{
    FVM_SWFReader reader(bytes, size);
    out.tag = tag;
    if (tag == 5 || tag == 28)
    {
        if (tag == 5)
            out.character = (unsigned int)reader.integer(2);
        out.depth = (unsigned int)reader.integer(2);
        if (!reader.finished() || !out.depth ||
            (tag == 5 && known.find(out.character) == known.end()))
            return FVM_INVALID_ARGUMENT;
        return FVM_OK;
    }
    if (tag == 4)
    {
        out.flags = 6;
        out.character = (unsigned int)reader.integer(2);
        out.depth = (unsigned int)reader.integer(2);
        if (!fvm_swf_read_matrix(reader, out.matrix))
            return FVM_INVALID_ARGUMENT;
        if (!reader.finished())
        {
            out.flags |= 8;
            if (!fvm_swf_read_color(reader, false, out.color))
                return FVM_INVALID_ARGUMENT;
        }
    }
    else
    {
        if (version < 3)
            return FVM_INVALID_ARGUMENT;
        out.flags = (unsigned int)reader.integer(1);
        out.depth = (unsigned int)reader.integer(2);
        if (!(out.flags & 3))
            return FVM_INVALID_ARGUMENT;
        if (out.flags & 0xc0)
            return FVM_UNSUPPORTED;
        if (out.flags & 2)
            out.character = (unsigned int)reader.integer(2);
        if ((out.flags & 4) && !fvm_swf_read_matrix(reader, out.matrix))
            return FVM_INVALID_ARGUMENT;
        if ((out.flags & 8) && !fvm_swf_read_color(reader, true, out.color))
            return FVM_INVALID_ARGUMENT;
        if (out.flags & 16)
            out.ratio = (unsigned int)reader.integer(2);
        if ((out.flags & 32) && !reader.string(out.name))
            return FVM_INVALID_ARGUMENT;
    }
    if (!reader.finished() || !out.depth ||
        ((out.flags & 2) && known.find(out.character) == known.end()))
        return FVM_INVALID_ARGUMENT;
    return FVM_OK;
}

/* Parsed timeline data contains offsets only; it owns no interpreter objects. */
struct Timeline
{
    unsigned int id;     /* Zero identifies the root; nonzero IDs identify sprites. */
    unsigned int frames; /* Declared number of frames. */
    FVM_SWFDocument document; /* Borrowed root metadata copied into managed frame storage later. */
    fvm_detail::vector<Placement> controls;  /* Ordered display-list controls for all frames. */
    fvm_detail::vector<ActionRange> actions; /* Independent DoAction byte ranges. */
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
                            fvm_detail::vector<Export> *exports,
                            fvm_detail::vector<FVM_SWFShape> *shapes,
                            fvm_detail::vector<FVM_SWFBitmap> *bitmaps, size_t bitmap_limit,
                            fvm_detail::map<unsigned long, bool> &known)
{
    if (!timeline.frames)
        return FVM_INVALID_ARGUMENT;
    timeline.labels.resize(timeline.frames);
    fvm_detail::vector<unsigned char> labelled(timeline.frames);
    const unsigned char *p = bytes.begin();
    unsigned int frame = 1;
    bool first_tag = true; /* FileAttributes must be the first root tag. */
    unsigned long token = 0;
    fvm_detail::vector<unsigned char> jpeg_tables;
    bool have_jpeg_tables = false;
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
        bool document_first = first_tag;
        first_tag = false;
        if (tag == 0)
        {
            if (count || at != end || frame != timeline.frames + 1)
                return FVM_INVALID_ARGUMENT;
            return fvm_swf_document_finish(timeline.document);
        }
        if (!sprites && (tag == 39 || tag == 56 || tag == 2 || tag == 22 || tag == 32 ||
                         tag == 20 || tag == 36 || tag == 21 || tag == 35 || tag == 6 || tag == 8))
            return FVM_INVALID_ARGUMENT;
        if (tag == 69 || tag == 77)
        {
            if (!sprites)
                return FVM_INVALID_ARGUMENT;
            FVM_Result r = fvm_swf_document_tag(tag, p + at, count, document_first, timeline.document);
            if (r != FVM_OK)
                return r;
        }
        else if (tag == 8 && bitmaps)
        {
            if (have_jpeg_tables)
                return FVM_INVALID_ARGUMENT;
            FVM_Result r = fvm_swf_read_jpeg_tables(p + at, count, version, jpeg_tables);
            if (r != FVM_OK)
                return r;
            have_jpeg_tables = true;
        }
        else if ((tag == 20 || tag == 36 || tag == 21 || tag == 35 || tag == 6) && bitmaps)
        {
            if (version < (tag == 6 ? 1U : tag == 20 || tag == 21 ? 2U : 3U))
                return FVM_INVALID_ARGUMENT;
            size_t used = 0;
            for (size_t i = 0; i < bitmaps->size(); ++i)
                used += (*bitmaps)[i].pixels.size();
            if (used >= bitmap_limit)
                return FVM_OUT_OF_MEMORY;
            FVM_SWFBitmap bitmap;
            FVM_Result r =
                tag == 6 ? fvm_swf_read_legacy_jpeg(p + at, count, jpeg_tables, version,
                                                    bitmap_limit - used, bitmap)
                : tag == 21 || tag == 35
                    ? fvm_swf_read_jpeg(p + at, count, tag, version, bitmap_limit - used, bitmap)
                    : fvm_swf_read_bitmap(p + at, count, tag, bitmap_limit - used, bitmap);
            if (r != FVM_OK)
                return r;
            if (known.find(bitmap.id) != known.end())
                return FVM_INVALID_ARGUMENT;
            bitmaps->push_back(bitmap);
            known[bitmap.id] = true;
        }
        else if ((tag == 2 || tag == 22 || tag == 32) && shapes)
        {
            if ((tag == 22 && version < 2) || (tag == 32 && version < 3))
                return FVM_INVALID_ARGUMENT;
            FVM_SWFShape shape;
            FVM_Result r = fvm_swf_read_shape(p + at, count, tag, shape);
            if (r != FVM_OK)
                return r;
            for (size_t i = 0; i < shape.paints.size(); ++i)
                if (shape.paints[i].bitmap_type)
                {
                    if (version < 8 && shape.paints[i].bitmap_type >= 0x42)
                        return FVM_INVALID_ARGUMENT;
                    bool found = false;
                    for (size_t j = 0; bitmaps && j < bitmaps->size(); ++j)
                        if ((*bitmaps)[j].id == shape.paints[i].bitmap_id)
                            found = true;
                    if (!found)
                        return FVM_INVALID_ARGUMENT;
                }
            if (known.find(shape.id) != known.end())
                return FVM_INVALID_ARGUMENT;
            shapes->push_back(shape);
            known[shape.id] = true;
        }
        else if (tag == 39 && sprites)
        {
            if (version < 3 || count < 6)
                return FVM_INVALID_ARGUMENT;
            Timeline sprite;
            sprite.id = (unsigned int)little(p + at, 2);
            sprite.frames = (unsigned int)little(p + at + 2, 2);
            if (!sprite.id)
                return FVM_INVALID_ARGUMENT;
            if (known.find(sprite.id) != known.end())
                return FVM_INVALID_ARGUMENT;
            FVM_Result r = inspect_timeline(bytes, at + 4, at + count, version, sprite, 0, 0, 0, 0,
                                            bitmap_limit, known);
            if (r != FVM_OK)
                return r;
            sprites->push_back(sprite);
            known[sprite.id] = true;
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
                if (known.find(asset.id) == known.end())
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
            else if (tag == 4 || tag == 5 || tag == 26 || tag == 28)
            {
                Placement control;
                FVM_Result r = inspect_placement(p + at, count, tag, version, known, control);
                if (r != FVM_OK)
                    return r;
                control.frame = frame;
                if (control.flags & 2)
                    control.token = ++token;
                timeline.controls.push_back(control);
            }
            else if (tag == 9)
            {
                if (count != 3)
                    return FVM_INVALID_ARGUMENT;
                Placement control;
                control.frame = frame;
                control.tag = tag;
                control.background = ((unsigned long)p[at] << 16) |
                                     ((unsigned long)p[at + 1] << 8) | p[at + 2];
                timeline.controls.push_back(control);
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

/* Copy matrix/color components into private scalar records for graph transport. */
FVM_Result component_record(FVM_Context *c, const char *const *keys, const double *values,
                            size_t count, FVM_Object **out)
{
    FVM_Object *record = 0;
    FVM_Result r = fvm_object_create(c, &record);
    if (r != FVM_OK)
        return r;
    for (size_t i = 0; i < count; ++i)
    {
        FVM_Value value;
        fvm_value_number(&value, values[i]);
        record->properties[keys[i]] = value;
    }
    *out = record;
    return FVM_OK;
}

/* [GC:] Store one control and its symbol/matrix/color edges in the owning frame table. */
FVM_Result build_control(FVM_Context *c, const Placement &control, FVM_Object *definitions,
                         FVM_Object **out)
{
    FVM_Object *record = 0;
    FVM_Result r = fvm_object_create(c, &record);
    if (r != FVM_OK)
        return r;
    const char *fields[] = {"op", "flags", "depth", "token", "character", "ratio", "background"};
    const unsigned long values[] = {control.tag,   control.flags,     control.depth,
                                    control.token, control.character, control.ratio, control.background};
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
    {
        FVM_Value value;
        fvm_value_number(&value, values[i]);
        record->properties[fields[i]] = value;
    }
    /* [GC:] The symbol edge retains its definition without a process-local dictionary pointer. */
    if (control.flags & 2)
        record->properties["symbol"] = definitions->indexed[control.character];
    if (control.flags & 4)
    {
        const char *keys[] = {"xx", "yx", "xy", "yy", "x0", "y0"};
        const double values[] = {control.matrix.xx, control.matrix.yx, control.matrix.xy,
                                 control.matrix.yy, control.matrix.x0, control.matrix.y0};
        FVM_Object *matrix = 0;
        r = component_record(c, keys, values, 6, &matrix);
        if (r != FVM_OK)
            return r;
        /* [GC:] Retain the immutable affine record with this control. */
        record->properties["matrix"] = managed(matrix);
    }
    if (control.flags & 8)
    {
        const char *keys[] = {"rm", "gm", "bm", "am", "ra", "ga", "ba", "aa"};
        double values[8];
        for (unsigned int i = 0; i < 4; ++i)
        {
            values[i] = control.color.multiply[i];
            values[i + 4] = control.color.add[i];
        }
        FVM_Object *color = 0;
        r = component_record(c, keys, values, 8, &color);
        if (r != FVM_OK)
            return r;
        /* [GC:] Retain color data with the control for later scene application. */
        record->properties["color"] = managed(color);
    }
    if (control.flags & 32)
    {
        FVM_Value name;
        fvm_value_string(&name, control.name.c_str());
        r = fvm_object_set(c, record, "name", &name);
        if (r != FVM_OK)
            return r;
    }
    *out = record;
    return FVM_OK;
}

/* [GC:] Build frame records whose labels and action strings belong to the managed graph. */
FVM_Result build_frames(FVM_Context *c, const fvm_detail::vector<unsigned char> &bytes,
                        const Timeline &timeline, FVM_Object *definitions, FVM_Object **out)
{
    FVM_Object *frames = 0;
    FVM_Result r = fvm_object_create(c, &frames);
    if (r != FVM_OK)
        return r;
    /* [GC:] Store scalar flags and owned UTF-8 in the graph, without executing metadata. */
    if (timeline.document.attributes_present)
    {
        FVM_Value value;
        fvm_value_number(&value, timeline.document.attributes);
        frames->properties["swf_file_attributes"] = value;
    }
    if (timeline.document.metadata_present)
    {
        FVM_Value value;
        r = fvm_value_string_bytes(&value, timeline.document.metadata, timeline.document.metadata_size);
        if (r == FVM_OK)
            r = fvm_object_set(c, frames, "swf_metadata", &value);
        if (r != FVM_OK)
            return r;
    }
    size_t next = 0, next_control = 0;
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
        FVM_Object *controls = 0;
        r = fvm_object_create(c, &controls);
        if (r != FVM_OK)
            return r;
        /* [GC:] Controls travel with the frame and retain their referenced symbols. */
        record->properties["controls"] = managed(controls);
        while (next_control < timeline.controls.size() &&
               timeline.controls[next_control].frame == frame)
        {
            FVM_Object *control = 0;
            r = build_control(c, timeline.controls[next_control++], definitions, &control);
            if (r != FVM_OK)
                return r;
            /* [GC:] Publish the control in input order before creating the next one. */
            controls->indexed[(unsigned long)controls->indexed.size()] = managed(control);
        }
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
                         const fvm_detail::vector<Export> &exports,
                         const fvm_detail::vector<FVM_SWFShape> &shapes,
                         const fvm_detail::vector<FVM_SWFBitmap> &bitmaps, FVM_Object *clip)
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
    /* [GC:] Allocate all definition identities before installing any inter-definition edges. */
    for (size_t i = 0; i < sprites.size(); ++i)
    {
        FVM_Object *definition = 0;
        r = fvm_object_create(c, &definition);
        if (r != FVM_OK)
            return r;
        FVM_Value total, id;
        fvm_value_number(&total, sprites[i].frames);
        fvm_value_number(&id, sprites[i].id);
        definition->properties["total"] = total;
        definition->properties["id"] = id;
        definitions->indexed[sprites[i].id] = managed(definition);
    }
    for (size_t i = 0; i < bitmaps.size(); ++i)
    {
        /* [GC:] Allocate the dictionary holder before linking bitmap identity into shape fills. */
        FVM_Object *definition = 0;
        r = fvm_object_create(c, &definition);
        if (r != FVM_OK)
            return r;
        FVM_Value pixels;
        r = fvm_bitmap_create_pixels(c, bitmaps[i].width, bitmaps[i].height, bitmaps[i].transparent,
                                     bitmaps[i].pixels, &pixels);
        if (r != FVM_OK)
            return r;
        /* [GC:] The dictionary owns the decoded bitmap before any fills reference it. */
        definition->properties["bitmap_data"] = pixels;
        /* [GC:] Retain each resource holder while constructing its optional direct-display
         * geometry. */
        definitions->indexed[bitmaps[i].id] = managed(definition);
        FVM_SWFShape geometry;
        geometry.id = bitmaps[i].id;
        geometry.bounds.x_min = geometry.bounds.y_min = 0;
        geometry.bounds.x_max = bitmaps[i].width;
        geometry.bounds.y_max = bitmaps[i].height;
        FVM_SWFShapePaint paint;
        paint.bitmap_id = bitmaps[i].id;
        paint.bitmap_type = 0x41;
        const double xy[] = {0,
                             0,
                             (double)bitmaps[i].width,
                             0,
                             (double)bitmaps[i].width,
                             (double)bitmaps[i].height,
                             0,
                             (double)bitmaps[i].height};
        for (unsigned j = 0; j < 8; j += 2)
        {
            FVM_PathCommand command;
            std::memset(&command, 0, sizeof(command));
            command.verb = j ? FVM_PATH_LINE : FVM_PATH_MOVE;
            command.x[0] = xy[j];
            command.y[0] = xy[j + 1];
            paint.commands.push_back(command);
        }
        FVM_PathCommand close;
        std::memset(&close, 0, sizeof(close));
        close.verb = FVM_PATH_CLOSE;
        paint.commands.push_back(close);
        geometry.paints.push_back(paint);
        r = fvm_swf_shape_build(c, geometry, &definition, definitions);
        if (r != FVM_OK)
            return r;
        /* [GC:] Keep the resource bitmap accessible independently of its static paint descriptor.
         */
        definition->properties["bitmap_data"] = pixels;
        /* [GC:] Publish the complete static definition with the same bitmap resource identity. */
        definitions->indexed[bitmaps[i].id] = managed(definition);
    }
    for (size_t i = 0; i < shapes.size(); ++i)
    {
        FVM_Object *definition = 0;
        r = fvm_swf_shape_build(c, shapes[i], &definition, definitions);
        if (r != FVM_OK)
            return r;
        /* [GC:] The dictionary owns the managed solid vector resource. */
        definitions->indexed[shapes[i].id] = managed(definition);
    }
    for (size_t i = 0; i < sprites.size(); ++i)
    {
        FVM_Object *frames = 0;
        r = build_frames(c, bytes, sprites[i], definitions, &frames);
        if (r != FVM_OK)
            return r;
        /* [GC:] A definition owns its frames, which may refer to earlier definitions. */
        definitions->indexed[sprites[i].id].object->properties["frames"] = managed(frames);
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
    for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i = names->properties.begin();
         i != names->properties.end(); ++i)
    {
        FVM_Value linkage;
        fvm_value_string(&linkage, i->first.c_str());
        r = fvm_object_set(c, i->second.object, "linkage", &linkage);
        if (r != FVM_OK)
            return r;
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
        fvm_detail::vector<FVM_SWFShape> shapes;
        fvm_detail::vector<FVM_SWFBitmap> bitmaps;
        size_t start = 8 + (5 + 4 * (bytes[8] >> 3) + 7) / 8 + 4;
        fvm_detail::map<unsigned long, bool> known;
        r = inspect_timeline(bytes, start, bytes.size(), info.version, main_timeline, &sprites,
                             &exports, &shapes, &bitmaps, limit, known);
        if (r != FVM_OK)
            return r;

        /* [GC:] Hold a native-call barrier until the unpublished clip owns its entire library. */
        FVM_NativeCallGuard guard(c);
        FVM_Object *clip = 0, *frames = 0;
        r = fvm_movieclip_create(c, name, info.frame_count, &clip);
        if (r != FVM_OK)
            return r;
        r = build_library(c, bytes, sprites, exports, shapes, bitmaps, clip);
        if (r != FVM_OK)
            return r;
        /* [GC:] The root frame controls refer to the same dictionary identities as sprites. */
        FVM_Object *definitions = clip->resource_result.object->properties["library"]
                                      .object->properties["sprites"]
                                      .object;
        r = build_frames(c, bytes, main_timeline, definitions, &frames);
        if (r != FVM_OK)
            return r;
        /* [GC:] Replace the initially empty frame table with fully linked managed definitions. */
        clip->resource_result.object->properties["frames"] = managed(frames);
        *out = clip;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Read neutral document records from private frame storage, without invoking script getters. */
extern "C" FVM_Result fvm_movieclip_swf_document_info(FVM_Context *c, FVM_Object *clip,
                                                       FVM_SWFDocumentInfo *out)
{
    if (!c || !clip || clip->owner != c || clip->resource_kind != 40 ||
        !clip->resource_result_valid || clip->resource_result.type != FVM_VALUE_OBJECT ||
        !clip->resource_result.object || clip->resource_result.object->owner != c ||
        !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_Object *state = clip->resource_result.object;
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator found = state->properties.find("frames");
        if (found == state->properties.end() || found->second.type != FVM_VALUE_OBJECT ||
            !found->second.object || found->second.object->owner != c)
            return FVM_INVALID_ARGUMENT;
        FVM_Object *frames = found->second.object;
        FVM_SWFDocumentInfo result;
        result.struct_size = sizeof(result);
        result.file_attributes_present = 0;
        result.file_attributes = 0;
        fvm_value_undefined(&result.metadata);
        found = frames->properties.find("swf_file_attributes");
        if (found != frames->properties.end())
        {
            if (found->second.type != FVM_VALUE_NUMBER ||
                (found->second.number != 0 && found->second.number != 16))
                return FVM_INVALID_ARGUMENT;
            result.file_attributes_present = 1;
            result.file_attributes = (unsigned long)found->second.number;
        }
        found = frames->properties.find("swf_metadata");
        if (found != frames->properties.end())
        {
            if (found->second.type != FVM_VALUE_STRING || result.file_attributes != 16 ||
                fvm_validate_value(c, &found->second) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
            /* [GC:] The owning frame graph retains this counted string until its host root is released. */
            result.metadata = found->second;
        }
        else if (result.file_attributes == 16)
            return FVM_INVALID_ARGUMENT;
        *out = result;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
