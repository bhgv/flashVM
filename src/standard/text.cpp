/* FlashVM: Built-in ActionScript objects: text.
 * File: src/standard/text.cpp.
 */

#include "../core/string_internal.hpp"
#include "../core/runtime.hpp"
#include "flashvm/standard_text.h"
#include "flashvm/standard.h"
#include "flashvm/utf8.h"
#include <cstring>
#include <cmath>
#include <new>
#include <limits>

#include "text_state.hpp"
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
    TEXT_FORMAT = 1001,
    TEXT_FIELD = 1002,
    TEXT_FORMAT_CLASS = 1003,
    TEXT_FIELD_CLASS = 1004
};
const char *properties[] = {"font",        "size",          "color",   "bold",        "italic",
                            "underline",   "url",           "target",  "align",       "leftMargin",
                            "rightMargin", "indent",        "leading", "blockIndent", "bullet",
                            "kerning",     "letterSpacing", "tabStops"};

/* Define Style struct and its representation. */
struct Style
{
    fvm_detail::string font;
    double size, leading;
    unsigned long color;
    FVM_TextAlignment alignment;
    /* Initialize Style and its owned state. */
    Style() : size(12), leading(2), color(0), alignment(FVM_TEXT_ALIGN_LEFT)
    {
    }
    /* Exchange the current value. */
    void swap(Style &s)
    {
        font.swap(s.font);
        double d = size;

        /* Computation / Вычисление и обновление состояния. */
        size = s.size;
        s.size = d;
        d = leading;
        leading = s.leading;
        s.leading = d;
        unsigned long c = color;

        /* Computation / Вычисление и обновление состояния. */
        color = s.color;
        s.color = c;
        FVM_TextAlignment a = alignment;

        /* Computation / Вычисление и обновление состояния. */
        alignment = s.alignment;
        s.alignment = a;
    }
};

/* Define Field struct and its representation. */
struct Field
{
    Style current, future;
    const char *text;
    double width, height;
    bool multiline, wrap;
    FVM_TextLayout *layout;
    /* Initialize Field and its owned state. */
    Field() : text(""), width(100), height(100), multiline(false), wrap(false), layout(0)
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
            if (v.type != FVM_VALUE_STRING)
                return FVM_UNSUPPORTED;
            style.font = v.string;
        }
        else if (std::strcmp(name, "align") == 0)
        {
            if (v.type != FVM_VALUE_STRING)
                return FVM_UNSUPPORTED;
            if (std::strcmp(v.string, "left") == 0)
                style.alignment = FVM_TEXT_ALIGN_LEFT;
            else if (std::strcmp(v.string, "center") == 0)
                style.alignment = FVM_TEXT_ALIGN_CENTER;
            else if (std::strcmp(v.string, "right") == 0)
                style.alignment = FVM_TEXT_ALIGN_RIGHT;
            else
                return FVM_UNSUPPORTED;
        }
        else if (std::strcmp(name, "url") == 0 || std::strcmp(name, "target") == 0)
        {
            if (v.type != FVM_VALUE_STRING || v.string[0])
                return FVM_UNSUPPORTED;
        }
        else if (std::strcmp(name, "tabStops") == 0)
            return FVM_UNSUPPORTED;
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
            else if (std::strcmp(name, "color") == 0)
            {
                if (!finite_range(d, 0, 16777215) || d != std::floor(d))
                    return FVM_INVALID_ARGUMENT;
                style.color = (unsigned long)d;
            }
            else if (std::strcmp(name, "kerning") == 0)
            {
                if (d != 1)
                    return FVM_UNSUPPORTED;
            }
            else if (d != 0)
                return FVM_UNSUPPORTED;
        }
    }
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
        c->fonts.find(style.font);
    if (source == c->fonts.end())
        return FVM_NOT_FOUND;
    return fvm_font_read(source->second->bytes.begin(), source->second->bytes.size(),
                         source->second->index, style.size, out);
#endif
}

