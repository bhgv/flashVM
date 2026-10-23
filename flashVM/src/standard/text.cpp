/* FlashVM: Built-in ActionScript objects: text.
 * File: src/standard/text.cpp.
 */

#include "../core/string_internal.hpp"
#include "../core/runtime.hpp"
#include "flashvm/standard_text.h"
#include "flashvm/standard.h"
#include "flashvm/execution.h"
#include "flashvm/utf8.h"
#include <cstring>
#include <cmath>
#include <new>
#include <limits>

#include "text_state.hpp"
#include "text_listeners.hpp"
#include "base_state.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/execution_work_scope.hpp"
#include "flashvm/table.h"
#include "text_constants.h"
/* Release standard text state owned by the destroyed context. */
void fvm_standard_text_cleanup(FVM_Context *context)
{
    for (fvm_detail::map<fvm_detail::string, FVM_FontSource *>::iterator i = context->fonts.begin();
         i != context->fonts.end(); ++i)
        delete i->second;
    context->fonts.clear();
}
namespace
{
/* Define anonymous enum and its representation. */
enum
{
    TEXT_FORMAT = FVM_RESOURCE_TEXT_FORMAT,
    TEXT_FIELD = FVM_RESOURCE_TEXT_FIELD,
    TEXT_FORMAT_CLASS = FVM_RESOURCE_TEXT_FORMAT_CLASS,
    TEXT_FIELD_CLASS = FVM_RESOURCE_TEXT_FIELD_CLASS
};
const char *properties[] = {"font",        "size",          "color",   "bold",        "italic",
                            "underline",   "url",           "target",  "align",       "leftMargin",
                            "rightMargin", "indent",        "leading", "blockIndent", "bullet",
                            "kerning",     "letterSpacing", "tabStops"};

/* Private counted keys cannot collide with public C family names: variant slots
 * begin with NUL, while regular names are nonempty terminated strings. */
static fvm_detail::string font_source_key(const char *name, size_t length, unsigned int variant)
{
    if (!variant)
        return fvm_detail::string(name, length);
    char key[FVM_TEXT_FONT_MAX_NAME + FVM_TEXT_FONT_KEY_PREFIX];
    key[0] = 0;
    key[1] = (char)variant;
    std::memcpy(key + FVM_TEXT_FONT_KEY_PREFIX, name, length);
    return fvm_detail::string(key, length + FVM_TEXT_FONT_KEY_PREFIX);
}

/* Keep the new Flash8 paragraph policy unavailable to older loaded movies. */
static bool justify_available(FVM_Context *context)
{
    return !context->swf_loaded || context->swf_info.version >= FVM_TEXT_JUSTIFY_MIN_VERSION;
}

/* Define Style struct and its representation. */
struct Style
{
    fvm_detail::string font; /* Public family name retained by the format. */
    fvm_detail::string source_key; /* Precomputed private slot key; layout lookup cannot allocate. */
    unsigned int variant; /* Explicit regular/bold/italic family selection. */
    double size, leading;
    fvm_detail::vector<double> tab_stops; /* Native owned copy, independent of the source managed Array. */
    double indent; /* First visual line only; zero for empty hard paragraphs. */
    double left_margin, right_margin, block_indent; /* Nonnegative whole-pixel paragraph insets. */
    double letter_spacing; /* Signed extra pixels after each supported character. */
    unsigned long color;
    FVM_TextAlignment alignment;
    bool underline; /* Uniform line decoration, independent of glyph shaping. */
    bool kerning; /* Explicit AS2 pair-kerning selection, independent of font ownership. */
    /* Initialize Style and its owned state. */
    Style() : variant(FVM_TEXT_FONT_REGULAR), size(12), leading(2), indent(0), left_margin(0), right_margin(0), block_indent(0), letter_spacing(0), color(0), alignment(FVM_TEXT_ALIGN_LEFT), underline(false), kerning(false)
    {
    }
    /* Exchange the current value. */
    void swap(Style &s)
    {
        tab_stops.swap(s.tab_stops);
        font.swap(s.font);
        source_key.swap(s.source_key);
        unsigned int v = variant; variant = s.variant; s.variant = v;
        double d = size;

        /* Computation / Вычисление и обновление состояния. */
        size = s.size;
        s.size = d;
        d = leading;
        leading = s.leading;
        s.leading = d;
        d = indent; indent = s.indent; s.indent = d;
        d = left_margin; left_margin = s.left_margin; s.left_margin = d;
        d = right_margin; right_margin = s.right_margin; s.right_margin = d;
        d = block_indent; block_indent = s.block_indent; s.block_indent = d;
        d = letter_spacing;
        letter_spacing = s.letter_spacing;
        s.letter_spacing = d;
        unsigned long c = color;

        /* Computation / Вычисление и обновление состояния. */
        color = s.color;
        s.color = c;
        FVM_TextAlignment a = alignment;

        /* Computation / Вычисление и обновление состояния. */
        alignment = s.alignment;
        s.alignment = a;
        bool u = underline;
        underline = s.underline;
        s.underline = u;
        bool k = kerning;
        kerning = s.kerning;
        s.kerning = k;
    }
};

/* Keep hanging text inside the paragraph's left gutter and its SWF8 version gate. */
static FVM_Result indent_policy(FVM_Context *context, const Style &style)
{
    if (style.indent < 0 &&
        ((context->swf_loaded && context->swf_info.version < FVM_TEXT_INDENT_NEGATIVE_MIN_VERSION) ||
         style.left_margin + style.block_indent + style.indent < 0))
        return FVM_UNSUPPORTED;
    return FVM_OK;
}

/* Define Field struct and its representation. */
struct Field
{
    Style current, future;
    const char *text; /* [GC:] Context-owned text retained by the reachable field. */
    size_t text_size; /* Counted UTF-8 bytes, including embedded NUL. */
    double width /* Current outer width including gutter. */, height; /* Current outer height. */
    bool multiline /* Allow hard line separators. */, wrap; /* Wrap inside the outer width. */
    bool auto_size_left; /* Automatic fitting enabled; anchor chooses the horizontal invariant. */
    double x, y; /* Parent-space translation; exported outlines remain field-local. */
    unsigned long anchor; /* Left, center or right automatic horizontal anchor. */
    unsigned long scroll, hscroll; /* Stored one-based line and zero-based pixel requests. */
    FVM_TextLayout *layout; /* Owned cache, replaced only after a successful measured edit. */
    /* Initialize Field and its owned state. */
    Field() : text(""), text_size(0), width(100), height(100), multiline(false), wrap(false), auto_size_left(false), x(0), y(0), anchor(FVM_TEXT_ANCHOR_LEFT), scroll(1), hscroll(0), layout(0)
    {
    }
    /* Release the resources owned by this Field instance. */
    ~Field()
    {
        fvm_text_layout_destroy(layout);
    }
    /* Implement invalidate for this subsystem. */
    void invalidate()
    {
        fvm_text_layout_destroy(layout);
        layout = 0;
    }
};

/* Define FontOwner struct and its representation. */
struct FontOwner
{
    FVM_Font *font;
    /* Initialize Font Owner and its owned state. */
    FontOwner() : font(0)
    {
    }
    /* Release the resources owned by this FontOwner instance. */
    ~FontOwner()
    {
        fvm_font_destroy(font);
    }

  private:
    /* Initialize Font Owner and its owned state. */
    FontOwner(const FontOwner &);
    FontOwner &operator=(const FontOwner &);
};

/* Define LayoutOwner struct and its representation. */
struct LayoutOwner
{
    FVM_TextLayout *layout;
    /* Initialize Layout Owner and its owned state. */
    LayoutOwner() : layout(0)
    {
    }
    /* Release the resources owned by this LayoutOwner instance. */
    ~LayoutOwner()
    {
        fvm_text_layout_destroy(layout);
    }

