/* FlashVM: C embedding API: text layout.
 * File: src/capi/text_layout_c.cpp.
 */

#include "flashvm/text_layout.h"
#include "font_internal.hpp"
#include "text_layout_constants.h"
#include "../details/vector.hpp"
#include <cstring>
#include <new>

/* Define FVM_TextLayout struct and its representation. */
struct FVM_TextLayout
{
    fvm_detail::vector<FVM_TextLayoutLine> lines;
    fvm_detail::vector<FVM_TextLayoutGlyph> glyphs;
    fvm_detail::vector<FVM_Path *> outlines;
    FVM_TextLayoutMetrics metrics;
    FVM_Path *path;
    int underline_winding; /* Immutable aggregate font-outline direction after construction. */
    bool underline_conflict; /* Reject bars when nondegenerate glyph directions disagree. */
    /* Initialize Text Layout and its owned state. */
    FVM_TextLayout() : path(0), underline_winding(FVM_TEXT_LAYOUT_WINDING_NONE), underline_conflict(false)
    {
        std::memset(&metrics, 0, sizeof(metrics));
    }
    /* Release the resources owned by this FVM_TextLayout instance. */
    ~FVM_TextLayout()
    {
        for (size_t i = 0; i < outlines.size(); ++i)
            fvm_path_destroy(outlines[i]);
        fvm_path_destroy(path);
    }
};
/* Small ownership guards keep partial layout failures and allocation exceptions
 * from leaking a glyph, shaping buffer or unfinished result. */
/* Define PathOwner struct and its representation. */
struct PathOwner
{
    FVM_Path *value;
    /* Initialize Path Owner and its owned state. */
    PathOwner() : value(0)
    {
    }
    /* Release the resources owned by this PathOwner instance. */
    ~PathOwner()
    {
        fvm_path_destroy(value);
    }

  private:
    /* Initialize Path Owner and its owned state. */
    PathOwner(const PathOwner &);
    PathOwner &operator=(const PathOwner &);
};

/* Copy path. */
static FVM_Result copy_path(const FVM_Path *source, const FVM_Affine *matrix, FVM_Path **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    *out = 0;
    PathOwner copy;
    FVM_Result result = fvm_path_create(&copy.value);
    if (result == FVM_OK)
        result = fvm_path_append(copy.value, source, matrix);
    if (result == FVM_OK)
    {
        *out = copy.value;
        copy.value = 0;
    }
    return result;
}

/* Destroy text layout. */
extern "C" void fvm_text_layout_destroy(FVM_TextLayout *layout)
{
    delete layout;
}

/* Compute and return text layout metrics. */
extern "C" FVM_Result fvm_text_layout_metrics(const FVM_TextLayout *layout,
                                              FVM_TextLayoutMetrics *out)
{
    if (!layout || !out)
        return FVM_INVALID_ARGUMENT;
    *out = layout->metrics;
    return FVM_OK;
}

/* Compute and return text layout line. */
extern "C" FVM_Result fvm_text_layout_line(const FVM_TextLayout *layout, size_t index,
                                           FVM_TextLayoutLine *out)
{
    if (!layout || !out || index >= layout->lines.size())
        return FVM_INVALID_ARGUMENT;
    *out = layout->lines[index];
    return FVM_OK;
}

/* Compute and return text layout glyph. */
extern "C" FVM_Result fvm_text_layout_glyph(const FVM_TextLayout *layout, size_t index,
                                            FVM_TextLayoutGlyph *out)
{
    if (!layout || !out || index >= layout->glyphs.size())
        return FVM_INVALID_ARGUMENT;
    *out = layout->glyphs[index];
    return FVM_OK;
}

/* Compute and return text layout path. */
extern "C" FVM_Result fvm_text_layout_path(const FVM_TextLayout *layout, FVM_Path **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    return layout ? copy_path(layout->path, 0, out) : FVM_INVALID_ARGUMENT;
}