/* Build layout. */
static FVM_Result build_layout(FVM_Context *c, const Style &style, const char *text, double width,
                               bool multiline, FVM_TextLayout **out, FVM_FontMetrics *font_metrics)
{

    /* Initialization / Инициализация локального состояния. */
    *out = 0;
    FontOwner font;
    FVM_Result result = open_font(c, style, &font.font);
    if (result != FVM_OK)
        return result;
    FVM_FontMetrics metrics;
    result = fvm_font_metrics(font.font, &metrics);
    if (result != FVM_OK)
        return result;
    double spacing = metrics.ascent + metrics.descent + style.leading;
    if (!finite_range(spacing, 1.0 / 64, 1000000))
        return FVM_INVALID_ARGUMENT;
    size_t size = std::strlen(text), offset = 0;
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
    options.line_height = spacing;
    options.alignment = width ? style.alignment : FVM_TEXT_ALIGN_LEFT;
    result = fvm_text_layout_create(font.font, text, size, &options, out);
    if (result == FVM_OK && font_metrics)
        *font_metrics = metrics;
    return result;
}

/* Compute and return ensure layout. */
static FVM_Result ensure_layout(FVM_Context *c, Field *field)
{
    if (field->layout)
        return FVM_OK;
    double width = field->wrap && field->multiline ? field->width - 4 : 0;
    if (field->wrap && field->multiline && width <= 0)
        width = 1.0 / 64;
    return build_layout(c, field->current, field->text, width, field->multiline, &field->layout, 0);
}

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
    result = build_layout(c, style, a[0].string, width, true, &layout.layout, &font_metrics);
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
                             metrics.height,      metrics.advance_width + 4, metrics.height + 4};
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

    /* Initialization / Инициализация локального состояния. */
    FVM_Value v;
    fvm_value_string(&v, style.font.c_str());
    result = fvm_object_set(c, object, "font", &v);
    if (result != FVM_OK)
        return result;
    fvm_value_number(&v, style.size);
    object->properties["size"] = v;
    fvm_value_number(&v, style.leading);
    object->properties["leading"] = v;
    fvm_value_number(&v, style.color);
    object->properties["color"] = v;

    /* Initialization / Инициализация локального состояния. */
    const char *align = style.alignment == FVM_TEXT_ALIGN_LEFT     ? "left"
                        : style.alignment == FVM_TEXT_ALIGN_CENTER ? "center"
                                                                   : "right";
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
static FVM_Result text_boundary(const char *text, size_t index, size_t &byte, size_t &length)
{

    /* Initialization / Инициализация локального состояния. */
    size_t offset = 0, units = 0, bytes = std::strlen(text), found = 0;
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
    if (!finite_range(d, 0, 8192) || d != std::floor(d))
        return FVM_INVALID_ARGUMENT;
    index = (size_t)d;
    return FVM_OK;
}