  private:
    /* Initialize Layout Owner and its owned state. */
    LayoutOwner(const LayoutOwner &);
    LayoutOwner &operator=(const LayoutOwner &);
};

/* Release field. */
static void release_field(void *resource)
{
    delete (Field *)resource;
}

/* Implement object value for this subsystem. */
static void object_value(FVM_Value *out, FVM_Object *object)
{
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    out->type = FVM_VALUE_OBJECT;
    out->object = object;
}

/* Compute and return defined. */
static bool defined(const FVM_Value &v)
{
    return v.type != FVM_VALUE_NULL && v.type != FVM_VALUE_UNDEFINED;
}

/* Compute and return finite range. */
static bool finite_range(double d, double minimum, double maximum)
{
    return d >= minimum && d <= maximum;
}

/* Compute and return number. */
static FVM_Result number(FVM_Context *c, const FVM_Value &v, double &out)
{
    FVM_Result result = fvm_value_to_number(c, &v, &out);
    return result != FVM_OK ? result
           : finite_range(out, -std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max())
               ? FVM_OK
               : FVM_INVALID_ARGUMENT;
}
/* Match a formatting keyword by its complete counted bytes, never a terminated prefix. */
static bool style_keyword(const FVM_Value &value, const char *expected)
{
    size_t size = std::strlen(expected);
    return value.type == FVM_VALUE_STRING && fvm_string_size(value) == size &&
           !std::memcmp(value.string, expected, size);
}

/* [GC:] Root the borrowed Array through length/index getters; retain copied numbers only. */
static FVM_Result read_tab_stops(FVM_Context *c, const FVM_Value &value, fvm_detail::vector<double> &out)
{
    if (value.type != FVM_VALUE_OBJECT || !value.object || value.object->owner != c ||
        value.object->resource_kind != FVM_RESOURCE_ARRAY) return FVM_UNSUPPORTED;
    FVM_ObjectRootScope root(c, value.object);
    FVM_ExecutionWorkScope work(c);
    FVM_Value length;
    FVM_Result result = fvm_object_get(c, value.object, "length", &length);
    if (result != FVM_OK) return result;
    if (length.type != FVM_VALUE_NUMBER || !finite_range(length.number, 0, FVM_TEXT_STYLE_MAX_TABS) ||
        length.number != std::floor(length.number)) return FVM_INVALID_ARGUMENT;
    size_t count = (size_t)length.number;
    fvm_detail::vector<double> next;
    for (size_t i = 0; i < count; ++i)
    {
        result = work.step();
        if (result != FVM_OK) return result;
        FVM_Value stop;
        result = fvm_table_get_index(c, value.object, (unsigned long)i, &stop);
        if (result != FVM_OK) return result;
        if (stop.type != FVM_VALUE_NUMBER || !finite_range(stop.number, 0, FVM_TEXT_STYLE_MAX_TAB_STOP) ||
            stop.number != std::floor(stop.number) || (i && stop.number <= next[i - 1]))
            return FVM_INVALID_ARGUMENT;
        next.push_back(stop.number);
    }
    out.swap(next);
    return FVM_OK;
}

/* Only explicitly implemented formatting is applied. Null/undefined properties
 * leave the previous style unchanged; unsupported visible effects are rejected. */
/* Compute and return merge style. */
static FVM_Result merge_style(FVM_Context *c, FVM_Object *format, Style &style)
{
    if (!format || format->owner != c || format->resource_kind != TEXT_FORMAT)
        return FVM_INVALID_ARGUMENT;
    for (size_t i = 0; i < sizeof(properties) / sizeof(properties[0]); ++i)
    {
        FVM_Value v;
        FVM_Result result = fvm_object_get(c, format, properties[i], &v);
        if (result != FVM_OK)
            return result;
        if (!defined(v))
            continue;
        const char *name = properties[i];
        if (std::strcmp(name, "font") == 0)
        {
            if (v.type != FVM_VALUE_STRING ||
                std::memchr(v.string, 0, fvm_string_size(v)))
                return FVM_UNSUPPORTED;
            style.font = fvm_detail::string(v.string, fvm_string_size(v));
        }
        else if (std::strcmp(name, "align") == 0)
        {
            if (v.type != FVM_VALUE_STRING)
                return FVM_UNSUPPORTED;
            if (style_keyword(v, "left"))
                style.alignment = FVM_TEXT_ALIGN_LEFT;
            else if (style_keyword(v, "center"))
                style.alignment = FVM_TEXT_ALIGN_CENTER;
            else if (style_keyword(v, "right"))
                style.alignment = FVM_TEXT_ALIGN_RIGHT;
            else if (style_keyword(v, "justify") && justify_available(c))
                style.alignment = FVM_TEXT_ALIGN_JUSTIFY;
            else
                return FVM_UNSUPPORTED;
        }
        else if (std::strcmp(name, "url") == 0 || std::strcmp(name, "target") == 0)
        {
            if (v.type != FVM_VALUE_STRING || fvm_string_size(v))
                return FVM_UNSUPPORTED;
        }
        else if (std::strcmp(name, "kerning") == 0 || std::strcmp(name, "underline") == 0 ||
                 std::strcmp(name, "bold") == 0 || std::strcmp(name, "italic") == 0)
        {
            FVM_Value converted;
            result = fvm_exec_to_boolean(c, &v, &converted);
            if (result != FVM_OK)
                return result;
            if (std::strcmp(name, "bold") == 0 || std::strcmp(name, "italic") == 0)
            {
                unsigned int bit = std::strcmp(name, "bold") == 0 ? FVM_TEXT_FONT_BOLD : FVM_TEXT_FONT_ITALIC;
                if (converted.boolean) style.variant |= bit;
                else style.variant &= ~bit;
            }
            else if (std::strcmp(name, "underline") == 0)
                style.underline = converted.boolean != 0;
            else
                style.kerning = converted.boolean != 0;
        }
        else if (std::strcmp(name, "tabStops") == 0)
        {
            result = read_tab_stops(c, v, style.tab_stops);
            if (result != FVM_OK) return result;
        }
        else
        {
            double d;
            result = number(c, v, d);
            if (result != FVM_OK)
                return result;
            if (std::strcmp(name, "size") == 0)
            {
                if (!finite_range(d, 1, 4096))
                    return FVM_INVALID_ARGUMENT;
                style.size = d;
            }
            else if (std::strcmp(name, "leading") == 0)
            {
                if (!finite_range(d, -4096, 4096))
                    return FVM_INVALID_ARGUMENT;
                style.leading = d;
            }
            else if (std::strcmp(name, "letterSpacing") == 0)
            {
                if (!finite_range(d, -FVM_TEXT_STYLE_MAX_SPACING, FVM_TEXT_STYLE_MAX_SPACING))
                    return FVM_INVALID_ARGUMENT;
                style.letter_spacing = d;
            }
            else if (std::strcmp(name, "indent") == 0)
            {
                if (!finite_range(d, -FVM_TEXT_STYLE_MAX_INDENT, FVM_TEXT_STYLE_MAX_INDENT) || d != std::floor(d))
                    return FVM_INVALID_ARGUMENT;
                style.indent = d;
            }
            else if (std::strcmp(name, "leftMargin") == 0 || std::strcmp(name, "rightMargin") == 0 ||
                     std::strcmp(name, "blockIndent") == 0)
            {
                if (!finite_range(d, 0, FVM_TEXT_STYLE_MAX_INSET) || d != std::floor(d))
                    return FVM_INVALID_ARGUMENT;
                if (!std::strcmp(name, "leftMargin")) style.left_margin = d;
                else if (!std::strcmp(name, "rightMargin")) style.right_margin = d;
                else style.block_indent = d;
            }
            else if (std::strcmp(name, "color") == 0)
            {
                if (!finite_range(d, 0, 16777215) || d != std::floor(d))
                    return FVM_INVALID_ARGUMENT;
                style.color = (unsigned long)d;
            }
            else if (d != 0)
                return FVM_UNSUPPORTED;
        }
    }
    if (style.font.size() > FVM_TEXT_FONT_MAX_NAME ||
        std::memchr(style.font.c_str(), 0, style.font.size()))
        return FVM_INVALID_ARGUMENT;
    FVM_Result policy = indent_policy(c, style);
    if (policy != FVM_OK) return policy;
    style.source_key = font_source_key(style.font.c_str(), style.font.size(), style.variant);
    return FVM_OK;
}

/* Open font. */
static FVM_Result open_font(FVM_Context *c, const Style &style, FVM_Font **out)
{
    *out = 0;
#ifndef FVM_HAS_FONTS
    (void)c;
    (void)style;
    return FVM_UNSUPPORTED;
#else
    fvm_detail::map<fvm_detail::string, FVM_FontSource *>::iterator source =
        c->fonts.find(style.source_key);
    if (source == c->fonts.end())
        return FVM_NOT_FOUND;
    return fvm_font_read(source->second->bytes.begin(), source->second->bytes.size(),
                         source->second->index, style.size, out);
#endif
}

/* Build layout. */
static FVM_Result build_layout(FVM_Context *c, const Style &style, const char *text, size_t size,
                               double width, bool multiline, FVM_TextLayout **out, FVM_FontMetrics *font_metrics)
{

    /* Initialization / Инициализация локального состояния. */
    *out = 0;
    if (style.alignment == FVM_TEXT_ALIGN_JUSTIFY && !justify_available(c))
        return FVM_UNSUPPORTED;
    FVM_Result policy = indent_policy(c, style);
    if (policy != FVM_OK) return policy;
    FontOwner font;
    FVM_Result result = open_font(c, style, &font.font);
    if (result != FVM_OK)
        return result;
    if (style.underline)
    {
        FVM_FontUnderlineMetrics underline;
        result = fvm_font_underline_metrics(font.font, &underline);
        if (result != FVM_OK)
            return result;
    }
    FVM_FontMetrics metrics;
    result = fvm_font_metrics(font.font, &metrics);
    if (result != FVM_OK)
        return result;
    double spacing = metrics.ascent + metrics.descent + style.leading;
    if (!finite_range(spacing, 1.0 / 64, 1000000))
        return FVM_INVALID_ARGUMENT;
    size_t offset = 0;
    while (!multiline && offset < size)
    {
        unsigned long cp;
        if (fvm_utf8_read(text, size, &offset, &cp) != FVM_UTF8_OK)
            return FVM_INVALID_ARGUMENT;
        if (cp == 10 || cp == 13 || cp == 11 || cp == 12 || cp == 0x85 || cp == 0x2028 ||
            cp == 0x2029)
            return FVM_UNSUPPORTED;
    }
    FVM_TextLayoutOptions options = {0, 0, FVM_TEXT_AUTO, FVM_TEXT_ALIGN_LEFT, 0};

    /* Computation / Вычисление и обновление состояния. */
    options.max_width = width;
    if (width > 0)
    {
        options.max_width -= style.left_margin + style.right_margin + style.block_indent;
        if (options.max_width <= 0) options.max_width = FVM_TEXT_FIELD_MIN_INNER_WIDTH;
    }
    options.line_height = spacing;
    options.alignment = width ? style.alignment : FVM_TEXT_ALIGN_LEFT;
    if (style.tab_stops.empty() && std::memchr(text, FVM_TEXT_FIELD_TAB, size))
    {
        double average = 0;
        result = fvm_font_average_width(font.font, &average);
        if (result != FVM_OK) return result;
        double period = average * FVM_TEXT_FIELD_TAB_AVERAGE_MULTIPLIER;
        if (!(period > 0 && period <= FVM_TEXT_FIELD_MAX_EXTENT)) return FVM_UNSUPPORTED;
        result = fvm_text_layout_create_periodic(font.font, text, size, &options,
                style.kerning ? 0 : FVM_FONT_SHAPE_NO_KERNING, style.letter_spacing,
                style.indent, period, out);
    }
    else
    {
        result = fvm_text_layout_create_tabbed(font.font, text, size, &options,
                style.kerning ? 0 : FVM_FONT_SHAPE_NO_KERNING, style.letter_spacing,
                style.indent, style.tab_stops.empty() ? 0 : style.tab_stops.begin(),
                style.tab_stops.size(), out);
    }
    if (result == FVM_OK && font_metrics)
        *font_metrics = metrics;
    return result;
}

/* Own a candidate layout and scalar geometry until every fallible edit succeeds. */
struct PreparedGeometry
{
    LayoutOwner owned; /* Unpublished layout; cleanup unwinds failed measurements. */
    double width, height; /* Candidate outer dimensions including the field gutter. */
    bool multiline, wrap; /* Candidate layout policies. */
    bool automatic; /* Fitting is active for this candidate. */
    double x; /* Candidate horizontal origin, published with measured width. */
    unsigned long anchor; /* Candidate horizontal fitting policy. */
    /* Copy only scalar policies; never copy a field's owned layout pointer. */
    PreparedGeometry(const Field &field) : width(field.width), height(field.height),
        multiline(field.multiline), wrap(field.wrap), automatic(field.auto_size_left), x(field.x), anchor(field.anchor) {}
    /* Publish without allocation after text/style admission and ownership succeed. */
    void commit(Field &field)
    {
        field.invalidate();
        field.width = width;
        field.height = height;
        field.multiline = multiline;
        field.wrap = wrap;
        field.auto_size_left = automatic;
        field.x = x;
        field.anchor = anchor;
        if (owned.layout)
        {
            field.layout = owned.layout;
            owned.layout = 0;
        }
    }
};

/* Measure before publication for tracking or automatic dimensions. Manual,
 * zero-spacing fields retain lazy layout; automatic fields retain the measured cache. */
static FVM_Result prepare_field_geometry(FVM_Context *c, const Style &style,
                      const char *text, size_t size, PreparedGeometry &next)
{
    if (!next.automatic && style.letter_spacing == 0 && style.alignment != FVM_TEXT_ALIGN_JUSTIFY &&
        style.left_margin == 0 && style.right_margin == 0 && style.block_indent == 0 && style.indent == 0 && style.tab_stops.empty() &&
        !std::memchr(text, FVM_TEXT_FIELD_TAB, size))
        return FVM_OK;
    double width = next.wrap && next.multiline ? next.width - FVM_TEXT_FIELD_GUTTER : 0;
    if (next.wrap && next.multiline && width <= 0)
        width = FVM_TEXT_FIELD_MIN_INNER_WIDTH;
    FVM_Result result = build_layout(c, style, text, size, width, next.multiline, &next.owned.layout, 0);
    if (result != FVM_OK || !next.automatic)
        return result;
    FVM_TextLayoutMetrics metrics;
    result = fvm_text_layout_metrics(next.owned.layout, &metrics);
    if (result != FVM_OK)
        return result;
    if (!next.wrap)
    {
        double measured = metrics.advance_width + FVM_TEXT_FIELD_GUTTER +
                          style.left_margin + style.right_margin + style.block_indent;
        double fraction = next.anchor == FVM_TEXT_ANCHOR_RIGHT ? 1.0 :
                          next.anchor == FVM_TEXT_ANCHOR_CENTER ? 0.5 : 0.0;
        next.x += (next.width - measured) * fraction;
        next.width = measured;
    }
    next.height = metrics.height + FVM_TEXT_FIELD_GUTTER;
    if (!finite_range(next.x, -FVM_TEXT_FIELD_MAX_POSITION, FVM_TEXT_FIELD_MAX_POSITION) ||
        !finite_range(next.width, 0, FVM_TEXT_FIELD_MAX_EXTENT) ||
        !finite_range(next.height, 0, FVM_TEXT_FIELD_MAX_EXTENT))
        return FVM_UNSUPPORTED;
    return FVM_OK;
}

/* Compute and return ensure layout. */
static FVM_Result ensure_layout(FVM_Context *c, Field *field)
{
    if (field->current.alignment == FVM_TEXT_ALIGN_JUSTIFY && !justify_available(c))
        return FVM_UNSUPPORTED;
    FVM_Result policy = indent_policy(c, field->current);
    if (policy != FVM_OK) return policy;
    if (field->layout)
        return FVM_OK;
    double width = field->wrap && field->multiline ? field->width - FVM_TEXT_FIELD_GUTTER : 0;
    if (field->wrap && field->multiline && width <= 0)
        width = FVM_TEXT_FIELD_MIN_INNER_WIDTH;
    return build_layout(c, field->current, field->text, field->text_size, width,
                        field->multiline, &field->layout, 0);
}

/* Compute uniform visual-line viewport geometry without publishing any field state. */
static FVM_Result scroll_geometry(FVM_Context *context, Field *field,
                                  FVM_TextFieldScrollInfo &info)
{
    FVM_Result result = ensure_layout(context, field);
    if (result != FVM_OK)
        return result;
    FVM_TextLayoutMetrics metrics;
    result = fvm_text_layout_metrics(field->layout, &metrics);
    if (result != FVM_OK)
        return result;
    if (!metrics.line_count || metrics.line_count > FVM_TEXT_FIELD_MAX_SCROLL_LINES)
        return FVM_UNSUPPORTED;
    double step = metrics.height + field->current.leading;
    if (metrics.line_count > 1)
    {
        FVM_TextLayoutLine first, second;
        result = fvm_text_layout_line(field->layout, 0, &first);
        if (result == FVM_OK)
            result = fvm_text_layout_line(field->layout, 1, &second);
        if (result != FVM_OK)
            return result;
        step = second.baseline - first.baseline;
    }
    if (!finite_range(step, FVM_TEXT_FIELD_MIN_INNER_WIDTH, FVM_TEXT_FIELD_MAX_EXTENT))
        return FVM_UNSUPPORTED;
    double inner_width = field->width > FVM_TEXT_FIELD_GUTTER ? field->width - FVM_TEXT_FIELD_GUTTER : 0;
    double inner_height = field->height > FVM_TEXT_FIELD_GUTTER ? field->height - FVM_TEXT_FIELD_GUTTER : 0;
    double body = metrics.height - (metrics.line_count - 1) * step;
    size_t visible = 1;
    if (inner_height >= body)
    {
        double fit = 1 + std::floor((inner_height - body) / step);
        visible = fit >= metrics.line_count ? metrics.line_count : (size_t)fit;
    }
    double overflow = metrics.advance_width + field->current.left_margin +
        field->current.right_margin + field->current.block_indent - inner_width;
    if (overflow < 0)
        overflow = 0;
    overflow = std::ceil(overflow);
    if (!finite_range(overflow, 0, FVM_TEXT_FIELD_MAX_SCROLL_PIXELS))
        return FVM_UNSUPPORTED;
    std::memset(&info, 0, sizeof(info));
    info.struct_size = sizeof(info);
    info.line_count = metrics.line_count;
    info.line_height = step;
    info.maxscroll = (unsigned long)(metrics.line_count - visible + 1);
    info.maxhscroll = (unsigned long)overflow;
    info.scroll = field->scroll < info.maxscroll ? field->scroll : info.maxscroll;
    info.hscroll = field->hscroll < info.maxhscroll ? field->hscroll : info.maxhscroll;
    info.bottom_scroll = (unsigned long)(info.scroll + visible - 1);
    info.offset_x = -(double)info.hscroll;
    info.offset_y = -(double)(info.scroll - 1) * step;
    info.clip_x = info.clip_y = FVM_TEXT_FIELD_GUTTER / 2;
    info.clip_width = inner_width;
    info.clip_height = inner_height;
    return FVM_OK;
}

/* [GC:] Prepare geometry observers without running getters or publishing candidate state.
 * Fields without a potential handler retain lazy layout admission. */
class GeometryNotification
{
    FVM_Context *context_; /* Borrowed owner, alive through synchronous dispatch. */
    Field *field_; /* Native resource rooted by plan_. */
    FVM_Object *object_; /* Source identity, never the temporary candidate. */
    FVM_TextFieldScrollPlan plan_; /* [GC:] Roots recipients before publication. */
    bool observed_, changed_; /* Candidate admission and delivery are independent. */
    unsigned long scroll_, horizontal_; /* Candidate clamped offsets. */
    GeometryNotification(const GeometryNotification &); /* Active roots cannot be copied. */
    GeometryNotification &operator=(const GeometryNotification &); /* Unique transaction ownership. */
  public:
    /* [GC:] Establish a rooted plan before any candidate allocation. */
    GeometryNotification(FVM_Context *context, FVM_Object *object)
        : context_(context), field_((Field *)object->resource), object_(object),
          plan_(context, object), observed_(false), changed_(false), scroll_(1), horizontal_(0) {}
    /* Detect potential observers by native descriptors only, including inherited handlers. */
    FVM_Result prepare(PreparedGeometry &geometry, const Style &style,
                       const char *text, size_t size)
    {
        if (context_->swf_loaded && context_->swf_info.version < FVM_TEXT_LISTENER_MIN_VERSION)
            return FVM_OK;
        FVM_Result probe = fvm_textfield_observer_present(context_, object_, &observed_);
        if (probe != FVM_OK || !observed_)
            return probe;
        FVM_TextFieldScrollInfo before, after;
        FVM_Result result = scroll_geometry(context_, field_, before);
        if (result != FVM_OK) return result;
        Field candidate;
        candidate.current = style;
        candidate.text = text;
        candidate.text_size = size;
        candidate.width = geometry.width;
        candidate.height = geometry.height;
        candidate.multiline = geometry.multiline;
        candidate.wrap = geometry.wrap;
        candidate.scroll = field_->scroll;
        candidate.hscroll = field_->hscroll;
        result = scroll_geometry(context_, &candidate, after);
        if (result != FVM_OK) return result;
        scroll_ = after.scroll;
        horizontal_ = after.hscroll;
        changed_ = before.scroll != after.scroll || before.hscroll != after.hscroll ||
                   before.maxscroll != after.maxscroll || before.maxhscroll != after.maxhscroll;
        if (changed_)
        {
            result = plan_.prepare();
            if (result != FVM_OK) return result;
        }
        /* Transfer the measured cache only after every prepublication check succeeds. */
        fvm_text_layout_destroy(geometry.owned.layout);
        geometry.owned.layout = candidate.layout;
        candidate.layout = 0;
        return FVM_OK;
    }
    /* Geometry-only edits borrow the current content/style without copying native ownership. */
    FVM_Result prepare(PreparedGeometry &geometry)
    {
        return prepare(geometry, field_->current, field_->text, field_->text_size);
    }
    /* Publish geometry and clamp together, then deliver without overwriting nested edits. */
    FVM_Result commit(PreparedGeometry &geometry)
    {
        geometry.commit(*field_);
        if (observed_)
        {
            field_->scroll = scroll_;
            field_->hscroll = horizontal_;
        }
        return changed_ ? plan_.dispatch() : FVM_OK;
    }
};

/* Read extent. */
static FVM_Result get_extent(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *a,
                             FVM_Value *out, void *)
{
    if (n < 1 || n > 2 || a[0].type != FVM_VALUE_STRING)
        return FVM_UNSUPPORTED;
    Style style;
    style.font = c->default_font;
    FVM_Result result = merge_style(c, self, style);
    if (result != FVM_OK)
        return result;
    double width = 0;
    if (n == 2)
    {
        result = number(c, a[1], width);
        if (result != FVM_OK || width <= 0)
            return FVM_INVALID_ARGUMENT;
    }

    /* Initialization / Инициализация локального состояния. */
    LayoutOwner layout;
    FVM_FontMetrics font_metrics;
    result = build_layout(c, style, a[0].string, fvm_string_size(a[0]), width, true,
                          &layout.layout, &font_metrics);
    if (result != FVM_OK)
        return result;
    FVM_TextLayoutMetrics metrics;
    result = fvm_text_layout_metrics(layout.layout, &metrics);
    if (result != FVM_OK)
        return result;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *object = 0;
    result = fvm_object_create(c, &object);
    if (result != FVM_OK)
        return result;

    /* Initialization / Инициализация локального состояния. */
    const char *names[] = {"ascent", "descent",        "width",
                           "height", "textFieldWidth", "textFieldHeight"};
    const double values[] = {font_metrics.ascent, font_metrics.descent,      metrics.advance_width,
                             metrics.height,      metrics.advance_width + FVM_TEXT_FIELD_GUTTER + style.left_margin + style.right_margin + style.block_indent, metrics.height + FVM_TEXT_FIELD_GUTTER};
    for (size_t i = 0; i < 6; ++i)
    {
        FVM_Value value;
        fvm_value_number(&value, values[i]);
        object->properties[names[i]] = value;
    }
    object_value(out, object);
    return FVM_OK;
}

/* Create format. */
static FVM_Result create_format(FVM_Context *c, FVM_Object *prototype, unsigned int n,
                                const FVM_Value *args, FVM_Object **out)
{
    *out = 0;
    if (n > 13)
        return FVM_UNSUPPORTED;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *object = 0;
    FVM_Result result = fvm_object_create(c, &object);
    if (result != FVM_OK)
        return result;

    /* Computation / Вычисление и обновление состояния. */
    object->resource_kind = TEXT_FORMAT;
    object->prototype = prototype;
    FVM_NativeEntry method;

    /* Computation / Вычисление и обновление состояния. */
    method.callback = get_extent;
    object->methods["getTextExtent"] = method;

    /* Initialization / Инициализация локального состояния. */
    FVM_Value null_value;
    fvm_value_undefined(&null_value);
    null_value.type = FVM_VALUE_NULL;
    for (size_t i = 0; i < sizeof(properties) / sizeof(properties[0]); ++i)
        object->properties[properties[i]] = null_value;
    FVM_Value kerning_default;
    fvm_value_boolean(&kerning_default, 0);
    object->properties["kerning"] = kerning_default;
    for (unsigned int i = 0; i < n; ++i)
        if (args[i].type != FVM_VALUE_UNDEFINED)
        {
            result = fvm_object_set(c, object, properties[i], &args[i]);
            if (result != FVM_OK)
                return result;
        }
    *out = object;
    return FVM_OK;
}

/* Compute and return format constructor. */
static FVM_Result format_constructor(FVM_Context *c, FVM_Object *, unsigned int n,
                                     const FVM_Value *args, FVM_Value *out, void *userdata)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *klass = (FVM_Object *)userdata, *object = 0;
    FVM_Value prototype;
    FVM_Result result = fvm_object_get(c, klass, "prototype", &prototype);
    if (result != FVM_OK)
        return result;
    if (prototype.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    result = create_format(c, prototype.object, n, args, &object);
    if (result == FVM_OK)
        object_value(out, object);
    return result;
}

/* Compute and return style format. */
static FVM_Result style_format(FVM_Context *c, const Style &style, FVM_Value *out)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value klass, prototype;
    FVM_Result result = fvm_get_global(c, "TextFormat", &klass);
    if (result != FVM_OK || klass.type != FVM_VALUE_OBJECT ||
        klass.object->resource_kind != TEXT_FORMAT_CLASS)
        return FVM_INVALID_ARGUMENT;
    result = fvm_object_get(c, klass.object, "prototype", &prototype);
    if (result != FVM_OK || prototype.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *object = 0;
    result = create_format(c, prototype.object, 0, 0, &object);
    if (result != FVM_OK)
        return result;

    /* [GC:] Keep both fresh managed snapshots reachable while native property storage allocates. */
    FVM_ObjectRootScope format_root(c, object);
    FVM_Object *tabs = 0;
    result = fvm_array_create_like(c, 0, (unsigned long)style.tab_stops.size(), &tabs, true);
    if (result != FVM_OK) return result;
    FVM_ObjectRootScope tabs_root(c, tabs);
    FVM_Value v;
    for (size_t i = 0; i < style.tab_stops.size(); ++i)
    {
        fvm_value_number(&v, style.tab_stops[i]);
        result = fvm_array_store_own(c, tabs, (unsigned long)i, v);
        if (result != FVM_OK) return result;
    }
    object_value(&v, tabs);
    object->properties["tabStops"] = v;
    fvm_value_string(&v, style.font.c_str());
    result = fvm_object_set(c, object, "font", &v);
    if (result != FVM_OK)
        return result;
    fvm_value_number(&v, style.size);
    object->properties["size"] = v;
    fvm_value_number(&v, style.leading);
    object->properties["leading"] = v;
    fvm_value_number(&v, style.letter_spacing);
    object->properties["letterSpacing"] = v;
    fvm_value_number(&v, style.indent); object->properties["indent"] = v;
    fvm_value_number(&v, style.left_margin); object->properties["leftMargin"] = v;
    fvm_value_number(&v, style.right_margin); object->properties["rightMargin"] = v;
    fvm_value_number(&v, style.block_indent); object->properties["blockIndent"] = v;
    fvm_value_number(&v, style.color);
    object->properties["color"] = v;
    fvm_value_boolean(&v, (style.variant & FVM_TEXT_FONT_BOLD) != 0);
    object->properties["bold"] = v;
    fvm_value_boolean(&v, (style.variant & FVM_TEXT_FONT_ITALIC) != 0);
    object->properties["italic"] = v;
    fvm_value_boolean(&v, style.underline);
    object->properties["underline"] = v;
    fvm_value_boolean(&v, style.kerning);
    object->properties["kerning"] = v;

    /* Initialization / Инициализация локального состояния. */
    const char *align = style.alignment == FVM_TEXT_ALIGN_LEFT     ? "left"
                        : style.alignment == FVM_TEXT_ALIGN_CENTER ? "center"
                        : style.alignment == FVM_TEXT_ALIGN_RIGHT ? "right" : "justify";
    fvm_value_string(&v, align);
    result = fvm_object_set(c, object, "align", &v);
    if (result != FVM_OK)
        return result;
    object_value(out, object);
    return FVM_OK;
}
/* Public field indices count UTF-16 units. The current string model stores only
 * Unicode scalars, so a boundary inside a surrogate pair cannot be represented.
 * Validate the entire string and never publish a partial offset on failure. */
/* Compute and return text boundary. */
static FVM_Result text_boundary(const char *text, size_t bytes, size_t index, size_t &byte, size_t &length)
{

    /* Initialization / Инициализация локального состояния. */
    size_t offset = 0, units = 0, found = 0;
    bool boundary = index == 0;
    while (offset < bytes)
    {
        unsigned long cp;
        if (fvm_utf8_read(text, bytes, &offset, &cp) != FVM_UTF8_OK)
            return FVM_INVALID_ARGUMENT;
        size_t next = units + (cp > 0xffff ? 2 : 1);
        if (index > units && index < next)
            return FVM_UNSUPPORTED;
        units = next;
        if (index == units)
        {
            found = offset;
            boundary = true;
        }
    }
    if (!boundary)
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    byte = found;
    length = units;
    return FVM_OK;
}

/* Compute and return text index. */
static FVM_Result text_index(FVM_Context *c, const FVM_Value &value, size_t &index)
{

    /* Initialization / Инициализация локального состояния. */
    double d;
    FVM_Result result = number(c, value, d);
    if (result != FVM_OK)
        return result;
    /* 4096 supported scalars can occupy at most 8192 UTF-16 units. */
    if (!finite_range(d, 0, FVM_TEXT_FIELD_MAX_UNITS) || d != std::floor(d))
        return FVM_INVALID_ARGUMENT;
    index = (size_t)d;
    return FVM_OK;
}

/* Compute and return text range. */
static FVM_Result text_range(FVM_Context *c, const char *text, size_t bytes, unsigned int n,
                             const FVM_Value *args, size_t &begin, size_t &end, size_t &length)
{

    /* Initialization / Инициализация локального состояния. */
    size_t first, last, ignored;
    FVM_Result result = text_index(c, args[0], first);
    if (result != FVM_OK)
        return result;
    last = first + 1;
    if (n == 2)
    {
        result = text_index(c, args[1], last);
        if (result != FVM_OK)
            return result;
    }
    if (last < first)
        return FVM_INVALID_ARGUMENT;
    result = text_boundary(text, bytes, first, begin, length);
    if (result != FVM_OK)
        return result;
    return text_boundary(text, bytes, last, end, ignored);
}

/* Compute and return replace text. */
static FVM_Result replace_text(FVM_Context *c, FVM_Object *self, unsigned int n,
                               const FVM_Value *args, FVM_Value *out, void *)
{
    if (!self || self->resource_kind != TEXT_FIELD || !self->resource)
        return FVM_INVALID_ARGUMENT;
    if (n != 3 || args[2].type != FVM_VALUE_STRING)
        return FVM_UNSUPPORTED;

    /* Initialization / Инициализация локального состояния. */
    Field *field = (Field *)self->resource;
    size_t begin, end, length;
    FVM_Result result = text_range(c, field->text, field->text_size, 2, args, begin, end, length);
    if (result != FVM_OK)
        return result;

    /* Initialization / Инициализация локального состояния. */
    const char *replacement = args[2].string;
    size_t bytes = field->text_size, inserted = fvm_string_size(args[2]), count;
    if (inserted > FVM_TEXT_FIELD_MAX_BYTES)
        return FVM_UNSUPPORTED;
    if (fvm_utf8_validate(replacement, inserted, &count) != FVM_UTF8_OK)
        return FVM_INVALID_ARGUMENT;
    size_t total = bytes - (end - begin) + inserted;
    if (total > FVM_TEXT_FIELD_MAX_BYTES)
        return FVM_UNSUPPORTED;
    if (begin == end && !inserted)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    try
    {
        fvm_detail::vector<char> joined;
        joined.resize(total + 1);
        std::memcpy(joined.begin(), field->text, begin);
        std::memcpy(joined.begin() + begin, replacement, inserted);
        std::memcpy(joined.begin() + begin + inserted, field->text + end, bytes - end);
        joined[total] = 0;
        if (fvm_utf8_validate(joined.begin(), total, &count) != FVM_UTF8_OK)
            return FVM_INVALID_ARGUMENT;
        if (count > FVM_TEXT_FIELD_MAX_SCALARS)
            return FVM_UNSUPPORTED;
        /* Preserve the uniform current format. In an empty field the first
         * inserted text uses the future format. Styled spans are not simulated. */
        Style next = length ? field->current : field->future;
        PreparedGeometry geometry(*field);
        result = prepare_field_geometry(c, next, joined.begin(), total, geometry);
        if (result != FVM_OK)
            return result;
        GeometryNotification notification(c, self);
        result = notification.prepare(geometry, next, joined.begin(), total);
        if (result != FVM_OK)
            return result;
        char *owned = new (std::nothrow) char[total + 1];
        if (!owned)
            return FVM_OUT_OF_MEMORY;
        std::memcpy(owned, joined.begin(), total + 1);
        try
        {
            /* [GC:] Register owned bytes and their counted length together; failed insertion leaves
             * caller cleanup active. */
            fvm_context_adopt_string(c, owned, total);
        }
        catch (...)
        {
            delete[] owned;
            return FVM_OUT_OF_MEMORY;
        }
        /* All validation/allocation completes before changing the field. Old
         * context-owned text snapshots and aliased replacement strings survive. */
        field->text = owned;
        field->text_size = total;
        field->current.swap(next);
        fvm_value_undefined(out);
        return notification.commit(geometry);
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Compute and return field method. */
static FVM_Result field_method(FVM_Context *c, FVM_Object *self, unsigned int n,
                               const FVM_Value *args, FVM_Value *out, void *userdata)
{
    if (!self || self->resource_kind != TEXT_FIELD || !self->resource)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    Field *field = (Field *)self->resource;
    const char *name = (const char *)userdata;
    bool future =
        std::strcmp(name, "getNewTextFormat") == 0 || std::strcmp(name, "setNewTextFormat") == 0;
    if (name[0] == 'g')
    {
        if (n)
        {
            if (future || n > 2)
                return FVM_UNSUPPORTED;
            size_t begin, end, length;
            FVM_Result result = text_range(c, field->text, field->text_size, n, args, begin, end, length);
            if (result != FVM_OK)
                return result;
            if (begin == end)
                return FVM_INVALID_ARGUMENT;
        }
        return style_format(c, future ? field->future : field->current, out);
    }
    if (n != 1 || args[0].type != FVM_VALUE_OBJECT)
        return FVM_UNSUPPORTED;

    /* Initialization / Инициализация локального состояния. */
    Style next = future ? field->future : field->current;
    FVM_Result result = merge_style(c, args[0].object, next);
    if (result != FVM_OK)
        return result;
    FontOwner font;
    result = open_font(c, next, &font.font);
    if (result != FVM_OK)
        return result;
    FVM_FontMetrics metrics;
    result = fvm_font_metrics(font.font, &metrics);
    if (result != FVM_OK)
        return result;
    if (!finite_range(metrics.ascent + metrics.descent + next.leading, 1.0 / 64, 1000000))
        return FVM_INVALID_ARGUMENT;
    if (next.underline)
    {
        FVM_FontUnderlineMetrics underline;
        result = fvm_font_underline_metrics(font.font, &underline);
        if (result != FVM_OK)
            return result;
    }
    PreparedGeometry geometry(*field);
    if (!future)
    {
        result = prepare_field_geometry(c, next, field->text, field->text_size, geometry);
        if (result != FVM_OK)
            return result;
    }
    if (future)
        field->future.swap(next);
    else
    {
        GeometryNotification notification(c, self);
        result = notification.prepare(geometry, next, field->text, field->text_size);
        if (result != FVM_OK)
            return result;
        field->current.swap(next);
        fvm_value_undefined(out);
        return notification.commit(geometry);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}

/* Compute and return unsupported. */
static FVM_Result unsupported(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                              FVM_Value *, void *)
{
    return FVM_UNSUPPORTED;
}

/* Read field. */
static FVM_Result field_get(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *,
                            FVM_Value *out, void *userdata)
{
    if (self->resource_kind != TEXT_FIELD || !self->resource)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    Field *field = (Field *)self->resource;
    const char *name = (const char *)userdata;
    if (!std::strcmp(name, "scroll") || !std::strcmp(name, "hscroll") ||
        !std::strcmp(name, "maxscroll") || !std::strcmp(name, "maxhscroll") ||
        !std::strcmp(name, "bottomScroll"))
    {
        FVM_TextFieldScrollInfo info;
        FVM_Result result = scroll_geometry(c, field, info);
        if (result != FVM_OK)
            return result;
        field->scroll = info.scroll;
        field->hscroll = info.hscroll;
        fvm_value_number(out, !std::strcmp(name, "scroll") ? info.scroll :
            !std::strcmp(name, "hscroll") ? info.hscroll :
            !std::strcmp(name, "maxscroll") ? info.maxscroll :
            !std::strcmp(name, "maxhscroll") ? info.maxhscroll : info.bottom_scroll);
        return FVM_OK;
    }
    if (std::strcmp(name, "autoSize") == 0)
        return fvm_value_string(out, !field->auto_size_left ? "none" : field->anchor == FVM_TEXT_ANCHOR_RIGHT ? "right" :
            field->anchor == FVM_TEXT_ANCHOR_CENTER ? "center" : "left");
    if (std::strcmp(name, "_x") == 0 || std::strcmp(name, "_y") == 0)
    {
        fvm_value_number(out, std::strcmp(name, "_x") == 0 ? field->x : field->y);
        return FVM_OK;
    }
    if (std::strcmp(name, "text") == 0)
        return fvm_value_string_bytes(out, field->text, field->text_size);
    if (std::strcmp(name, "multiline") == 0)
        fvm_value_boolean(out, field->multiline);
    else if (std::strcmp(name, "wordWrap") == 0)
        fvm_value_boolean(out, field->wrap);
    else if (std::strcmp(name, "_width") == 0)
        fvm_value_number(out, field->width);
    else if (std::strcmp(name, "_height") == 0)
        fvm_value_number(out, field->height);
    else if (std::strcmp(name, "textColor") == 0)
        fvm_value_number(out, field->current.color);
    else if (std::strcmp(name, "length") == 0)
    {
        size_t offset = 0, size = field->text_size, units = 0;
        while (offset < size)
        {
            unsigned long cp;
            if (fvm_utf8_read(field->text, size, &offset, &cp) != FVM_UTF8_OK)
                return FVM_ERROR;
            units += cp > 0xffff ? 2 : 1;
        }
        fvm_value_number(out, (double)units);
    }
    else
    {
        FVM_Result result = ensure_layout(c, field);
        if (result != FVM_OK)
            return result;
        FVM_TextLayoutMetrics metrics;
        result = fvm_text_layout_metrics(field->layout, &metrics);
        if (result != FVM_OK)
            return result;
        fvm_value_number(out, std::strcmp(name, "textWidth") == 0 ? metrics.advance_width
                                                                  : metrics.height);
    }
    return FVM_OK;
}

/* Update field. */
static FVM_Result field_set(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args,
                            FVM_Value *out, void *userdata)
{
    if (self->resource_kind != TEXT_FIELD || !self->resource || n != 1)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    Field *field = (Field *)self->resource;
    const char *name = (const char *)userdata;
    if (!std::strcmp(name, "scroll") || !std::strcmp(name, "hscroll"))
    {
        double requested;
        FVM_Result result = number(c, args[0], requested);
        if (result != FVM_OK)
            return result;
        /* Conversion may execute user code; inspect current geometry only after it returns. */
        FVM_TextFieldScrollInfo info;
        result = scroll_geometry(c, field, info);
        if (result != FVM_OK)
            return result;
        bool vertical = !std::strcmp(name, "scroll");
        double minimum = vertical ? 1 : 0;
        double maximum = vertical ? info.maxscroll : info.maxhscroll;
        requested = requested < minimum ? minimum : requested > maximum ? maximum : std::floor(requested);
        unsigned long next_scroll = vertical ? (unsigned long)requested : info.scroll;
        unsigned long next_hscroll = vertical ? info.hscroll : (unsigned long)requested;
        bool changed = next_scroll != info.scroll || next_hscroll != info.hscroll;
        if (!changed)
        {
            field->scroll = next_scroll;
            field->hscroll = next_hscroll;
            fvm_value_undefined(out);
            return FVM_OK;
        }
        FVM_TextFieldScrollPlan notification(c, self);
        result = notification.prepare();
        if (result != FVM_OK)
            return result;
        field->scroll = next_scroll;
        field->hscroll = next_hscroll;
        fvm_value_undefined(out);
        return notification.dispatch();
    }
    if (std::strcmp(name, "text") == 0)
    {
        if (args[0].type != FVM_VALUE_STRING)
            return FVM_UNSUPPORTED;
        size_t size = fvm_string_size(args[0]), count = 0;
        if (size > FVM_TEXT_FIELD_MAX_BYTES || fvm_utf8_validate(args[0].string, size, &count) != FVM_UTF8_OK)
            return FVM_INVALID_ARGUMENT;
        if (count > FVM_TEXT_FIELD_MAX_SCALARS)
            return FVM_UNSUPPORTED;
        /* Loaded assignment of identical content preserves both current formatting and layout.
         * Compare counted bytes so a different suffix after NUL still performs a real edit. */
        if (c->swf_loaded && size == field->text_size &&
            !std::memcmp(args[0].string, field->text, size))
        {
            fvm_value_undefined(out);
            return FVM_OK;
        }
        Style next = field->future;
        PreparedGeometry geometry(*field);
        FVM_Result result = prepare_field_geometry(c, next, args[0].string, size, geometry);
        if (result != FVM_OK)
            return result;
        GeometryNotification notification(c, self);
        result = notification.prepare(geometry, next, args[0].string, size);
        if (result != FVM_OK)
            return result;
        field->current.swap(next);
        field->text = args[0].string;
        field->text_size = size;
        fvm_value_undefined(out);
        return notification.commit(geometry);
    }
    else if (std::strcmp(name, "multiline") == 0 || std::strcmp(name, "wordWrap") == 0)
    {
        if (args[0].type != FVM_VALUE_BOOLEAN)
            return FVM_UNSUPPORTED;
        PreparedGeometry geometry(*field);
        if (std::strcmp(name, "multiline") == 0)
            geometry.multiline = args[0].boolean != 0;
        else
            geometry.wrap = args[0].boolean != 0;
        FVM_Result result = prepare_field_geometry(c, field->current, field->text, field->text_size, geometry);
        if (result != FVM_OK)
            return result;
        GeometryNotification notification(c, self);
        FVM_Result notify_result = notification.prepare(geometry);
        if (notify_result != FVM_OK)
            return notify_result;
        fvm_value_undefined(out);
        return notification.commit(geometry);
    }
    else if (std::strcmp(name, "autoSize") == 0)
    {
        bool automatic;
        if (args[0].type == FVM_VALUE_BOOLEAN)
            automatic = args[0].boolean != 0;
        else if (style_keyword(args[0], "left") || style_keyword(args[0], "center") ||
                 style_keyword(args[0], "right"))
            automatic = true;
        else if (style_keyword(args[0], "none"))
            automatic = false;
        else
            return FVM_UNSUPPORTED;
        PreparedGeometry geometry(*field);
        geometry.automatic = automatic;
        geometry.anchor = style_keyword(args[0], "right") ? FVM_TEXT_ANCHOR_RIGHT :
                          style_keyword(args[0], "center") ? FVM_TEXT_ANCHOR_CENTER : FVM_TEXT_ANCHOR_LEFT;
        if (automatic)
        {
            FVM_Result result = prepare_field_geometry(c, field->current, field->text, field->text_size, geometry);
            if (result != FVM_OK)
                return result;
        }
        GeometryNotification notification(c, self);
        FVM_Result notify_result = notification.prepare(geometry);
        if (notify_result != FVM_OK)
            return notify_result;
        fvm_value_undefined(out);
        return notification.commit(geometry);
    }
    else
    {
        double d;
        FVM_Result result = number(c, args[0], d);
        if (result != FVM_OK)
            return result;
        if (std::strcmp(name, "_x") == 0 || std::strcmp(name, "_y") == 0)
        {
            if (!finite_range(d, -FVM_TEXT_FIELD_MAX_POSITION, FVM_TEXT_FIELD_MAX_POSITION))
                return FVM_INVALID_ARGUMENT;
            if (std::strcmp(name, "_x") == 0) field->x = d;
            else field->y = d;
        }
        else if (std::strcmp(name, "textColor") == 0)
        {
            if (!finite_range(d, 0, 16777215) || d != std::floor(d))
                return FVM_INVALID_ARGUMENT;
            field->current.color = field->future.color = (unsigned long)d;
        }
        else
        {
            if (!finite_range(d, 0, FVM_TEXT_FIELD_MAX_EXTENT))
                return FVM_INVALID_ARGUMENT;
            PreparedGeometry geometry(*field);
            if (field->auto_size_left && (std::strcmp(name, "_width") != 0 || !field->wrap))
                return FVM_UNSUPPORTED;
            if (std::strcmp(name, "_width") == 0)
                geometry.width = d;
            else
                geometry.height = d;
            result = prepare_field_geometry(c, field->current, field->text, field->text_size, geometry);
            if (result != FVM_OK)
                return result;
            GeometryNotification notification(c, self);
            result = notification.prepare(geometry);
            if (result != FVM_OK)
                return result;
            fvm_value_undefined(out);
            return notification.commit(geometry);
        }
    }
    fvm_value_undefined(out);
    return FVM_OK;
}
} // namespace
/* Map a native text callback to its stable serialization identifier. */
unsigned long fvm_text_callback_id(FVM_NativeFunction callback)
{
    if (callback == fvm_textfield_add_listener) return FVM_TEXT_LISTENER_CALLBACK_ADD;
    if (callback == fvm_textfield_remove_listener) return FVM_TEXT_LISTENER_CALLBACK_REMOVE;
    if (callback == format_constructor)
        return 206;
    if (callback == get_extent)
        return 200;
    if (callback == field_method)
        return 201;
    if (callback == field_get)
        return 202;
    if (callback == field_set)
        return 203;
    if (callback == replace_text)
        return 204;
    if (callback == unsupported)
        return 205;
    return 0;
}

/* Restore a native text callback from its stable serialization identifier. */
FVM_NativeFunction fvm_text_callback_from_id(unsigned long id)
{
    if (id == FVM_TEXT_LISTENER_CALLBACK_ADD) return fvm_textfield_add_listener;
    if (id == FVM_TEXT_LISTENER_CALLBACK_REMOVE) return fvm_textfield_remove_listener;
    if (id == 206)
        return format_constructor;
    if (id == 200)
        return get_extent;
    if (id == 201)
        return field_method;
    if (id == 202)
        return field_get;
    if (id == 203)
        return field_set;
    if (id == 204)
        return replace_text;
    if (id == 205)
        return unsupported;
    return 0;
}

/* Compute and return standard text install. */
extern "C" FVM_Result fvm_standard_text_install(FVM_Context *c)
{
    /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
    if (!c || c->destroying || c->gc_collecting || c->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    if (c->standard_text_library)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value format, field;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            c->constructors.find("TextFormat");
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (fvm_get_global(c, "TextFormat", &format) != FVM_OK || format.type != FVM_VALUE_OBJECT ||
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            format.object->resource_kind != TEXT_FORMAT_CLASS ||
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_get_global(c, "TextField", &field) != FVM_OK || field.type != FVM_VALUE_OBJECT ||
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            field.object->resource_kind != TEXT_FIELD_CLASS || ctor == c->constructors.end() ||
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            ctor->second.callback != format_constructor || ctor->second.root != format.object)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    const char *names[] = {"TextFormat", "TextField"};
    for (size_t i = 0; i < 2; ++i)
        if (c->global->properties.find(names[i]) != c->global->properties.end() ||
            c->global->accessors.find(names[i]) != c->global->accessors.end() ||
            c->constructors.find(names[i]) != c->constructors.end() ||
            c->functions.find(names[i]) != c->functions.end() ||
            c->script_functions.find(names[i]) != c->script_functions.end() ||
            c->stack_functions.find(names[i]) != c->stack_functions.end())
            return FVM_INVALID_ARGUMENT;
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, FVM_Value> globals = c->global->properties;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry> constructors = c->constructors;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *classes[2], *prototypes[2];
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (size_t i = 0; i < 2; ++i)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Result result = fvm_object_create(c, &classes[i]);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (result != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return result;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            result = fvm_object_create(c, &prototypes[i]);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (result != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return result;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            classes[i]->resource_kind = i == 0 ? TEXT_FORMAT_CLASS : TEXT_FIELD_CLASS;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value v;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            object_value(&v, prototypes[i]);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            classes[i]->properties["prototype"] = v;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            classes[i]->attributes["prototype"] = 3;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            object_value(&v, classes[i]);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            prototypes[i]->properties["constructor"] = v;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            prototypes[i]->attributes["constructor"] = 1;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            globals[names[i]] = v;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeEntry ctor;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        ctor.callback = format_constructor;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        ctor.userdata = classes[0];
        /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
        ctor.root = classes[0];
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        constructors["TextFormat"] = ctor;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        classes[0]->constructable = ctor;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        classes[0]->invocable = ctor;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeEntry field_constructor;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        field_constructor.callback = unsupported;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        classes[1]->invocable = field_constructor;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        classes[1]->constructable = field_constructor;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (c->standard_library)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value function, object;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Result result = fvm_get_global(c, "Function", &function);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (result == FVM_OK)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                result = fvm_get_global(c, "Object", &object);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (result != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return result;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (function.type != FVM_VALUE_OBJECT || object.type != FVM_VALUE_OBJECT)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_INVALID_ARGUMENT;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value function_proto, object_proto;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            result = fvm_object_get(c, function.object, "prototype", &function_proto);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (result == FVM_OK)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                result = fvm_object_get(c, object.object, "prototype", &object_proto);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (result != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return result;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (function_proto.type != FVM_VALUE_OBJECT || object_proto.type != FVM_VALUE_OBJECT)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_INVALID_ARGUMENT;
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (size_t i = 0; i < 2; ++i)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                classes[i]->prototype = function_proto.object;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                prototypes[i]->prototype = object_proto.object;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                classes[i]->attributes["prototype"] = 3;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                prototypes[i]->attributes["constructor"] = 1;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->global->properties.swap(globals);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->constructors.swap(constructors);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->standard_text_library = true;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Register an independently owned face in one family/style slot, transactionally. */
extern "C" FVM_Result fvm_standard_text_register_font_variant(
    FVM_Context *c, const char *name, unsigned int variant, const void *bytes, size_t size, long index)
{
    /* [GC:] Context-owned sources cannot change during teardown or collection. */
    if (!c || !name || !*name || std::strlen(name) > FVM_TEXT_FONT_MAX_NAME ||
        (variant & ~FVM_TEXT_FONT_VARIANTS) || c->destroying || c->gc_collecting || c->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    FVM_FontSource *source = 0;
    try
    {
        fvm_detail::string key = font_source_key(name, std::strlen(name), variant);
        if (c->fonts.find(key) != c->fonts.end())
            return FVM_INVALID_ARGUMENT;
        if (size > FVM_TEXT_FONT_MAX_BYTES || c->fonts.size() >= FVM_TEXT_FONT_MAX_FACES)
            return FVM_UNSUPPORTED;
        size_t total = size;
        for (fvm_detail::map<fvm_detail::string, FVM_FontSource *>::iterator i = c->fonts.begin();
             i != c->fonts.end(); ++i)
        {
            if (total > FVM_TEXT_FONT_MAX_BYTES - i->second->bytes.size())
                return FVM_UNSUPPORTED;
            total += i->second->bytes.size();
        }
        FontOwner font;
        FVM_Result result = fvm_font_read(bytes, size, index, 12, &font.font);
        if (result != FVM_OK)
            return result;
        source = new FVM_FontSource;
        source->index = index;
        source->bytes.resize(size);
        std::memcpy(source->bytes.begin(), bytes, size);
        fvm_detail::string default_name = c->default_font;
        if (!variant && default_name.empty())
            default_name = name;
        /* All fallible preparation precedes publication; map owns the face afterwards. */
        c->fonts[key] = source;
        source = 0;
        c->default_font.swap(default_name);
        return FVM_OK;
    }
    catch (...)
    {
        delete source;
        return FVM_OUT_OF_MEMORY;
    }
}

/* Preserve the original API as registration of the regular family slot. */
extern "C" FVM_Result fvm_standard_text_register_font(
    FVM_Context *c, const char *name, const void *bytes, size_t size, long index)
{
    return fvm_standard_text_register_font_variant(c, name, FVM_TEXT_FONT_REGULAR, bytes, size, index);
}

/* Create textfield. */
extern "C" FVM_Result fvm_textfield_create(FVM_Context *c, const char *font_name, double width,
                                           double height, FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
    if (!c || !c->standard_text_library || c->destroying || c->gc_releasing ||
        !finite_range(width, 0, FVM_TEXT_FIELD_MAX_EXTENT) || !finite_range(height, 0, FVM_TEXT_FIELD_MAX_EXTENT))
        return FVM_INVALID_ARGUMENT;
    Field *field = 0;
    try
    {
        field = new Field;
        field->current.font = font_name ? font_name : c->default_font.c_str();
        field->current.source_key = field->current.font;
        field->future = field->current;
        field->width = width;
        field->height = height;
        FontOwner font;
        FVM_Result result = open_font(c, field->current, &font.font);
        if (result != FVM_OK)
        {
            delete field;
            return result;
        }
        FVM_Object *object = 0;
        result = fvm_object_create(c, &object);
        if (result != FVM_OK)
        {
            delete field;
            return result;
        }
        object->resource_kind = TEXT_FIELD;
        object->resource = field;
        object->release_resource = release_field;
        field = 0; /* The GC-owned object now owns all native resources. */
        FVM_Value klass, prototype;
        result = fvm_get_global(c, "TextField", &klass);
        if (result != FVM_OK || klass.type != FVM_VALUE_OBJECT ||
            klass.object->resource_kind != TEXT_FIELD_CLASS)
            return FVM_INVALID_ARGUMENT;
        result = fvm_object_get(c, klass.object, "prototype", &prototype);
        if (result != FVM_OK || prototype.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        object->prototype = prototype.object;
        const char *names[] = {"text",      "multiline", "wordWrap",  "_width",    "_height",
                               "textColor", "autoSize", "_x", "_y", "length",    "textWidth", "textHeight"};
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        {
            result = fvm_object_define_accessor_rooted(c, object, names[i], field_get,
                                                       i < FVM_TEXT_FIELD_WRITABLE_PROPERTIES ? field_set : 0, (void *)names[i], 0);
            if (result != FVM_OK)
                return result;
        }
        const char *methods[] = {"getTextFormat", "getNewTextFormat", "setTextFormat",
                                 "setNewTextFormat"};
        for (size_t i = 0; i < 4; ++i)
        {
            FVM_NativeEntry entry;
            entry.callback = field_method;
            entry.userdata = (void *)methods[i];
            object->methods[methods[i]] = entry;
        }
        FVM_NativeEntry replace;
        replace.callback = replace_text;
        object->methods["replaceText"] = replace;
        const char *scroll_names[] = {"scroll", "hscroll", "maxscroll", "maxhscroll", "bottomScroll"};
        for (size_t i = 0; i < sizeof(scroll_names) / sizeof(scroll_names[0]); ++i)
        {
            result = fvm_object_define_accessor_rooted(c, object, scroll_names[i], field_get,
                i < FVM_TEXT_FIELD_SCROLL_WRITABLE_COUNT ? field_set : 0, (void *)scroll_names[i], 0);
            if (result != FVM_OK)
                return result;
        }
        const char *unimplemented[] = {
            "html",       "htmlText",     "type",
            "border",     "borderColor",  "background",    "backgroundColor",
            "embedFonts", "password",     "selectable",    "maxChars",
            "restrict", "antiAliasType", "gridFitType",
            "sharpness",  "thickness",    "condenseWhite", "filters",
            "_alpha",        "_visible"};
        for (size_t i = 0; i < sizeof(unimplemented) / sizeof(unimplemented[0]); ++i)
        {
            result = fvm_object_define_accessor_rooted(c, object, unimplemented[i], unsupported,
                                                       unsupported, 0, 0);
            if (result != FVM_OK)
                return result;
        }
        FVM_NativeEntry listener;
        listener.callback = fvm_textfield_add_listener;
        object->methods["addListener"] = listener;
        listener.callback = fvm_textfield_remove_listener;
        object->methods["removeListener"] = listener;
        const char *unimplemented_methods[] = {"replaceSel", "removeTextField", "getDepth"};
        for (size_t i = 0; i < sizeof(unimplemented_methods) / sizeof(unimplemented_methods[0]);
             ++i)
        {
            FVM_NativeEntry entry;
            entry.callback = unsupported;
            object->methods[unimplemented_methods[i]] = entry;
        }
        *out = object;
        return FVM_OK;
    }
    catch (...)
    {
        delete field;
        return FVM_OUT_OF_MEMORY;
    }
}

/* [GC:] Owner-only field query retains no managed references after returning copied scalars. */
extern "C" FVM_Result fvm_textfield_scroll_info(
    FVM_Context *context, FVM_Object *object, FVM_TextFieldScrollInfo *out)
{
    if (!context || !object || object->owner != context || object->resource_kind != TEXT_FIELD ||
        !object->resource || !out || out->struct_size != sizeof(*out) ||
        context->destroying || context->gc_collecting || context->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    try
    {
        Field *field = (Field *)object->resource;
        FVM_TextFieldScrollInfo info;
        FVM_Result result = scroll_geometry(context, field, info);
        if (result != FVM_OK)
            return result;
        field->scroll = info.scroll;
        field->hscroll = info.hscroll;
        *out = info;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Unwrapped native layout has unlimited width; apply the field's gutter-relative
 * alignment once to both glyph outlines and underline bars. */
static double field_line_shift(const Field *field, const FVM_TextLayoutLine &line)
{
    double left = field->current.left_margin + field->current.block_indent;
    if (field->wrap && field->multiline)
        return left;
    double available = field->width - FVM_TEXT_FIELD_GUTTER - left -
                       field->current.right_margin - line.advance - line.x;
    if (available <= 0)
        return left;
    if (field->current.alignment == FVM_TEXT_ALIGN_CENTER)
        return left + available / 2;
    return left + (field->current.alignment == FVM_TEXT_ALIGN_RIGHT ? available : 0);
}

/* Compute and return textfield path. */
extern "C" FVM_Result fvm_textfield_path(FVM_Context *c, FVM_Object *object, FVM_Path **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || !object || object->owner != c || object->resource_kind != TEXT_FIELD ||
        !object->resource)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    Field *field = (Field *)object->resource;
    FVM_Result result = ensure_layout(c, field);
    if (result != FVM_OK)
        return result;

    /* Initialization / Инициализация локального состояния. */
    FVM_TextFieldScrollInfo viewport;
    result = scroll_geometry(c, field, viewport);
    if (result != FVM_OK)
        return result;
    FVM_Path *path = 0;
    result = fvm_path_create(&path);
    if (result != FVM_OK)
        return result;
    FVM_TextLayoutMetrics metrics;
    result = fvm_text_layout_metrics(field->layout, &metrics);
    for (size_t i = 0; result == FVM_OK && i < metrics.glyph_count; ++i)
    {
        FVM_TextLayoutGlyph glyph;
        FVM_TextLayoutLine line;
        FVM_Path *copy = 0;
        result = fvm_text_layout_glyph(field->layout, i, &glyph);
        if (result == FVM_OK)
            result = fvm_text_layout_line(field->layout, glyph.line, &line);
        if (result == FVM_OK)
            result = fvm_text_layout_glyph_path(field->layout, i, &copy);
        double shift = result == FVM_OK ? field_line_shift(field, line) : 0;
        FVM_Affine place = {1, 0, 0, 1, 2 + shift + viewport.offset_x, 2 + viewport.offset_y};
        if (result == FVM_OK)
            result = fvm_path_append(path, copy, &place);
        fvm_path_destroy(copy);
    }
    if (result == FVM_OK && field->current.underline)
    {
        FontOwner font;
        FVM_FontUnderlineMetrics underline;
        result = open_font(c, field->current, &font.font);
        if (result == FVM_OK)
            result = fvm_font_underline_metrics(font.font, &underline);
        for (size_t i = 0; result == FVM_OK && i < metrics.line_count; ++i)
        {
            FVM_TextLayoutLine line;
            FVM_Path *bar = 0;
            result = fvm_text_layout_line(field->layout, i, &line);
            if (result == FVM_OK)
                result = fvm_text_layout_line_underline_path(field->layout, i,
                                      underline.center_y, underline.thickness, &bar);
            double shift = result == FVM_OK ? field_line_shift(field, line) : 0;
            FVM_Affine place = {1, 0, 0, 1, 2 + shift + viewport.offset_x, 2 + viewport.offset_y};
            if (result == FVM_OK)
                result = fvm_path_append(path, bar, &place);
            fvm_path_destroy(bar);
        }
    }
    if (result != FVM_OK)
    {
        fvm_path_destroy(path);
        return result;
    }
    field->scroll = viewport.scroll;
    field->hscroll = viewport.hscroll;
    *out = path;
    return FVM_OK;
}

namespace
{
const char *field_selectors[] = {
    "getTextFormat", "getNewTextFormat", "setTextFormat", "setNewTextFormat", "text",
    "multiline",     "wordWrap",         "_width",        "_height",          "textColor",
    "autoSize", "_x", "_y", "length",        "textWidth",        "textHeight",
    "scroll", "hscroll", "maxscroll", "maxhscroll", "bottomScroll"};

/* Capture snapshot of style. */
void snapshot_style(FVM_Context *context, const Style &style, FVM_TextStyleState &out)
{

    /* Computation / Вычисление и обновление состояния. */
    out.font = style.font;
    out.size = style.size;
    out.leading = style.leading;
    out.letter_spacing = style.letter_spacing;
    out.tab_stops = style.tab_stops;
    out.indent = style.indent;
    out.left_margin = style.left_margin; out.right_margin = style.right_margin; out.block_indent = style.block_indent;
    out.color = style.color;
    out.alignment = (style.alignment == FVM_TEXT_ALIGN_JUSTIFY ? 0UL : (unsigned long)style.alignment) |
                    (style.alignment == FVM_TEXT_ALIGN_JUSTIFY ? FVM_TEXT_STYLE_JUSTIFY : 0UL) |
                    (style.kerning ? 0UL : FVM_TEXT_STYLE_NO_KERNING) |
                    (style.letter_spacing != 0 ? FVM_TEXT_STYLE_HAS_SPACING : 0UL) |
                    (style.left_margin || style.right_margin || style.block_indent ? FVM_TEXT_STYLE_HAS_INSETS : 0UL) |
                    (style.indent != 0 ? FVM_TEXT_STYLE_HAS_INDENT : 0UL) |
                    (!style.tab_stops.empty() ? FVM_TEXT_STYLE_HAS_TABS : 0UL) |
                    (style.underline ? FVM_TEXT_STYLE_UNDERLINE : 0UL) |
                    (style.variant & FVM_TEXT_FONT_BOLD ? FVM_TEXT_STYLE_BOLD : 0UL) |
                    (style.variant & FVM_TEXT_FONT_ITALIC ? FVM_TEXT_STYLE_ITALIC : 0UL);
    out.source = *context->fonts.find(style.source_key)->second;
}

/* Restore style. */
FVM_Result restore_style(FVM_Context *context, const FVM_TextStyleState &src, Style &out)
{
    if (src.tab_stops.size() > FVM_TEXT_STYLE_MAX_TABS ||
        (!src.tab_stops.empty() != ((src.alignment & FVM_TEXT_STYLE_HAS_TABS) != 0)))
        return FVM_INVALID_ARGUMENT;
    for (size_t i = 0; i < src.tab_stops.size(); ++i)
        if (!finite_range(src.tab_stops[i], 0, FVM_TEXT_STYLE_MAX_TAB_STOP) ||
            src.tab_stops[i] != std::floor(src.tab_stops[i]) ||
            (i && src.tab_stops[i] <= src.tab_stops[i - 1])) return FVM_INVALID_ARGUMENT;
    if (!finite_range(src.indent, -FVM_TEXT_STYLE_MAX_INDENT, FVM_TEXT_STYLE_MAX_INDENT) ||
        src.indent != std::floor(src.indent) ||
        ((src.indent != 0) != ((src.alignment & FVM_TEXT_STYLE_HAS_INDENT) != 0)))
        return FVM_INVALID_ARGUMENT;
    if (src.indent < 0 &&
        ((context->swf_loaded && context->swf_info.version < FVM_TEXT_INDENT_NEGATIVE_MIN_VERSION) ||
         src.left_margin + src.block_indent + src.indent < 0))
        return FVM_UNSUPPORTED;
    if (!finite_range(src.left_margin, 0, FVM_TEXT_STYLE_MAX_INSET) || src.left_margin != std::floor(src.left_margin) ||
        !finite_range(src.right_margin, 0, FVM_TEXT_STYLE_MAX_INSET) || src.right_margin != std::floor(src.right_margin) ||
        !finite_range(src.block_indent, 0, FVM_TEXT_STYLE_MAX_INSET) || src.block_indent != std::floor(src.block_indent) ||
        ((src.left_margin != 0 || src.right_margin != 0 || src.block_indent != 0) !=
         ((src.alignment & FVM_TEXT_STYLE_HAS_INSETS) != 0)))
        return FVM_INVALID_ARGUMENT;
    if (src.font.empty() || src.font.size() > FVM_TEXT_FONT_MAX_NAME ||
        std::memchr(src.font.c_str(), 0, src.font.size()) || !finite_range(src.size, 1, 4096) ||
        !finite_range(src.leading, -4096, 4096) ||
        !finite_range(src.letter_spacing, -FVM_TEXT_STYLE_MAX_SPACING, FVM_TEXT_STYLE_MAX_SPACING) ||
        ((src.letter_spacing != 0) != ((src.alignment & FVM_TEXT_STYLE_HAS_SPACING) != 0)) || src.color > 16777215UL ||
        (src.alignment & ~FVM_TEXT_STYLE_FLAGS) > 2 ||
        ((src.alignment & FVM_TEXT_STYLE_JUSTIFY) && (src.alignment & ~FVM_TEXT_STYLE_FLAGS)))
        return FVM_INVALID_ARGUMENT;
    if ((src.alignment & FVM_TEXT_STYLE_JUSTIFY) && !justify_available(context))
        return FVM_UNSUPPORTED;
    unsigned int variant = (src.alignment & FVM_TEXT_STYLE_BOLD ? FVM_TEXT_FONT_BOLD : 0U) |
                           (src.alignment & FVM_TEXT_STYLE_ITALIC ? FVM_TEXT_FONT_ITALIC : 0U);
    fvm_detail::string key = font_source_key(src.font.c_str(), src.font.size(), variant);
    fvm_detail::map<fvm_detail::string, FVM_FontSource *>::iterator existing = context->fonts.find(key);
    if (existing != context->fonts.end())
    {
        FVM_FontSource *face = existing->second;
        if (face->index != src.source.index || face->bytes.size() != src.source.bytes.size() ||
            (face->bytes.size() &&
             std::memcmp(face->bytes.begin(), src.source.bytes.begin(), face->bytes.size())))
            return FVM_INVALID_ARGUMENT;
    }
    else
    {
        FVM_Result result =
            fvm_standard_text_register_font_variant(context, src.font.c_str(), variant, src.source.bytes.begin(),
                                            src.source.bytes.size(), src.source.index);
        if (result != FVM_OK)
            return result;
    }

    /* Computation / Вычисление и обновление состояния. */
    out.font = src.font;
    out.source_key = key;
    out.variant = variant;
    out.size = src.size;
    out.leading = src.leading;
    out.letter_spacing = src.letter_spacing;
    out.tab_stops = src.tab_stops;
    out.indent = src.indent;
    out.left_margin = src.left_margin; out.right_margin = src.right_margin; out.block_indent = src.block_indent;
    out.color = src.color;
    out.alignment = src.alignment & FVM_TEXT_STYLE_JUSTIFY ? FVM_TEXT_ALIGN_JUSTIFY :
                    (FVM_TextAlignment)(src.alignment & ~FVM_TEXT_STYLE_FLAGS);
    out.underline = (src.alignment & FVM_TEXT_STYLE_UNDERLINE) != 0;
    out.kerning = (src.alignment & FVM_TEXT_STYLE_NO_KERNING) == 0;
    return FVM_OK;
}
} // namespace
/* Compute and return text callback selector. */
const char *fvm_text_callback_selector(FVM_NativeFunction callback, void *userdata)
{
    unsigned long id = fvm_text_callback_id(callback);
    if (id < 201 || id > 203 || !userdata)
        return 0;
    const char *name = (const char *)userdata;
    for (size_t i = 0; i < sizeof(field_selectors) / sizeof(field_selectors[0]); ++i)
        if (!std::strcmp(name, field_selectors[i]) && ((id == 201) == (i < FVM_TEXT_FIELD_METHOD_SELECTORS)) &&
            (id != 203 || i < FVM_TEXT_FIELD_WRITABLE_SELECTORS ||
             (i >= FVM_TEXT_FIELD_SCROLL_SELECTOR_FIRST &&
              i < FVM_TEXT_FIELD_SCROLL_SELECTOR_FIRST + FVM_TEXT_FIELD_SCROLL_WRITABLE_COUNT)))
            return field_selectors[i];
    return 0;
}

/* Compute and return text callback userdata. */
void *fvm_text_callback_userdata(unsigned long id, const char *name)
{
    return (void *)fvm_text_callback_selector(fvm_text_callback_from_id(id), (void *)name);
}

/* [GC:] Upgrade only the rootless retired built-in geometry stubs. Host callbacks,
 * custom userdata and captured roots retain their original graph semantics. */
void fvm_textfield_upgrade_accessor(const char *name, FVM_AccessorEntry &entry)
{
    const char *scroll_names[] = {"scroll", "hscroll", "maxscroll", "maxhscroll", "bottomScroll"};
    for (size_t i = 0; name && i < sizeof(scroll_names) / sizeof(scroll_names[0]); ++i)
    {
        if (!std::strcmp(name, scroll_names[i]) && entry.getter == unsupported &&
            entry.setter == unsupported && !entry.userdata && !entry.root)
        {
            entry.getter = field_get;
            entry.setter = i < FVM_TEXT_FIELD_SCROLL_WRITABLE_COUNT ? field_set : 0;
            entry.userdata = (void *)scroll_names[i];
            return;
        }
    }
    if (name && (!std::strcmp(name, "autoSize") || !std::strcmp(name, "_x") ||
                 !std::strcmp(name, "_y")) && entry.getter == unsupported &&
        entry.setter == unsupported && !entry.userdata && !entry.root)
    {
        entry.getter = field_get;
        entry.setter = field_set;
        entry.userdata = (void *)(!std::strcmp(name, "_x") ? "_x" :
                                 !std::strcmp(name, "_y") ? "_y" : "autoSize");
    }
}

/* [GC:] Retain native TextField text only when its owning managed object is reachable. */
const char *fvm_textfield_string_root(FVM_Object *object)
{
    /* [GC:] Other native kinds and released fields do not own context text buffers. */
    return object && object->resource_kind == TEXT_FIELD && object->resource
               ? ((Field *)object->resource)->text
               : 0;
}

/* Capture snapshot of textfield size. */
size_t fvm_textfield_snapshot_size(FVM_Object *object)
{
    Field *field = (Field *)object->resource;
    if (!field)
        return (size_t)-1;

    /* Initialization / Инициализация локального состояния. */
    FVM_Context *context = object->owner;
    fvm_detail::map<fvm_detail::string, FVM_FontSource *>::iterator a =
        context->fonts.find(field->current.source_key);
    fvm_detail::map<fvm_detail::string, FVM_FontSource *>::iterator b =
        context->fonts.find(field->future.source_key);
    if (a == context->fonts.end() || b == context->fonts.end())
        return (size_t)-1;
    return FVM_TEXT_FIELD_SNAPSHOT_OVERHEAD + FVM_TEXT_STYLE_TAB_WIRE_HEADER *
           ((field->current.tab_stops.empty() ? 0 : 1) + (field->future.tab_stops.empty() ? 0 : 1)) +
           FVM_TEXT_STYLE_TAB_WIRE_BYTES * (field->current.tab_stops.size() + field->future.tab_stops.size()) + field->text_size + field->current.font.size() + field->future.font.size() +
           a->second->bytes.size() + b->second->bytes.size();
}

/* Capture snapshot of textfield. */
FVM_Result fvm_textfield_snapshot(FVM_Object *object, FVM_TextFieldState &out)
{
    Field *field = (Field *)object->resource;
    if (!field || fvm_textfield_snapshot_size(object) == (size_t)-1)
        return FVM_UNSUPPORTED;

    FVM_TextFieldScrollInfo info;
    info.scroll = field->scroll;
    info.hscroll = field->hscroll;
    if (field->scroll != 1 || field->hscroll != 0)
    {
        FVM_Result result = scroll_geometry(object->owner, field, info);
        if (result != FVM_OK)
            return result;
    }
    /* Computation / Вычисление и обновление состояния. */
    snapshot_style(object->owner, field->current, out.current);
    snapshot_style(object->owner, field->future, out.future);
    out.text = fvm_detail::string(field->text, field->text_size);
    out.width = field->width;
    out.height = field->height;
    out.multiline = field->multiline;
    out.wrap = field->wrap;
    out.auto_size_left = field->auto_size_left;
    out.x = field->x; out.y = field->y;
    out.anchor = field->auto_size_left ? field->anchor : FVM_TEXT_ANCHOR_LEFT;
    out.scroll = info.scroll; out.hscroll = info.hscroll;
    return FVM_OK;
}

/* Restore textfield. */
FVM_Result fvm_textfield_restore(FVM_Context *context, FVM_Object *object,
                                 const FVM_TextFieldState &state)
{
    if (!context->standard_text_library)
        return FVM_UNSUPPORTED;
    size_t scalar_count = 0;
    /* Validate the same storage envelope as editing before allocating native state or fonts. */
    if (!finite_range(state.scroll, 1, FVM_TEXT_FIELD_MAX_SCROLL_LINES) ||
        state.scroll != std::floor(state.scroll) ||
        !finite_range(state.hscroll, 0, FVM_TEXT_FIELD_MAX_SCROLL_PIXELS) ||
        state.hscroll != std::floor(state.hscroll) ||
        !finite_range(state.width, 0, FVM_TEXT_FIELD_MAX_EXTENT) || !finite_range(state.height, 0, FVM_TEXT_FIELD_MAX_EXTENT) ||
        !finite_range(state.x, -FVM_TEXT_FIELD_MAX_POSITION, FVM_TEXT_FIELD_MAX_POSITION) ||
        !finite_range(state.y, -FVM_TEXT_FIELD_MAX_POSITION, FVM_TEXT_FIELD_MAX_POSITION) ||
        state.anchor > FVM_TEXT_ANCHOR_RIGHT || (!state.auto_size_left && state.anchor != FVM_TEXT_ANCHOR_LEFT) ||
        state.text.size() > FVM_TEXT_FIELD_MAX_BYTES ||
        fvm_utf8_validate(state.text.c_str(), state.text.size(), &scalar_count) != FVM_UTF8_OK ||
        scalar_count > FVM_TEXT_FIELD_MAX_SCALARS)
        return FVM_INVALID_ARGUMENT;
    Field *field = new Field;
    try
    {
        FVM_Result result = restore_style(context, state.current, field->current);
        if (result == FVM_OK)
            result = restore_style(context, state.future, field->future);
        if (result != FVM_OK)
        {
            delete field;
            return result;
        }
        /* Validate metrics as a normal field edit does, then discard layout caches. */
        Style *styles[] = {&field->current, &field->future};
        for (size_t i = 0; i < 2; ++i)
        {
            FontOwner font;
            result = open_font(context, *styles[i], &font.font);
            if (result != FVM_OK)
            {
                delete field;
                return result;
            }
            if (styles[i]->underline)
            {
                FVM_FontUnderlineMetrics underline;
                result = fvm_font_underline_metrics(font.font, &underline);
                if (result != FVM_OK)
                {
                    delete field;
                    return result;
                }
            }
            FVM_FontMetrics metrics;
            result = fvm_font_metrics(font.font, &metrics);
            if (result != FVM_OK ||
                !finite_range(metrics.ascent + metrics.descent + styles[i]->leading, 1.0 / 64,
                              1000000))
            {
                delete field;
                return result == FVM_OK ? FVM_INVALID_ARGUMENT : result;
            }
        }
        /* Restore uses the same tracked-current admission as ordinary edits,
         * before retaining text bytes or attaching any field to the receiver. */
        field->width = state.width;
        field->height = state.height;
        field->multiline = state.multiline;
        field->wrap = state.wrap;
        field->auto_size_left = state.auto_size_left;
        field->x = state.x; field->y = state.y; field->anchor = state.anchor;
        PreparedGeometry geometry(*field);
        /* Serialized dimensions are already anchored. Validate/rebuild the local
         * layout without moving the stored origin, including tolerated subpixels. */
        geometry.anchor = FVM_TEXT_ANCHOR_LEFT;
        result = prepare_field_geometry(context, field->current, state.text.c_str(), state.text.size(), geometry);
        if (result != FVM_OK)
        {
            delete field;
            return result;
        }
        if (state.auto_size_left &&
            (std::fabs(geometry.width - state.width) > FVM_TEXT_FIELD_GEOMETRY_TOLERANCE ||
             std::fabs(geometry.height - state.height) > FVM_TEXT_FIELD_GEOMETRY_TOLERANCE))
        {
            delete field;
            return FVM_INVALID_ARGUMENT;
        }
        field->scroll = (unsigned long)state.scroll;
        field->hscroll = (unsigned long)state.hscroll;
        if (field->scroll != 1 || field->hscroll != 0)
        {
            /* Borrow only the temporary state while checking geometry before publication. */
            field->text = state.text.c_str();
            field->text_size = state.text.size();
            FVM_TextFieldScrollInfo info;
            result = scroll_geometry(context, field, info);
            if (result != FVM_OK || info.scroll != field->scroll || info.hscroll != field->hscroll)
            {
                delete field;
                return result == FVM_OK ? FVM_INVALID_ARGUMENT : result;
            }
        }
        fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator legacy_auto =
            object->accessors.find("autoSize");
        geometry.anchor = state.anchor; /* Publish the original policy after measurement. */
        fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator legacy_x = object->accessors.find("_x");
        fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator legacy_y = object->accessors.find("_y");
        char *text = new char[state.text.size() + 1];
        std::memcpy(text, state.text.c_str(), state.text.size() + 1);
        try
        {
            /* [GC:] Register owned bytes and their counted length together; failed insertion leaves
             * caller cleanup active. */
            fvm_context_adopt_string(context, text, state.text.size());
        }
        catch (...)
        {
            delete[] text;
            throw;
        }
        field->text = text;
        field->text_size = state.text.size();
        geometry.commit(*field);
        if (legacy_auto != object->accessors.end())
            fvm_textfield_upgrade_accessor("autoSize", legacy_auto->second);
        if (legacy_x != object->accessors.end()) fvm_textfield_upgrade_accessor("_x", legacy_x->second);
        if (legacy_y != object->accessors.end()) fvm_textfield_upgrade_accessor("_y", legacy_y->second);
        object->resource = field;
        object->release_resource = release_field;
        return FVM_OK;
    }
    catch (...)
    {
        delete field;
        throw;
    }
}
