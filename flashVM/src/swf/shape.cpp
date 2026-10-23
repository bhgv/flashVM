/* FlashVM: bounded SWF shape decoding and directed-edge assembly for owned vector paints. */
#include "format_constants.h"
#include "shape.hpp"
#include "reader.hpp"
#include "../details/hash_table.hpp"
#include <cstdio>
#include <cstring>
#include <new>

namespace
{
/* Native edge geometry uses exact integer twips until final command construction. */
struct Edge
{
    long x0, y0; /* Start point in twips. */
    long cx, cy; /* Quadratic control point, ignored for a straight edge. */
    long x1, y1; /* End point in twips. */
    bool curve;  /* Whether this is a quadratic rather than straight edge. */
    bool begin;  /* Explicit move/style boundary that must break a stroke contour. */
    /* Initialize an empty straight edge. */
    Edge() : x0(0), y0(0), cx(0), cy(0), x1(0), y1(0), curve(false), begin(false)
    {
    }
};
typedef fvm_detail::vector<Edge> Edges;
typedef fvm_detail::map<unsigned long, Edges> Groups;

/* Enforce the common path coordinate domain before adding signed twip deltas. */
bool coordinate(long n)
{
    return n >= -20000000L && n <= 20000000L;
}

/* Read one RGB/RGBA solid style without borrowing bytes from the source file. */
bool color(FVM_SWFReader &reader, bool rgba, FVM_SWFShapePaint &paint)
{
    unsigned long r = reader.integer(1), g = reader.integer(1), b = reader.integer(1);
    paint.rgb = (r << 16) | (g << 8) | b;
    paint.alpha = rgba ? reader.integer(1) * 100.0 / 255 : 100;
    return reader.good();
}

/* Decode versioned gradient modes and ordered stops without relaxing legacy reserved bits. */
FVM_Result gradient(FVM_SWFReader &reader, unsigned int tag, FVM_SWFShapePaint &paint)
{
    if (!fvm_swf_read_matrix(reader, paint.gradient_matrix))
        return FVM_INVALID_ARGUMENT;
    unsigned long header = reader.integer(1);
    unsigned long count = header;
    if (tag == FVM_SWF_TAG_DEFINE_SHAPE4)
    {
        paint.gradient_spread = (unsigned int)(header >> FVM_SWF_GRADIENT_SPREAD_SHIFT);
        paint.gradient_interpolation = (unsigned int)(
            (header >> FVM_SWF_GRADIENT_INTERPOLATION_SHIFT) & FVM_SWF_GRADIENT_MODE_MASK);
        count = header & FVM_SWF_GRADIENT_COUNT_MASK;
        if (paint.gradient_spread > FVM_GRADIENT_SPREAD_REPEAT ||
            paint.gradient_interpolation > FVM_GRADIENT_INTERPOLATION_LINEAR_RGB)
            return FVM_INVALID_ARGUMENT;
    }
    unsigned long limit = tag == FVM_SWF_TAG_DEFINE_SHAPE4
                              ? FVM_GRADIENT_MAX_STOPS : FVM_SWF_LEGACY_GRADIENT_MAX_STOPS;
    if (!reader.good() || count < 1 || count > limit)
        return FVM_INVALID_ARGUMENT;
    for (unsigned long i = 0; i < count; ++i)
    {
        FVM_SWFGradientStop stop;
        stop.ratio = (unsigned int)reader.integer(1);
        stop.color.r = (unsigned char)reader.integer(1);
        stop.color.g = (unsigned char)reader.integer(1);
        stop.color.b = (unsigned char)reader.integer(1);
        stop.color.a = (tag == FVM_SWF_TAG_DEFINE_SHAPE3 || tag == FVM_SWF_TAG_DEFINE_SHAPE4) ? (unsigned char)reader.integer(1) : 255;
        if (!reader.good() || (i && stop.ratio < paint.stops[i - 1].ratio))
            return FVM_INVALID_ARGUMENT;
        paint.stops.push_back(stop);
    }
    if (paint.gradient_type == FVM_SWF_FILL_FOCAL_GRADIENT)
    {
        unsigned long raw = reader.integer(2);
        if (!reader.good())
            return FVM_INVALID_ARGUMENT;
        long signed_value = raw & FVM_SWF_FOCAL_SIGN
                                ? -(long)(FVM_SWF_FOCAL_MODULUS - raw) : (long)raw;
        paint.gradient_focal = signed_value / FVM_SWF_FOCAL_SCALE;
        /* The current ray sampler intentionally excludes degenerate circle-boundary origins. */
        if (paint.gradient_focal <= -FVM_GRADIENT_FOCAL_LIMIT ||
            paint.gradient_focal >= FVM_GRADIENT_FOCAL_LIMIT)
            return FVM_UNSUPPORTED;
    }
    return FVM_OK;
}

/* Decode one owned fill for a fill bank or the FillType of a LINESTYLE2 stroke. */
FVM_Result fill(FVM_SWFReader &reader, unsigned int tag, FVM_SWFShapePaint &paint)
{
    unsigned long type = reader.integer(1);
    if (!reader.good())
        return FVM_INVALID_ARGUMENT;
    if (type == FVM_SWF_FILL_SOLID)
    {
        if (!color(reader, (tag == FVM_SWF_TAG_DEFINE_SHAPE3 || tag == FVM_SWF_TAG_DEFINE_SHAPE4), paint))
            return FVM_INVALID_ARGUMENT;
    }
    else if (type == FVM_SWF_FILL_LINEAR_GRADIENT || type == FVM_SWF_FILL_RADIAL_GRADIENT ||
             (tag == FVM_SWF_TAG_DEFINE_SHAPE4 && type == FVM_SWF_FILL_FOCAL_GRADIENT))
    {
        paint.gradient_type = (unsigned int)type;
        FVM_Result r = gradient(reader, tag, paint);
        if (r != FVM_OK)
            return r;
    }
    else if (type >= FVM_SWF_FILL_BITMAP_REPEAT && type <= FVM_SWF_FILL_BITMAP_CLIPPED_NEAREST)
    {
        paint.bitmap_type = (unsigned int)type;
        paint.bitmap_id = (unsigned int)reader.integer(2);
        if (!paint.bitmap_id || !fvm_swf_read_matrix(reader, paint.bitmap_matrix))
            return FVM_INVALID_ARGUMENT;
        /* Bitmap scales map pixel coordinates into SWF twips; translation is already in pixels.
         */
        paint.bitmap_matrix.xx /= 20;
        paint.bitmap_matrix.yx /= 20;
        paint.bitmap_matrix.xy /= 20;
        paint.bitmap_matrix.yy /= 20;
    }
    else
        return FVM_UNSUPPORTED;
    return FVM_OK;
}

/* Decode a style bank, appending it while retaining previously referenced global styles. */
FVM_Result styles(FVM_SWFReader &reader, unsigned int tag,
                  fvm_detail::vector<FVM_SWFShapePaint> &fills,
                  fvm_detail::vector<FVM_SWFShapePaint> &lines, unsigned long &fill_count,
                  unsigned long &line_count)
{
    fill_count = reader.integer(1);
    if (fill_count == 255 && tag != FVM_SWF_TAG_DEFINE_SHAPE)
        fill_count = reader.integer(2);
    if (!reader.good())
        return FVM_INVALID_ARGUMENT;
    if (fill_count > 4096 - fills.size())
        return FVM_OUT_OF_MEMORY;
    for (unsigned long i = 0; i < fill_count; ++i)
    {
        FVM_SWFShapePaint paint;
        FVM_Result r = fill(reader, tag, paint);
        if (r != FVM_OK)
            return r;
        fills.push_back(paint);
    }
    line_count = reader.integer(1);
    if (line_count == 255 && tag != FVM_SWF_TAG_DEFINE_SHAPE)
        line_count = reader.integer(2);
    if (!reader.good())
        return FVM_INVALID_ARGUMENT;
    if (line_count > 4096 - lines.size())
        return FVM_OUT_OF_MEMORY;
    for (unsigned long i = 0; i < line_count; ++i)
    {
        FVM_SWFShapePaint paint;
        paint.stroke = true;
        paint.width = reader.integer(2) / 20.0;
        if (tag == FVM_SWF_TAG_DEFINE_SHAPE4)
        {
            /* Decode both flag bytes in wire bit order, independently of host endianness. */
            unsigned long start_cap = reader.bits(FVM_SWF_LINESTYLE_KIND_BITS);
            unsigned long join = reader.bits(FVM_SWF_LINESTYLE_KIND_BITS);
            bool has_fill = reader.bits(1) != 0;
            bool no_h_scale = reader.bits(1) != 0;
            bool no_v_scale = reader.bits(1) != 0;
            bool pixel_hinting = reader.bits(1) != 0;
            unsigned long reserved = reader.bits(FVM_SWF_LINESTYLE_RESERVED_BITS);
            paint.no_close = reader.bits(1) != 0;
            unsigned long end_cap = reader.bits(FVM_SWF_LINESTYLE_KIND_BITS);
            reader.align();
            if (!reader.good() || reserved || start_cap == FVM_SWF_LINESTYLE_RESERVED_KIND ||
                join == FVM_SWF_LINESTYLE_RESERVED_KIND || end_cap == FVM_SWF_LINESTYLE_RESERVED_KIND)
                return FVM_INVALID_ARGUMENT;
            /* Shared caps and round/bevel joins map directly to the selected backend. */
            if (start_cap != end_cap || join > FVM_SWF_LINESTYLE_BEVEL ||
                no_h_scale || no_v_scale || pixel_hinting)
                return FVM_UNSUPPORTED;
            paint.stroke_cap = (unsigned)start_cap;
            paint.stroke_join = (unsigned)join;
            if (has_fill)
            {
                FVM_Result r = fill(reader, tag, paint);
                if (r != FVM_OK)
                    return r;
            }
            else if (!color(reader, true, paint))
                return FVM_INVALID_ARGUMENT;
        }
        else if (!color(reader, tag == FVM_SWF_TAG_DEFINE_SHAPE3, paint))
            return FVM_INVALID_ARGUMENT;
        lines.push_back(paint);
    }
    return FVM_OK;
}

/* Read a one-based style index; zero deselects painting instead of addressing a bank. */
bool index(FVM_SWFReader &reader, unsigned int bits, unsigned long count, unsigned long offset,
           unsigned long &out)
{
    unsigned long value = reader.bits(bits);
    if (!reader.good() || value > count)
        return false;
    out = value ? offset + value : 0;
    return true;
}

/* Reverse the orientation of an edge on the opposite side of a fill boundary. */
Edge reversed(const Edge &edge)
{
    Edge out = edge;
    out.x0 = edge.x1;
    out.y0 = edge.y1;
    out.x1 = edge.x0;
    out.y1 = edge.y0;
    return out;
}

/* Build a collision-free text key for an exact twip endpoint using portable long formatting. */
fvm_detail::string point_key(long x, long y)
{
    char buffer[64];
    std::sprintf(buffer, "%ld,%ld", x, y);
    return fvm_detail::string(buffer);
}

/* Append one zero-initialized path command, converting only its used points to pixels. */
void command(FVM_SWFShapePaint &paint, FVM_PathVerb verb, long x0, long y0, long x1 = 0,
             long y1 = 0)
{
    FVM_PathCommand value;
    std::memset(&value, 0, sizeof(value));
    value.verb = verb;
    value.x[0] = x0 / 20.0;
    value.y[0] = y0 / 20.0;
    value.x[1] = x1 / 20.0;
    value.y[1] = y1 / 20.0;
    paint.commands.push_back(value);
}

/* Assemble exact connected contours in linear expected time, preserving holes and shared edges. */
void contours(const Edges &edges, FVM_SWFShapePaint &paint)
{
    fvm_detail::map<fvm_detail::string, unsigned long> heads;
    fvm_detail::vector<unsigned long> next(edges.size());
    fvm_detail::vector<unsigned char> used(edges.size());
    for (size_t i = edges.size(); i; --i)
    {
        fvm_detail::string key = point_key(edges[i - 1].x0, edges[i - 1].y0);
        next[i - 1] = heads[key];
        heads[key] = (unsigned long)i;
    }
    for (size_t start = 0; start < edges.size(); ++start)
    {
        if (used[start])
            continue;
        command(paint, FVM_PATH_MOVE, edges[start].x0, edges[start].y0);
        unsigned long current = (unsigned long)start + 1;
        long end_x = edges[start].x0, end_y = edges[start].y0;
        while (current)
        {
            unsigned long i = current - 1;
            const Edge &edge = edges[i];
            used[i] = 1;
            if (edge.curve)
                command(paint, FVM_PATH_QUADRATIC, edge.cx, edge.cy, edge.x1, edge.y1);
            else
                command(paint, FVM_PATH_LINE, edge.x1, edge.y1);
            end_x = edge.x1;
            end_y = edge.y1;
            if (end_x == edges[start].x0 && end_y == edges[start].y0)
                break;
            fvm_detail::string key = point_key(end_x, end_y);
            current = heads[key];
            while (current && used[current - 1])
                current = next[current - 1];
            heads[key] = current;
        }
        if (!paint.stroke || (end_x == edges[start].x0 && end_y == edges[start].y0))
            command(paint, FVM_PATH_CLOSE, 0, 0);
    }
}

/* Preserve original stroke order and explicit path breaks, including coincident endpoints. */
void stroke_contours(const Edges &edges, FVM_SWFShapePaint &paint)
{
    bool open = false;
    long x = 0, y = 0, start_x = 0, start_y = 0;
    for (size_t i = 0; i < edges.size(); ++i)
    {
        const Edge &edge = edges[i];
        if (!open || edge.begin || edge.x0 != x || edge.y0 != y)
        {
            command(paint, FVM_PATH_MOVE, edge.x0, edge.y0);
            start_x = edge.x0;
            start_y = edge.y0;
            open = true;
        }
        if (edge.curve)
            command(paint, FVM_PATH_QUADRATIC, edge.cx, edge.cy, edge.x1, edge.y1);
        else
            command(paint, FVM_PATH_LINE, edge.x1, edge.y1);
        x = edge.x1;
        y = edge.y1;
        if (!paint.no_close && x == start_x && y == start_y)
        {
            command(paint, FVM_PATH_CLOSE, 0, 0);
            open = false;
        }
    }
}

/* Validate a complete record before transferring any decoded state to the caller. */
FVM_Result decode(const unsigned char *bytes, size_t size, unsigned int tag, FVM_SWFShape &out)
{
    FVM_SWFReader reader(bytes, size);
    out.id = (unsigned int)reader.integer(2);
    unsigned int bits = (unsigned int)reader.bits(5);
    long xmin = reader.signed_bits(bits), xmax = reader.signed_bits(bits);
    long ymin = reader.signed_bits(bits), ymax = reader.signed_bits(bits);
    reader.align();
    if (!reader.good() || !out.id || xmin > xmax || ymin > ymax || !coordinate(xmin) ||
        !coordinate(xmax) || !coordinate(ymin) || !coordinate(ymax))
        return FVM_INVALID_ARGUMENT;
    out.bounds.x_min = xmin / 20.0;
    out.bounds.x_max = xmax / 20.0;
    out.bounds.y_min = ymin / 20.0;
    out.bounds.y_max = ymax / 20.0;
    if (tag == FVM_SWF_TAG_DEFINE_SHAPE4)
    {
        /* Validate the separate edge rectangle even though fill-only layout uses ShapeBounds. */
        unsigned int edge_bits = (unsigned int)reader.bits(5);
        long ex0 = reader.signed_bits(edge_bits), ex1 = reader.signed_bits(edge_bits);
        long ey0 = reader.signed_bits(edge_bits), ey1 = reader.signed_bits(edge_bits);
        reader.align();
        unsigned long flags = reader.integer(1);
        if (!reader.good() || ex0 > ex1 || ey0 > ey1 || !coordinate(ex0) || !coordinate(ex1) ||
            !coordinate(ey0) || !coordinate(ey1) || (flags & FVM_SWF_SHAPE4_RESERVED_MASK))
            return FVM_INVALID_ARGUMENT;
    }
    fvm_detail::vector<FVM_SWFShapePaint> fills, lines;
    unsigned long fill_count = 0, line_count = 0, fill_offset = 0, line_offset = 0;
    FVM_Result r = styles(reader, tag, fills, lines, fill_count, line_count);
    if (r != FVM_OK)
        return r;
    unsigned int fill_bits = (unsigned int)reader.bits(4), line_bits = (unsigned int)reader.bits(4);
    unsigned long fill0 = 0, fill1 = 0, line = 0, edge_count = 0, record_count = 0;
    long x = 0, y = 0;
    bool stroke_break = true;
    Groups filled, stroked;
    for (;;)
    {
        if (++record_count > 131072)
            return FVM_OUT_OF_MEMORY;
        bool edge_record = reader.bits(1) != 0;
        if (!reader.good())
            return FVM_INVALID_ARGUMENT;
        if (!edge_record)
        {
            unsigned long flags = reader.bits(5);
            if (!reader.good())
                return FVM_INVALID_ARGUMENT;
            if (!flags)
                break;
            if (flags & 9)
                stroke_break = true;
            if (flags & 1)
            {
                unsigned int count = (unsigned int)reader.bits(5);
                x = reader.signed_bits(count);
                y = reader.signed_bits(count);
                if (!coordinate(x) || !coordinate(y))
                    return FVM_INVALID_ARGUMENT;
            }
            if ((flags & 2) && !index(reader, fill_bits, fill_count, fill_offset, fill0))
                return FVM_INVALID_ARGUMENT;
            if ((flags & 4) && !index(reader, fill_bits, fill_count, fill_offset, fill1))
                return FVM_INVALID_ARGUMENT;
            if ((flags & 8) && !index(reader, line_bits, line_count, line_offset, line))
                return FVM_INVALID_ARGUMENT;
            if (flags & 16)
            {
                if (tag == FVM_SWF_TAG_DEFINE_SHAPE)
                    return FVM_INVALID_ARGUMENT;
                reader.align();
                fill_offset = (unsigned long)fills.size();
                line_offset = (unsigned long)lines.size();
                r = styles(reader, tag, fills, lines, fill_count, line_count);
                if (r != FVM_OK)
                    return r;
                fill_bits = (unsigned int)reader.bits(4);
                line_bits = (unsigned int)reader.bits(4);
            }
            if (!reader.good())
                return FVM_INVALID_ARGUMENT;
            continue;
        }
        if (++edge_count > 65536)
            return FVM_OUT_OF_MEMORY;
        Edge edge;
        edge.begin = stroke_break;
        stroke_break = false;
        edge.x0 = x;
        edge.y0 = y;
        bool straight = reader.bits(1) != 0;
        unsigned int count = (unsigned int)reader.bits(4) + 2;
        if (straight)
        {
            if (reader.bits(1))
            {
                x += reader.signed_bits(count);
                y += reader.signed_bits(count);
            }
            else if (reader.bits(1))
                y += reader.signed_bits(count);
            else
                x += reader.signed_bits(count);
        }
        else
        {
            edge.curve = true;
            edge.cx = x + reader.signed_bits(count);
            edge.cy = y + reader.signed_bits(count);
            x = edge.cx + reader.signed_bits(count);
            y = edge.cy + reader.signed_bits(count);
            if (!coordinate(edge.cx) || !coordinate(edge.cy))
                return FVM_INVALID_ARGUMENT;
        }
        if (!reader.good() || !coordinate(x) || !coordinate(y))
            return FVM_INVALID_ARGUMENT;
        edge.x1 = x;
        edge.y1 = y;
        if (fill0 && fill0 != fill1)
            filled[fill0].push_back(reversed(edge));
        if (fill1 && fill0 != fill1)
            filled[fill1].push_back(edge);
        if (line)
            stroked[line].push_back(edge);
    }
    if (!reader.finished())
        return FVM_INVALID_ARGUMENT;
    for (unsigned long i = 1; i <= fills.size(); ++i)
    {
        Groups::iterator group = filled.find(i);
        if (group == filled.end())
            continue;
        FVM_SWFShapePaint paint = fills[i - 1];
        contours(group->second, paint);
        out.paints.push_back(paint);
    }
    for (unsigned long i = 1; i <= lines.size(); ++i)
    {
        Groups::iterator group = stroked.find(i);
        if (group == stroked.end())
            continue;
        FVM_SWFShapePaint paint = lines[i - 1];
        stroke_contours(group->second, paint);
        out.paints.push_back(paint);
    }
    return FVM_OK;
}
} // namespace

/* Publish a decoded solid shape only after complete validation; allocation failures leave it
 * intact. */
FVM_Result fvm_swf_read_shape(const unsigned char *bytes, size_t size, unsigned int tag,
                              FVM_SWFShape &out)
{
    if (!bytes || (tag != FVM_SWF_TAG_DEFINE_SHAPE && tag != FVM_SWF_TAG_DEFINE_SHAPE2 &&
                   tag != FVM_SWF_TAG_DEFINE_SHAPE3 && tag != FVM_SWF_TAG_DEFINE_SHAPE4))
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_SWFShape value;
        FVM_Result r = decode(bytes, size, tag, value);
        if (r != FVM_OK)
            return r;
        out.id = value.id;
        out.bounds = value.bounds;
        out.paints.swap(value.paints);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