/* Compute and return text range. */
static FVM_Result text_range(FVM_Context *c, const char *text, unsigned int n,
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
    result = text_boundary(text, first, begin, length);
    if (result != FVM_OK)
        return result;
    return text_boundary(text, last, end, ignored);
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
    FVM_Result result = text_range(c, field->text, 2, args, begin, end, length);
    if (result != FVM_OK)
        return result;

    /* Initialization / Инициализация локального состояния. */
    const char *replacement = args[2].string;
    size_t bytes = std::strlen(field->text), inserted = fvm_string_size(args[2]), count;
    if (std::memchr(replacement, 0, inserted))
        return FVM_UNSUPPORTED;
    if (inserted > 16384)
        return FVM_UNSUPPORTED;
    if (fvm_utf8_validate(replacement, inserted, &count) != FVM_UTF8_OK)
        return FVM_INVALID_ARGUMENT;
    size_t total = bytes - (end - begin) + inserted;
    if (total > 16384)
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
        if (count > 4096)
            return FVM_UNSUPPORTED;
        /* Preserve the uniform current format. In an empty field the first
         * inserted text uses the future format. Styled spans are not simulated. */
        Style next = length ? field->current : field->future;
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
        field->current.swap(next);
        field->invalidate();
        fvm_value_undefined(out);
        return FVM_OK;
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
            FVM_Result result = text_range(c, field->text, n, args, begin, end, length);
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
    if (future)
        field->future.swap(next);
    else
    {
        field->current.swap(next);
        field->invalidate();
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
    if (std::strcmp(name, "text") == 0)
        return fvm_value_string(out, field->text);
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
        size_t offset = 0, size = std::strlen(field->text), units = 0;
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
    if (std::strcmp(name, "text") == 0)
    {
        if (args[0].type != FVM_VALUE_STRING)
            return FVM_UNSUPPORTED;
        size_t size = fvm_string_size(args[0]), count = 0;
        if (std::memchr(args[0].string, 0, size))
            return FVM_UNSUPPORTED;
        if (size > 16384 || fvm_utf8_validate(args[0].string, size, &count) != FVM_UTF8_OK)
            return FVM_INVALID_ARGUMENT;
        if (count > 4096)
            return FVM_UNSUPPORTED;
        Style next = field->future;
        field->current.swap(next);
        field->text = args[0].string;
        field->invalidate();
    }
    else if (std::strcmp(name, "multiline") == 0 || std::strcmp(name, "wordWrap") == 0)
    {
        if (args[0].type != FVM_VALUE_BOOLEAN)
            return FVM_UNSUPPORTED;
        if (std::strcmp(name, "multiline") == 0)
            field->multiline = args[0].boolean != 0;
        else
            field->wrap = args[0].boolean != 0;
        field->invalidate();
    }
    else
    {
        double d;
        FVM_Result result = number(c, args[0], d);
        if (result != FVM_OK)
            return result;
        if (std::strcmp(name, "textColor") == 0)
        {
            if (!finite_range(d, 0, 16777215) || d != std::floor(d))
                return FVM_INVALID_ARGUMENT;
            field->current.color = field->future.color = (unsigned long)d;
        }
        else
        {
            if (!finite_range(d, 0, 1000000))
                return FVM_INVALID_ARGUMENT;
            if (std::strcmp(name, "_width") == 0)
                field->width = d;
            else
                field->height = d;
            field->invalidate();
        }
    }
    fvm_value_undefined(out);
    return FVM_OK;
}
} // namespace
/* Map a native text callback to its stable serialization identifier. */
unsigned long fvm_text_callback_id(FVM_NativeFunction callback)
{
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

/* Register standard text font. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_standard_text_register_font(
    FVM_Context *c, const char *name,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const void *bytes, size_t size, long index)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
    if (!c || !name || !*name || std::strlen(name) > 255 || c->destroying || c->gc_collecting ||
        /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
        c->gc_releasing)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (c->fonts.find(name) != c->fonts.end())
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    size_t total = size;
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (fvm_detail::map<fvm_detail::string, FVM_FontSource *>::iterator i = c->fonts.begin();
         /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
         i != c->fonts.end(); ++i)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (total > 64 * 1024 * 1024 - i->second->bytes.size())
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_UNSUPPORTED;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        total += i->second->bytes.size();
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (total > 64 * 1024 * 1024 || c->fonts.size() >= 16)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_UNSUPPORTED;

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FontOwner font;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result result = fvm_font_read(bytes, size, index, 12, &font.font);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (result != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return result;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_FontSource *source = 0;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        source = new FVM_FontSource;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        source->index = index;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        source->bytes.resize(size);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        std::memcpy(source->bytes.begin(), bytes, size);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::string default_name = c->default_font.empty() ? name : c->default_font.c_str();
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->fonts[name] = source;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        source = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->default_font.swap(default_name);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    catch (...)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Удаляем запись или область корней после завершения работы с ней. */
        delete source;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OUT_OF_MEMORY;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
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
        !finite_range(width, 0, 1000000) || !finite_range(height, 0, 1000000))
        return FVM_INVALID_ARGUMENT;
    Field *field = 0;
    try
    {
        field = new Field;
        field->current.font = font_name ? font_name : c->default_font.c_str();
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
                               "textColor", "length",    "textWidth", "textHeight"};
        for (size_t i = 0; i < 9; ++i)
        {
            result = fvm_object_define_accessor_rooted(c, object, names[i], field_get,
                                                       i < 6 ? field_set : 0, (void *)names[i], 0);
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
        const char *unimplemented[] = {
            "html",       "htmlText",     "type",          "autoSize",
            "border",     "borderColor",  "background",    "backgroundColor",
            "embedFonts", "password",     "selectable",    "maxChars",
            "restrict",   "scroll",       "hscroll",       "maxscroll",
            "maxhscroll", "bottomScroll", "antiAliasType", "gridFitType",
            "sharpness",  "thickness",    "condenseWhite", "filters",
            "_x",         "_y",           "_alpha",        "_visible"};
        for (size_t i = 0; i < sizeof(unimplemented) / sizeof(unimplemented[0]); ++i)
        {
            result = fvm_object_define_accessor_rooted(c, object, unimplemented[i], unsupported,
                                                       unsupported, 0, 0);
            if (result != FVM_OK)
                return result;
        }
        const char *unimplemented_methods[] = {"replaceSel", "removeTextField", "getDepth",
                                               "addListener", "removeListener"};
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
        double shift = 0;
        if (result == FVM_OK && !(field->wrap && field->multiline))
        {
            double available = field->width - 4 - line.advance;
            if (available > 0 && field->current.alignment == FVM_TEXT_ALIGN_CENTER)
                shift = available / 2;
            else if (available > 0 && field->current.alignment == FVM_TEXT_ALIGN_RIGHT)
                shift = available;
        }
        FVM_Affine place = {1, 0, 0, 1, 2 + shift, 2};
        if (result == FVM_OK)
            result = fvm_path_append(path, copy, &place);
        fvm_path_destroy(copy);
    }
    if (result != FVM_OK)
    {
        fvm_path_destroy(path);
        return result;
    }
    *out = path;
    return FVM_OK;
}