/* Compute and return text layout glyph path. */
extern "C" FVM_Result fvm_text_layout_glyph_path(const FVM_TextLayout *layout, size_t index,
                                                 FVM_Path **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!layout || index >= layout->glyphs.size())
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    const FVM_TextLayoutGlyph &g = layout->glyphs[index];
    FVM_Affine matrix = {1, 0, 0, 1, g.x, g.y};
    return copy_path(layout->outlines[index], &matrix, out);
}
#ifdef FVM_HAS_FONTS
/* Capture control-polygon winding once while building owned glyph outlines.
 * The immutable layout then shares one direction across ink and whitespace lines,
 * avoiding repeated full-glyph scans for every exported underline bar. */
static FVM_Result outline_underline_direction(const FVM_Path *outline, int &direction)
{
    direction = FVM_TEXT_LAYOUT_WINDING_NONE;
    double area = 0, first_x = 0, first_y = 0, previous_x = 0, previous_y = 0;
    bool open = false;
    for (size_t i = 0; i < fvm_path_count(outline); ++i)
    {
        FVM_PathCommand command;
        FVM_Result result = fvm_path_get(outline, i, &command);
        if (result != FVM_OK)
            return result;
        if (command.verb == FVM_PATH_MOVE)
        {
            if (open)
                area += previous_x * first_y - first_x * previous_y;
            first_x = previous_x = command.x[0];
            first_y = previous_y = command.y[0];
            open = true;
        }
        else if (command.verb == FVM_PATH_CLOSE)
        {
            if (open)
                area += previous_x * first_y - first_x * previous_y;
            open = false;
        }
        else
        {
            unsigned points = command.verb == FVM_PATH_CUBIC ? FVM_TEXT_LAYOUT_CUBIC_POINTS :
                              command.verb == FVM_PATH_QUADRATIC ? FVM_TEXT_LAYOUT_QUADRATIC_POINTS :
                              FVM_TEXT_LAYOUT_LINE_POINTS;
            for (unsigned point = 0; point < points; ++point)
            {
                area += previous_x * command.y[point] - command.x[point] * previous_y;
                previous_x = command.x[point];
                previous_y = command.y[point];
            }
        }
    }
    if (open)
        area += previous_x * first_y - first_x * previous_y;
    if (area != 0)
        direction = area < 0 ? FVM_TEXT_LAYOUT_WINDING_NEGATIVE : FVM_TEXT_LAYOUT_WINDING_POSITIVE;
    return FVM_OK;
}
#endif

/* Copy a full logical-line underline into an owned path. The caller supplies
 * font-derived metrics so the immutable layout never retains a mutable font. */
extern "C" FVM_Result fvm_text_layout_line_underline_path(const FVM_TextLayout *layout, size_t index,
                                            double center, double thickness, FVM_Path **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!layout || index >= layout->lines.size() ||
        !(center >= -FVM_TEXT_LAYOUT_MAX_EXTENT && center <= FVM_TEXT_LAYOUT_MAX_EXTENT) ||
        !(thickness > 0 && thickness <= FVM_TEXT_LAYOUT_MAX_EXTENT))
        return FVM_INVALID_ARGUMENT;
    const FVM_TextLayoutLine &line = layout->lines[index];
    PathOwner result;
    FVM_Result status = fvm_path_create(&result.value);
    if (status != FVM_OK)
        return status;
    if (line.advance > 0)
    {
        double left = line.x, right = line.x + line.advance;
        double top = line.baseline + center - thickness / 2;
        double bottom = top + thickness;
        if (!(left >= -FVM_TEXT_LAYOUT_MAX_EXTENT && right <= FVM_TEXT_LAYOUT_MAX_EXTENT &&
              top >= -FVM_TEXT_LAYOUT_MAX_EXTENT && bottom <= FVM_TEXT_LAYOUT_MAX_EXTENT))
            return FVM_INVALID_ARGUMENT;
        if (layout->underline_conflict)
            return FVM_UNSUPPORTED;
        bool reverse = layout->underline_winding == FVM_TEXT_LAYOUT_WINDING_NEGATIVE;
        FVM_PathCommand commands[FVM_TEXT_LAYOUT_RECT_COMMANDS];
        std::memset(commands, 0, sizeof(commands));
        commands[0].verb = FVM_PATH_MOVE; commands[0].x[0] = left; commands[0].y[0] = top;
        commands[1].verb = FVM_PATH_LINE; commands[1].x[0] = reverse ? left : right; commands[1].y[0] = reverse ? bottom : top;
        commands[2].verb = FVM_PATH_LINE; commands[2].x[0] = right; commands[2].y[0] = bottom;
        commands[3].verb = FVM_PATH_LINE; commands[3].x[0] = reverse ? right : left; commands[3].y[0] = reverse ? top : bottom;
        commands[4].verb = FVM_PATH_CLOSE;
        for (size_t i = 0; i < FVM_TEXT_LAYOUT_RECT_COMMANDS; ++i)
        {
            status = fvm_path_add(result.value, commands + i);
            if (status != FVM_OK)
                return status;
        }
    }
    *out = result.value;
    result.value = 0;
    return FVM_OK;
}

