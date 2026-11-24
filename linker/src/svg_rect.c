/* Copyright (c) 2026 bhgv
 * EN: Read the supported SVG rectangle profile before committing a SWL resource.
 * RU: Разобрать поддерживаемый профиль прямоугольника SVG до добавления ресурса SWL.
 */
#include "internal.h"
#include <string.h>

#define SVG_RECT_LIMIT 8192u /* Maximum number of borrowed XML bytes. */
#define SVG_MAX_PIXELS 3276u /* Largest dimension supported by the shape encoder. */


/* EN: Borrow a span from the caller's SVG input for the duration of parsing.
 * RU: Заимствованный фрагмент входного SVG на время разбора. */
typedef struct
{
    const char *text; /* Null only for an absent attribute. */
    size_t size;      /* Byte length, excluding any delimiters. */
} SvgSpan;


/* EN: Track progress over counted input with no NUL terminator required.
 * RU: Позиция во входе заданной длины без обязательного завершающего NUL. */
typedef struct
{
    const char *next; /* First unread byte. */
    const char *end;  /* One past the caller's byte range. */
} SvgInput;


/* EN: Match one exact ASCII spelling without reading beyond text.
 * RU: Сравнить фрагмент с точным ASCII-значением в его границах. */
static int equals(SvgSpan span, const char *text)
{
    size_t size = strlen(text);
    return span.text && span.size == size && !memcmp(span.text, text, size);
}


/* EN: Consume spelling at the cursor if it fits and matches.
 * RU: Продвинуть позицию на длину совпавшего фрагмента. */
static int take(SvgInput *in, const char *text)
{
    size_t size = strlen(text);
    if ((size_t)(in->end - in->next) < size || memcmp(in->next, text, size))
    {
        return 0;
    }
    in->next += size;
    return 1;
}


/* EN: Recognize XML whitespace on a byte, independent of the process locale.
 * RU: Проверить, является ли байт пробельным символом XML, независимо от локали. */
static int xml_space(char byte)
{
    return byte == ' ' || byte == '\t' || byte == '\r' || byte == '\n';
}


/* EN: Advance across any XML whitespace at the cursor.
 * RU: Продвинуть позицию через пробельные символы XML. */
static void skip_space(SvgInput *in)
{
    while (in->next != in->end && xml_space(*in->next))
    {
        ++in->next;
    }
}


/* EN: Consume one XML 1.0 character from a UTF-8 comment.
 * RU: Прочитать один символ XML 1.0 из комментария UTF-8. */
static int comment_character(SvgInput *in)
{
    unsigned first = (unsigned char)*in->next++;
    uint32_t rune, minimum;
    unsigned remaining;
    if (first < 0x80)
    {
        return first >= 0x20 || xml_space((char)first);
    }
    if (first >= 0xc2 && first <= 0xdf)
    {
        rune = first & 0x1f;
        minimum = 0x80;
        remaining = 1;
    }
    else if (first >= 0xe0 && first <= 0xef)
    {
        rune = first & 0x0f;
        minimum = 0x800;
        remaining = 2;
    }
    else if (first >= 0xf0 && first <= 0xf4)
    {
        rune = first & 0x07;
        minimum = 0x10000;
        remaining = 3;
    }
    else
    {
        return 0;
    }
    while (remaining)
    {
        unsigned next;
        if (in->next == in->end)
        {
            return 0;
        }
        next = (unsigned char)*in->next++;
        if ((next & 0xc0) != 0x80)
        {
            return 0;
        }
        rune = (rune << 6) | (next & 0x3f);
        --remaining;
    }
    return rune >= minimum && rune <= 0x10ffff && !(rune >= 0xd800 && rune <= 0xdfff) &&
        rune != 0xfffe && rune != 0xffff;
}


/* EN: Skip whitespace and complete XML comments. Leave the next element unread.
 * RU: Пропустить пробелы и целые комментарии XML до следующего элемента. */
static int skip_misc(SvgInput *in)
{
    skip_space(in);
    while (take(in, "<!--"))
    {
        while ((size_t)(in->end - in->next) >= 2 && memcmp(in->next, "--", 2))
        {
            if (!comment_character(in))
            {
                return 0;
            }
        }
        if (!take(in, "-->"))
        {
            return 0;
        }
        skip_space(in);
    }
    return 1;
}