namespace
{
const char *field_selectors[] = {
    "getTextFormat", "getNewTextFormat", "setTextFormat", "setNewTextFormat", "text",
    "multiline",     "wordWrap",         "_width",        "_height",          "textColor",
    "length",        "textWidth",        "textHeight"};

/* Capture snapshot of style. */
void snapshot_style(FVM_Context *context, const Style &style, FVM_TextStyleState &out)
{

    /* Computation / Вычисление и обновление состояния. */
    out.font = style.font;
    out.size = style.size;
    out.leading = style.leading;
    out.color = style.color;
    out.alignment = (unsigned long)style.alignment;
    out.source = *context->fonts.find(style.font)->second;
}

/* Restore style. */
FVM_Result restore_style(FVM_Context *context, const FVM_TextStyleState &src, Style &out)
{
    if (src.font.empty() || src.font.size() > 255 || !finite_range(src.size, 1, 4096) ||
        !finite_range(src.leading, -4096, 4096) || src.color > 16777215UL || src.alignment > 2)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::map<fvm_detail::string, FVM_FontSource *>::iterator existing =
        context->fonts.find(src.font);
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
            fvm_standard_text_register_font(context, src.font.c_str(), src.source.bytes.begin(),
                                            src.source.bytes.size(), src.source.index);
        if (result != FVM_OK)
            return result;
    }

    /* Computation / Вычисление и обновление состояния. */
    out.font = src.font;
    out.size = src.size;
    out.leading = src.leading;
    out.color = src.color;
    out.alignment = (FVM_TextAlignment)src.alignment;
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
        if (!std::strcmp(name, field_selectors[i]) && ((id == 201) == (i < 4)) &&
            (id != 203 || i < 10))
            return field_selectors[i];
    return 0;
}

/* Compute and return text callback userdata. */
void *fvm_text_callback_userdata(unsigned long id, const char *name)
{
    return (void *)fvm_text_callback_selector(fvm_text_callback_from_id(id), (void *)name);
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
        context->fonts.find(field->current.font);
    fvm_detail::map<fvm_detail::string, FVM_FontSource *>::iterator b =
        context->fonts.find(field->future.font);
    if (a == context->fonts.end() || b == context->fonts.end())
        return (size_t)-1;
    return 128 + std::strlen(field->text) + field->current.font.size() + field->future.font.size() +
           a->second->bytes.size() + b->second->bytes.size();
}

/* Capture snapshot of textfield. */
FVM_Result fvm_textfield_snapshot(FVM_Object *object, FVM_TextFieldState &out)
{
    Field *field = (Field *)object->resource;
    if (!field || fvm_textfield_snapshot_size(object) == (size_t)-1)
        return FVM_UNSUPPORTED;

    /* Computation / Вычисление и обновление состояния. */
    snapshot_style(object->owner, field->current, out.current);
    snapshot_style(object->owner, field->future, out.future);
    out.text = field->text;
    out.width = field->width;
    out.height = field->height;
    out.multiline = field->multiline;
    out.wrap = field->wrap;
    return FVM_OK;
}

/* Restore textfield. */
FVM_Result fvm_textfield_restore(FVM_Context *context, FVM_Object *object,
                                 const FVM_TextFieldState &state)
{
    if (!context->standard_text_library)
        return FVM_UNSUPPORTED;
    if (!finite_range(state.width, 0, 1000000) || !finite_range(state.height, 0, 1000000) ||
        fvm_utf8_validate(state.text.c_str(), state.text.size(), 0) != FVM_UTF8_OK)
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
        field->width = state.width;
        field->height = state.height;
        field->multiline = state.multiline;
        field->wrap = state.wrap;
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