#ifdef FVM_HAS_FONTS
#include "linebreak.h"
#include "graphemebreak.h"
#include "fribidi.h"

/* Define RunOwner struct and its representation. */
struct RunOwner
{
    FVM_TextRun *value;
    /* Run Owner. */
    RunOwner() : value(0)
    {
    }
    /* Release the resources owned by this RunOwner instance. */
    ~RunOwner()
    {
        fvm_text_run_destroy(value);
    }

  private:
    /* Run Owner. */
    RunOwner(const RunOwner &);
    RunOwner &operator=(const RunOwner &);
};

/* Define LayoutOwner struct and its representation. */
struct LayoutOwner
{
    FVM_TextLayout *value;
    /* Initialize Layout Owner and its owned state. */
    LayoutOwner() : value(0)
    {
    }
    /* Release the resources owned by this LayoutOwner instance. */
    ~LayoutOwner()
    {
        delete value;
    }

  private:
    /* Initialize Layout Owner and its owned state. */
    LayoutOwner(const LayoutOwner &);
    LayoutOwner &operator=(const LayoutOwner &);
};

/* Define Paragraph struct and its representation. */
struct Paragraph
{
    fvm_detail::vector<FriBidiChar> chars;
    fvm_detail::vector<FriBidiCharType> types;
    fvm_detail::vector<FriBidiLevel> levels;
    fvm_detail::vector<hb_script_t> scripts;
    fvm_detail::vector<size_t> bytes;
    FriBidiParType base;
};

/* Compute and return separator. */
static bool separator(unsigned int c)
{
    return c == 10 || c == 13 || c == 11 || c == 12 || c == 0x85 || c == 0x2028 || c == 0x2029;
}
/* Strict decoder: rejects overlong sequences, surrogates and truncated scalars.
 * Byte boundaries are retained, never inferred from shaped glyph counts. */