/* EN: Read a quoted value. This profile accepts literal attributes without entities.
 * RU: Прочитать значение в кавычках. Профиль принимает литералы без XML-сущностей. */
static int quoted(SvgInput *in, SvgSpan *value)
{
    char delimiter;
    if (in->next == in->end || (*in->next != '"' && *in->next != '\''))
    {
        return 0;
    }
    delimiter = *in->next++;
    value->text = in->next;
    while (in->next != in->end && *in->next != delimiter && *in->next != '<' && *in->next != '&')
    {
        ++in->next;
    }
    if (in->next == in->end || *in->next != delimiter)
    {
        return 0;
    }
    value->size = (size_t)(in->next - value->text);
    ++in->next;
    return 1;
}


/* EN: Parse a tag's unique authorized keys into caller-zeroed spans. An optional
 * empty output permits a self-closing element. Reject unknown rendering attributes.
 * RU: Заполнить обнулённые фрагменты значениями разрешённых уникальных атрибутов.
 * Параметр empty разрешает самозакрытие. Неизвестные атрибуты отклоняются. */
static int attributes(SvgInput *in, const char *end, const char *const *keys, SvgSpan *values,
    size_t count, int *empty)
{
    for (;;)
    {
        const char *before = in->next;
        SvgSpan name;
        size_t index;
        skip_space(in);
        if (take(in, end))
        {
            return 1;
        }
        if (empty && take(in, "/>"))
        {
            *empty = 1;
            return 1;
        }
        if (in->next == before)
        {
            return 0;
        }
        name.text = in->next;
        while (in->next != in->end && !xml_space(*in->next) && *in->next != '=')
        {
            ++in->next;
        }
        name.size = (size_t)(in->next - name.text);
        for (index = 0; index < count && !equals(name, keys[index]); ++index)
        {
        }
        skip_space(in);
        if (index == count || values[index].text || !take(in, "="))
        {
            return 0;
        }
        skip_space(in);
        if (!quoted(in, &values[index]))
        {
            return 0;
        }
    }
}


/* EN: Read a matching end tag after permitted whitespace or comments.
 * RU: Прочитать закрывающий тег после допустимых пробелов и комментариев. */
static int close_tag(SvgInput *in, const char *start)
{
    if (!skip_misc(in) || !take(in, start))
    {
        return 0;
    }
    skip_space(in);
    return take(in, ">");
}


/* EN: Parse whole pixels, optionally suffixed by px, with overflow rejection.
 * RU: Прочитать целые пиксели с необязательным px и проверкой переполнения. */
static int pixels(SvgSpan span, unsigned *result)
{
    size_t i;
    unsigned value = 0;
    if (span.size >= 2 && !memcmp(span.text + span.size - 2, "px", 2))
    {
        span.size -= 2;
    }
    if (!span.size)
    {
        return 0;
    }
    for (i = 0; i < span.size; ++i)
    {
        unsigned digit = (unsigned)(span.text[i] - '0');
        if (digit > 9 || value > (SVG_MAX_PIXELS - digit) / 10)
        {
            return 0;
        }
        value = value * 10 + digit;
    }
    *result = value;
    return 1;
}


/* EN: Parse a supported SVG solid fill, defaulting to black for absent fill.
 * RU: Прочитать поддерживаемый сплошной цвет SVG. По умолчанию чёрный. */
static int fill(SvgSpan span, uint32_t *rgb)
{
    size_t i;
    uint32_t value = 0;
    if (!span.text)
    {
        *rgb = 0;
        return 1;
    }
    if ((span.size != 4 && span.size != 7) || span.text[0] != '#')
    {
        return 0;
    }
    for (i = 1; i < span.size; ++i)
    {
        unsigned char ch = (unsigned char)span.text[i];
        unsigned digit;
        if (ch >= '0' && ch <= '9')
        {
            digit = ch - '0';
        }
        else if (ch >= 'a' && ch <= 'f')
        {
            digit = ch - 'a' + 10;
        }
        else if (ch >= 'A' && ch <= 'F')
        {
            digit = ch - 'A' + 10;
        }
        else
        {
            return 0;
        }
        if (span.size == 4)
        {
            value = (value << 8) | (digit * 17);
        }
        else
        {
            value = (value << 4) | digit;
        }
    }
    *rgb = value;
    return 1;
}