/* Decode the current value. */
static FVM_Result decode(const char *text, size_t size, fvm_detail::vector<FriBidiChar> &chars,
                         fvm_detail::vector<size_t> &bytes)
{
    if ((!text && size) || size > 16384)
        return FVM_INVALID_ARGUMENT;
    const unsigned char *input = (const unsigned char *)text;
    for (size_t i = 0; i < size;)
    {
        bytes.push_back(i);
        unsigned int c = input[i++], following = 0, minimum = 0;
        if (c >= 0xc2 && c <= 0xdf)
        {
            c &= 31;
            following = 1;
            minimum = 0x80;
        }
        else if (c >= 0xe0 && c <= 0xef)
        {
            c &= 15;
            following = 2;
            minimum = 0x800;
        }
        else if (c >= 0xf0 && c <= 0xf4)
        {
            c &= 7;
            following = 3;
            minimum = 0x10000;
        }
        else if (c >= 0x80)
            return FVM_INVALID_ARGUMENT;
        if (following > size - i)
            return FVM_INVALID_ARGUMENT;
        for (unsigned int n = 0; n < following; ++n)
        {
            unsigned int b = input[i++];
            if ((b & 0xc0) != 0x80)
                return FVM_INVALID_ARGUMENT;
            c = (c << 6) | (b & 63);
        }
        if (c < minimum || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
            return FVM_INVALID_ARGUMENT;
        if (c == 9 || c == 0xad)
            return FVM_UNSUPPORTED;
        if (chars.size() == 4096)
            return FVM_UNSUPPORTED;
        chars.push_back(c);
    }
    bytes.push_back(size);
    return FVM_OK;
}

/* Compute and return weak script. */
static bool weak_script(hb_script_t script)
{
    return script == HB_SCRIPT_COMMON || script == HB_SCRIPT_INHERITED ||
           script == HB_SCRIPT_UNKNOWN;
}

/* Initialize paragraph. */
static FVM_Result paragraph_init(Paragraph &p, const fvm_detail::vector<FriBidiChar> &chars,
                                 const fvm_detail::vector<size_t> &bytes, size_t start, size_t end,
                                 FVM_TextDirection direction)
{
    size_t count = end - start;

    /* Computation / Вычисление и обновление состояния. */
    p.chars.resize(count);
    p.types.resize(count);
    p.levels.resize(count);
    p.scripts.resize(count);
    p.bytes.resize(count + 1);

    /* Initialization / Инициализация локального состояния. */
    hb_unicode_funcs_t *unicode = hb_unicode_funcs_get_default();
    hb_script_t previous = HB_SCRIPT_COMMON;
    for (size_t i = 0; i < count; ++i)
    {
        p.chars[i] = chars[start + i];
        p.bytes[i] = bytes[start + i];
        hb_script_t script = hb_unicode_script(unicode, p.chars[i]);
        if (weak_script(script))
            script = previous;
        else
            previous = script;
        p.scripts[i] = script;
    }
    /* Leading neutrals inherit the following script. Interior neutrals keep the
     * preceding one. Bidi levels still split numbers and opposite-direction runs. */
    hb_script_t next = HB_SCRIPT_COMMON;
    for (size_t i = count; i > 0; --i)
    {
        if (weak_script(p.scripts[i - 1]))
            p.scripts[i - 1] = next;
        else
            next = p.scripts[i - 1];
    }

    /* Computation / Вычисление и обновление состояния. */
    p.bytes[count] = bytes[end];
    p.base = direction == FVM_TEXT_AUTO  ? FRIBIDI_PAR_ON
             : direction == FVM_TEXT_RTL ? FRIBIDI_PAR_RTL
                                         : FRIBIDI_PAR_LTR;
    if (!count)
    {
        if (direction == FVM_TEXT_AUTO)
            p.base = FRIBIDI_PAR_LTR;
        return FVM_OK;
    }
    fvm_detail::vector<FriBidiBracketType> brackets(count);

    /* Computation / Вычисление и обновление состояния. */
    fribidi_get_bidi_types(p.chars.begin(), (FriBidiStrIndex)count, p.types.begin());
    fribidi_get_bracket_types(p.chars.begin(), (FriBidiStrIndex)count, p.types.begin(),
                              brackets.begin());
    if (!fribidi_get_par_embedding_levels_ex(p.types.begin(), brackets.begin(),
                                             (FriBidiStrIndex)count, &p.base, p.levels.begin()))
        return FVM_OUT_OF_MEMORY;
    return FVM_OK;
}

/* Define Segment struct and its representation. */
struct Segment
{
    size_t start, end, rank;
    FriBidiLevel level;
};
/* Reorder whole shaped segments rather than codepoints or individual glyphs:
 * combining marks and ligature clusters keep HarfBuzz's within-run ordering. */
/* Compute and return shape line. */
static FVM_Result shape_line(FVM_Font *font, const char *text, const char *language,
                             const Paragraph &p, size_t start, size_t end, unsigned int flags, double spacing, size_t &budget,
                             fvm_detail::vector<FVM_TextLayoutGlyph> &glyphs, double &advance)
{

    /* Computation / Вычисление и обновление состояния. */
    glyphs.clear();
    advance = 0;
    if (start == end)
        return FVM_OK;
    if (end - start > budget)
        return FVM_UNSUPPORTED;
    budget -= end - start;

    /* Initialization / Инициализация локального состояния. */
    fvm_detail::vector<FriBidiLevel> levels = p.levels;
    fvm_detail::vector<FriBidiStrIndex> map(p.chars.size());
    for (size_t i = 0; i < map.size(); ++i)
        map[i] = (FriBidiStrIndex)i;
    if (!fribidi_reorder_line(0, p.types.begin(), (FriBidiStrIndex)(end - start),
                              (FriBidiStrIndex)start, p.base, levels.begin(), 0, map.begin()))
        return FVM_OUT_OF_MEMORY;
    fvm_detail::vector<size_t> rank(p.chars.size());
    for (size_t i = start; i < end; ++i)
        rank[(size_t)map[i]] = i;
    fvm_detail::vector<Segment> segments;
    for (size_t i = start; i < end;)
    {
        Segment s;
        s.start = i;
        s.level = levels[i];
        s.rank = rank[i];
        for (++i; i < end && levels[i] == s.level && p.scripts[i] == p.scripts[s.start]; ++i)
            if (rank[i] < s.rank)
                s.rank = rank[i];
        s.end = i;
        segments.push_back(s);
    }
    /* Insertion sort avoids STL and preserves deterministic visual ordering. */
    for (size_t i = 1; i < segments.size(); ++i)
    {
        Segment current = segments[i];
        size_t j = i;
        while (j && segments[j - 1].rank > current.rank)
        {
            segments[j] = segments[j - 1];
            --j;
        }
        segments[j] = current;
    }
    size_t first_byte = p.bytes[start], line_size = p.bytes[end] - first_byte;
    for (size_t n = 0; n < segments.size(); ++n)
    {
        const Segment &s = segments[n];
        RunOwner run;
        FVM_Result result = fvm_font_shape_segment_configured(
            font, text + first_byte, line_size, p.bytes[s.start] - first_byte,
            p.bytes[s.end] - p.bytes[s.start], (s.level & 1) ? FVM_TEXT_RTL : FVM_TEXT_LTR,
            p.scripts[s.start], language, flags, &run.value);
        if (result != FVM_OK)
            return result;
        for (size_t i = 0; i < fvm_text_run_count(run.value); ++i)
        {
            FVM_TextLayoutGlyph g;
            std::memset(&g, 0, sizeof(g));
            result = fvm_text_run_get(run.value, i, &g.shaped);
            if (result != FVM_OK)
                return result;
            g.shaped.cluster += (unsigned int)first_byte;
            g.x = advance + g.shaped.x_offset;
            g.y = g.shaped.y_offset;
            g.bidi_level = (unsigned char)s.level;
            if (!(g.shaped.x_advance >= 0 && g.shaped.x_advance <= 1000000) ||
                g.shaped.y_advance != 0)
                return FVM_UNSUPPORTED;
            advance += g.shaped.x_advance;
            if (!(advance <= 1000000) || glyphs.size() == 65536)
                return FVM_UNSUPPORTED;
            glyphs.push_back(g);
        }
    }
    if (spacing != 0)
    {
        /* Until complex-cluster tracking has a reference oracle, accept exactly
         * one glyph per scalar with unique source clusters. Never split a ligature
         * or silently add spacing inside a combining sequence. */
        if (glyphs.size() != end - start)
            return FVM_UNSUPPORTED;
        fvm_detail::vector<unsigned char> seen(end - start);
        std::memset(seen.begin(), 0, seen.size());
        for (size_t i = 0; i < glyphs.size(); ++i)
        {
            FVM_TextLayoutGlyph &g = glyphs[i];
            size_t low = start, high = end;
            while (low < high)
            {
                size_t middle = low + (high - low) / 2;
                if (p.bytes[middle] < g.shaped.cluster)
                    low = middle + 1;
                else
                    high = middle;
            }
            if (low == end || p.bytes[low] != g.shaped.cluster || seen[low - start] ||
                !(g.shaped.x_advance + spacing >= 0))
                return FVM_UNSUPPORTED;
            seen[low - start] = 1;
            g.x += i * spacing;
            g.shaped.x_advance += spacing;
        }
        advance += glyphs.size() * spacing;
        if (!(advance >= 0 && advance <= FVM_TEXT_LAYOUT_MAX_EXTENT))
            return FVM_UNSUPPORTED;
    }
    return FVM_OK;
}

/* Compute and return allowed. */
static bool allowed(const fvm_detail::vector<char> &breaks, const Paragraph &p, size_t end)
{
    return end == p.chars.size() || breaks[p.bytes[end] - 1] == LINEBREAK_ALLOWBREAK ||
           breaks[p.bytes[end] - 1] == LINEBREAK_MUSTBREAK;
}

/* Compute and return choose line. */
static FVM_Result choose_line(FVM_Font *font, const char *text, const char *language,
                              const Paragraph &p, size_t start, double width, unsigned int flags, double spacing,
                              const fvm_detail::vector<char> &line,
                              const fvm_detail::vector<char> &grapheme, size_t &budget, size_t &end)
{
    end = p.chars.size();
    if (!width || start == end)
        return FVM_OK;

    /* Initialization / Инициализация локального состояния. */
    size_t last = start, first_overflow = end;
    fvm_detail::vector<FVM_TextLayoutGlyph> trial;
    for (size_t candidate = start + 1; candidate <= p.chars.size(); ++candidate)
    {
        if (!allowed(line, p, candidate))
            continue;
        double advance;
        FVM_Result result =
            shape_line(font, text, language, p, start, candidate, flags, spacing, budget, trial, advance);
        if (result != FVM_OK)
            return result;
        if (advance > width)
        {
            first_overflow = candidate;
            break;
        }
        last = candidate;
    }
    if (last != start)
    {
        end = last;
        return FVM_OK;
    }
    /* An oversized word falls back to grapheme boundaries; never split a base
     * from its combining mark. A single oversized cluster still makes progress. */
    last = start;
    for (size_t candidate = start + 1; candidate <= first_overflow; ++candidate)
    {
        if (candidate != p.chars.size() && grapheme[p.bytes[candidate] - 1] != GRAPHEMEBREAK_BREAK)
            continue;
        double advance;
        FVM_Result result =
            shape_line(font, text, language, p, start, candidate, flags, spacing, budget, trial, advance);
        if (result != FVM_OK)
            return result;
        if (advance > width)
        {
            end = last == start ? candidate : last;
            return FVM_OK;
        }
        last = candidate;
    }
    end = last == start ? first_overflow : last;
    return FVM_OK;
}

/* Append line. */
static FVM_Result append_line(FVM_TextLayout &layout, FVM_Font *font, const char *text,
                              const FVM_TextLayoutOptions &options, const FVM_FontMetrics &metrics,
                              const Paragraph &p, size_t start, size_t end, size_t next,
                              unsigned int flags, double spacing, size_t &budget)
{

    /* Initialization / Инициализация локального состояния. */
    fvm_detail::vector<FVM_TextLayoutGlyph> glyphs;
    double advance;
    FVM_Result result =
        shape_line(font, text, options.language, p, start, end, flags, spacing, budget, glyphs, advance);
    if (result != FVM_OK)
        return result;
    FVM_TextLayoutLine line;

    /* Computation / Вычисление и обновление состояния. */
    std::memset(&line, 0, sizeof(line));
    line.byte_start = p.bytes[start];
    line.byte_end = p.bytes[end];
    line.byte_next = next;
    line.direction = FRIBIDI_IS_RTL(p.base) ? FVM_TEXT_RTL : FVM_TEXT_LTR;
    line.glyph_start = layout.glyphs.size();
    line.glyph_count = glyphs.size();
    line.advance = advance;
    line.baseline = metrics.ascent + layout.lines.size() * options.line_height;
    if (!(line.baseline <= 1000000) || glyphs.size() > 65536 - layout.glyphs.size())
        return FVM_UNSUPPORTED;
    double available = options.max_width > advance ? options.max_width - advance : 0;
    if (options.alignment == FVM_TEXT_ALIGN_CENTER)
        line.x = available / 2;
    else if (options.alignment == FVM_TEXT_ALIGN_RIGHT ||
             (options.alignment == FVM_TEXT_ALIGN_START && line.direction == FVM_TEXT_RTL))
        line.x = available;
    for (size_t i = 0; i < glyphs.size(); ++i)
    {
        FVM_TextLayoutGlyph g = glyphs[i];
        g.x += line.x;
        g.y += line.baseline;
        g.line = layout.lines.size();
        PathOwner outline;
        double ax, ay;
        result = fvm_font_glyph(font, g.shaped.glyph, &outline.value, &ax, &ay);
        if (result != FVM_OK)
            return result;
        int direction;
        result = outline_underline_direction(outline.value, direction);
        if (result != FVM_OK)
            return result;
        if (direction != FVM_TEXT_LAYOUT_WINDING_NONE)
        {
            if (layout.underline_winding != FVM_TEXT_LAYOUT_WINDING_NONE &&
                layout.underline_winding != direction)
                layout.underline_conflict = true;
            else
                layout.underline_winding = direction;
        }
        FVM_Affine matrix = {1, 0, 0, 1, g.x, g.y};
        result = fvm_path_append(layout.path, outline.value, &matrix);
        if (result != FVM_OK)
            return result;
        layout.outlines.push_back(outline.value);
        outline.value = 0;
        layout.glyphs.push_back(g);
    }
    layout.lines.push_back(line);
    if (advance > layout.metrics.advance_width)
        layout.metrics.advance_width = advance;
    return FVM_OK;
}
/* A C caller can supply an out-of-range enum representation. Inspect its bytes
 * before any C++ scalar load, which would itself be undefined for that value. */
/* Compute and return valid option enums. */
static bool valid_option_enums(const FVM_TextLayoutOptions *options)
{
    bool direction = false, alignment = false;
    for (int i = 0; i <= 2; ++i)
    {
        FVM_TextDirection candidate = (FVM_TextDirection)i;
        if (std::memcmp(&options->direction, &candidate, sizeof(candidate)) == 0)
            direction = true;
    }
    for (int i = 0; i <= 3; ++i)
    {
        FVM_TextAlignment candidate = (FVM_TextAlignment)i;
        if (std::memcmp(&options->alignment, &candidate, sizeof(candidate)) == 0)
            alignment = true;
    }
    return direction && alignment;
}

/* Create text layout. */
extern "C" FVM_Result fvm_text_layout_create_spaced(FVM_Font *font, const char *text, size_t size,
                                             const FVM_TextLayoutOptions *provided, unsigned int flags, double spacing,
                                             FVM_TextLayout **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if ((flags & ~FVM_FONT_SHAPE_NO_KERNING) || !font ||
        !(spacing >= -FVM_TEXT_LAYOUT_MAX_SPACING && spacing <= FVM_TEXT_LAYOUT_MAX_SPACING))
        return FVM_INVALID_ARGUMENT;
    FVM_TextLayoutOptions options = {0, 0, FVM_TEXT_AUTO, FVM_TEXT_ALIGN_START, 0};
    if (provided)
    {
        if (!valid_option_enums(provided))
            return FVM_INVALID_ARGUMENT;
        options = *provided;
    }
    if (!(options.max_width >= 0 && options.max_width <= 1000000) ||
        !(options.line_height >= 0 && options.line_height <= 1000000) ||
        options.direction < FVM_TEXT_AUTO || options.direction > FVM_TEXT_RTL ||
        options.alignment < FVM_TEXT_ALIGN_LEFT || options.alignment > FVM_TEXT_ALIGN_START)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::vector<FriBidiChar> chars;
        fvm_detail::vector<size_t> bytes;
        FVM_Result result = decode(text, size, chars, bytes);
        if (result != FVM_OK)
            return result;
        FVM_FontMetrics metrics;
        result = fvm_font_metrics(font, &metrics);
        if (result != FVM_OK)
            return result;
        if (!options.line_height)
            options.line_height = metrics.line_height;
        if (!(options.line_height > 0))
            return FVM_UNSUPPORTED;
        LayoutOwner layout;
        layout.value = new FVM_TextLayout;
        result = fvm_path_create(&layout.value->path);
        if (result != FVM_OK)
            return result;
        fvm_detail::vector<char> line(size), grapheme(size);
        if (size)
        {
            result =
                fvm_text_breaks(text, size, options.language, line.begin(), 0, grapheme.begin());
            if (result != FVM_OK)
                return result;
        }
        if (!text)
            text = "";
        size_t budget = 1000000;
        for (size_t start = 0;;)
        {
            size_t end = start;
            while (end < chars.size() && !separator(chars[end]))
                ++end;
            size_t next = end;
            if (next < chars.size())
            {
                ++next;
                if (chars[end] == 13 && next < chars.size() && chars[next] == 10)
                    ++next;
            }
            Paragraph p;
            result = paragraph_init(p, chars, bytes, start, end, options.direction);
            if (result != FVM_OK)
                return result;
            size_t line_start = 0;
            do
            {
                size_t line_end;
                result = choose_line(font, text, options.language, p, line_start, options.max_width, flags, spacing,
                                     line, grapheme, budget, line_end);
                if (result != FVM_OK)
                    return result;
                size_t next_byte = line_end == p.chars.size() ? bytes[next] : p.bytes[line_end];
                result = append_line(*layout.value, font, text, options, metrics, p, line_start,
                                     line_end, next_byte, flags, spacing, budget);
                if (result != FVM_OK)
                    return result;
                line_start = line_end;
            } while (line_start < p.chars.size());
            if (end == chars.size())
                break;
            start = next;
        }
        layout.value->metrics.line_count = layout.value->lines.size();
        layout.value->metrics.glyph_count = layout.value->glyphs.size();
        layout.value->metrics.height = metrics.ascent + metrics.descent +
                                       (layout.value->lines.size() - 1) * options.line_height;
        if (!(layout.value->metrics.height <= 1000000))
            return FVM_UNSUPPORTED;
        result = fvm_path_bounds(layout.value->path, &layout.value->metrics.ink);
        if (result != FVM_OK)
            return result;
        *out = layout.value;
        layout.value = 0;
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
#else
/* Create text layout. */
extern "C" FVM_Result fvm_text_layout_create_spaced(FVM_Font *, const char *, size_t,
                                             const FVM_TextLayoutOptions *, unsigned int flags, double spacing, FVM_TextLayout **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    return (flags & ~FVM_FONT_SHAPE_NO_KERNING) ||
           !(spacing >= -FVM_TEXT_LAYOUT_MAX_SPACING && spacing <= FVM_TEXT_LAYOUT_MAX_SPACING)
               ? FVM_INVALID_ARGUMENT : FVM_UNSUPPORTED;
}
#endif

/* Preserve native callers' default HarfBuzz behavior without changing any option layout. */
extern "C" FVM_Result fvm_text_layout_create(FVM_Font *font, const char *text, size_t size,
                                             const FVM_TextLayoutOptions *options,
                                             FVM_TextLayout **out)
{
    return fvm_text_layout_create_configured(font, text, size, options, 0, out);
}

/* Keep the legacy configured ABI and its zero-spacing shaping behavior unchanged. */
extern "C" FVM_Result fvm_text_layout_create_configured(FVM_Font *font, const char *text, size_t size,
                                             const FVM_TextLayoutOptions *options, unsigned int flags,
                                             FVM_TextLayout **out)
{
    return fvm_text_layout_create_spaced(font, text, size, options, flags, 0, out);
}