/* EN: Convert a unitless decimal SVG fill-opacity to one rounded alpha byte.
 * RU: Преобразовать десятичную долю fill-opacity в округлённый байт альфа-канала. */
static int opacity(SvgSpan span, unsigned *alpha)
{
    uint32_t parts = 0, scale = 1;
    unsigned whole = 0;
    size_t i = 0;
    if (!span.text || equals(span, "1"))
    {
        *alpha = 255;
        return 1;
    }
    if (equals(span, "0"))
    {
        *alpha = 0;
        return 1;
    }
    if (span.size && (span.text[0] == '0' || span.text[0] == '1'))
    {
        whole = (unsigned)(span.text[i++] - '0');
    }
    if (span.size < i + 2 || span.size > i + 7 || span.text[i++] != '.')
    {
        return 0;
    }
    for (; i < span.size; ++i)
    {
        unsigned digit = (unsigned)(span.text[i] - '0');
        if (digit > 9)
        {
            return 0;
        }
        parts = parts * 10 + digit;
        scale *= 10;
    }
    if (whole)
    {
        *alpha = 255;
        return parts == 0;
    }
    *alpha = (unsigned)((parts * 255 + scale / 2) / scale);
    return 1;
}


/* EN: Validate the complete borrowed SVG before adding the derived shape.
 * RU: Проверить весь входной SVG перед добавлением полученной фигуры. */
int swl_add_svg(SwlContext *c, const char *name, const void *data, size_t size, char *error, size_t capacity)
{
    static const char *const xml_keys[] = {"version", "encoding"};
    static const char *const svg_keys[] = {"xmlns", "width", "height"};
    static const char *const rect_keys[] = {"width", "height", "x", "y", "fill", "fill-opacity"};
    SvgSpan xml[2] = {{0}}, svg[3] = {{0}}, rect[6] = {{0}};
    SvgInput in;
    unsigned width, height, rect_width, rect_height, origin, alpha;
    uint32_t rgb;
    int empty = 0;
    if (!data || !size || size > SVG_RECT_LIMIT || memchr(data, 0, size))
    {
        return swl_fail(error, capacity, "SVG rectangle input must be 1..8192 bytes without NUL");
    }
    in.next = data;
    in.end = in.next + size;
    (void)take(&in, "\xef\xbb\xbf");
    if (take(&in, "<?xml") &&
        (!attributes(&in, "?>", xml_keys, xml, 2, NULL) || !equals(xml[0], "1.0") ||
         (xml[1].text && (xml[1].text < xml[0].text || !equals(xml[1], "UTF-8")))))
    {
        return swl_fail(error, capacity, "expected a UTF-8 XML 1.0 declaration");
    }
    if (!skip_misc(&in) || !take(&in, "<svg") || !attributes(&in, ">", svg_keys, svg, 3, NULL) ||
        !equals(svg[0], "http://www.w3.org/2000/svg") ||
        !skip_misc(&in) || !take(&in, "<rect") || !attributes(&in, ">", rect_keys, rect, 6, &empty) ||
        (!empty && !close_tag(&in, "</rect")) || !close_tag(&in, "</svg") ||
        !skip_misc(&in) || in.next != in.end)
    {
        return swl_fail(error, capacity, "expected one rect in an SVG document with supported attributes");
    }
    if (!pixels(svg[1], &width) || !pixels(svg[2], &height) ||
        !pixels(rect[0], &rect_width) || !pixels(rect[1], &rect_height) ||
        !width || !height || rect_width != width || rect_height != height ||
        (rect[2].text && (!pixels(rect[2], &origin) || origin)) ||
        (rect[3].text && (!pixels(rect[3], &origin) || origin)))
    {
        return swl_fail(error, capacity, "SVG rect must fill the viewport at (0,0), with dimensions in 1..3276 px");
    }
    if (!fill(rect[4], &rgb) || !opacity(rect[5], &alpha))
    {
        return swl_fail(error, capacity, "expected SVG hex fill (#RGB or #RRGGBB) and decimal fill-opacity in [0,1]");
    }
    return swl_add_rect(c, name, width, height, (rgb << 8) | alpha, error, capacity);
}
