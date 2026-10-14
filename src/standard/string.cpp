/* FlashVM: Built-in ActionScript objects: string.
 * File: src/standard/string.cpp.
 */

#include "string_state.hpp"
#include "../core/string_internal.hpp"
#include "../../external/unicode/case_table.hpp"
#include "../../external/flash_case/case_table.hpp"
#include "../avm1/coercion.hpp"
#include "base_state.hpp"
#include "flashvm/table.h"
#include "flashvm/standard.h"
#include "flashvm/utf8.h"
#include <cmath>
#include <cstring>
#include <limits>
namespace
{
/* Define anonymous enum and its representation. */
enum
{
    STRING_OBJECT = 26,
    STRING_CLASS = 27
};
typedef fvm_detail::vector<unsigned int> Units;

/* Decode a counted string into UTF16 units, including lone surrogates. */
FVM_Result decode(const FVM_Value &text, Units &units)
{
    return fvm_string_units(text, units);
}

/* Encode the current value. */
FVM_Result encode(FVM_Context *c, const Units &units, size_t start, size_t end, FVM_Value *out)
{
    fvm_detail::vector<char> text;
    for (size_t i = start; i < end; ++i)
    {
        unsigned long cp = units[i];
        if (cp >= 0xd800 && cp <= 0xdbff && i + 1 < end && units[i + 1] >= 0xdc00 &&
            units[i + 1] <= 0xdfff)
        {
            cp = 0x10000 + ((cp - 0xd800) << 10) + units[++i] - 0xdc00;
        }
        FVM_Result r = fvm_string_append(cp, text);
        if (r != FVM_OK)
            return r;
    }
    size_t length = text.size();
    text.push_back(0);
    FVM_Value temporary;
    fvm_value_string_bytes(&temporary, text.begin(), length);
    return fvm_value_to_string(c, &temporary, out);
}

/* Compute and return value. */
FVM_Result value(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *, FVM_Value *out,
                 void *)
{
    if (!self || self->owner != c || self->resource_kind != STRING_OBJECT ||
        !self->resource_result_valid || self->resource_result.type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    *out = self->resource_result;
    return FVM_OK;
}

/* Decode a borrowed receiver after the current version's string coercion. */
FVM_Result receiver(FVM_Context *c, FVM_Object *self, Units &units)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value text;
    FVM_Result result;
    if (self->resource_kind == STRING_OBJECT)
        result = value(c, self, 0, 0, &text, 0);
    else
    {
        FVM_Value object;
        fvm_value_undefined(&object);
        object.type = FVM_VALUE_OBJECT;
        object.object = self;
        result = c->swf_loaded ? fvm_avm1_string(c, object, &text)
                               : fvm_value_to_string(c, &object, &text);
    }
    return result == FVM_OK ? decode(text, units) : result;
}

/* Compute and return argument. */
FVM_Result argument(FVM_Context *c, unsigned int count, const FVM_Value *args, unsigned int index,
                    double fallback, double &number)
{
    if (index >= count)
    {
        number = fallback;
        return FVM_OK;
    }
    FVM_Result result = fvm_value_to_number(c, args + index, &number);
    if (result != FVM_OK)
        return result;
    if (number != number)
        number = 0;
    else if (number > 0)
        number = std::floor(number);
    else
        number = -std::floor(-number);
    return FVM_OK;
}

/* Compute and return clamp. */
size_t clamp(double number, size_t length, bool relative)
{
    if (relative && number < 0)
        number += (double)length;
    if (number <= 0)
        return 0;
    if (number >= (double)length)
        return length;
    return (size_t)number;
}

/* Compute and return length. */
FVM_Result length(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *, FVM_Value *out,
                  void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value text;
    FVM_Result result = value(c, self, 0, 0, &text, 0);
    if (result != FVM_OK)
        return result;
    Units units;
    result = decode(text, units);
    if (result == FVM_OK)
        fvm_value_number(out, (double)units.size());
    return result;
}

/* Compute and return character. */
FVM_Result character(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args,
                     FVM_Value *out, bool code)
{

    /* Initialization / Инициализация локального состояния. */
    Units units;
    FVM_Result result = receiver(c, self, units);
    if (result != FVM_OK)
        return result;
    double index = 0;
    result = argument(c, count, args, 0, 0, index);
    if (result != FVM_OK)
        return result;
    if (index < 0 || index >= (double)units.size())
    {
        if (code)
            fvm_value_number(out, std::numeric_limits<double>::quiet_NaN());
        else
            fvm_value_string(out, "");
        return FVM_OK;
    }
    size_t i = (size_t)index;
    if (code)
    {
        fvm_value_number(out, units[i]);
        return FVM_OK;
    }
    return encode(c, units, i, i + 1, out);
}

/* Compute and return char at. */
FVM_Result char_at(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a, FVM_Value *o,
                   void *)
{
    return character(c, s, n, a, o, false);
}

/* Compute and return char code. */
FVM_Result char_code(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a,
                     FVM_Value *o, void *)
{
    return character(c, s, n, a, o, true);
}

/* Compute and return portion. */
FVM_Result portion(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args,
                   FVM_Value *out, int mode)
{

    /* Initialization / Инициализация локального состояния. */
    Units units;
    FVM_Result result = receiver(c, self, units);
    if (result != FVM_OK)
        return result;
    double first = 0, last = 0;
    result = argument(c, count, args, 0, 0, first);
    if (result != FVM_OK)
        return result;
    result = argument(c, count > 1 && args[1].type == FVM_VALUE_UNDEFINED ? 1 : count, args, 1,
                      (double)units.size(), last);
    if (result != FVM_OK)
        return result;
    size_t start = clamp(first, units.size(), mode != 0), end;
    if (mode == 2) /* substr's second argument is a count. */
        end = start + clamp(last, units.size() - start, false);
    else
        end = clamp(last, units.size(), mode == 1);
    if (mode == 0 && start > end)
    {
        size_t swap = start;
        start = end;
        end = swap;
    }
    if (end < start)
        end = start;
    return encode(c, units, start, end, out);
}

/* Compute and return substring. */
FVM_Result substring(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a,
                     FVM_Value *o, void *)
{
    return portion(c, s, n, a, o, 0);
}

/* Copy the selected Array range, preserving absent indexed properties. */
FVM_Result slice(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a, FVM_Value *o,
                 void *)
{
    return portion(c, s, n, a, o, 1);
}

/* Compute and return substr. */
FVM_Result substr(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a, FVM_Value *o,
                  void *)
{
    return portion(c, s, n, a, o, 2);
}

/* Compute and return search. */
FVM_Result search(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args,
                  FVM_Value *out, bool backwards)
{

    /* Initialization / Инициализация локального состояния. */
    Units haystack, needle;
    FVM_Result result = receiver(c, self, haystack);
    if (result != FVM_OK)
        return result;

    /* Initialization / Инициализация локального состояния. */
    FVM_Value absent, text;
    fvm_value_undefined(&absent);
    result = fvm_value_to_string(c, count ? args : &absent, &text);
    if (result != FVM_OK)
        return result;
    result = decode(text, needle);
    if (result != FVM_OK)
        return result;
    double position = backwards ? (double)haystack.size() : 0;
    if (count > 1)
    {
        result = fvm_value_to_number(c, args + 1, &position);
        if (result != FVM_OK)
            return result;
        if (position != position)
            position = backwards ? (double)haystack.size() : 0;
        else
            position = position < 0 ? -std::floor(-position) : std::floor(position);
    }
    size_t start = clamp(position, haystack.size(), false);
    if (needle.empty())
    {
        fvm_value_number(out, (double)start);
        return FVM_OK;
    }
    if (needle.size() > haystack.size())
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    size_t max = haystack.size() - needle.size();
    if (backwards && start > max)
        start = max;
    for (size_t i = start; i <= max;)
    {
        bool equal = true;
        for (size_t j = 0; j < needle.size(); ++j)
            if (haystack[i + j] != needle[j])
            {
                equal = false;
                break;
            }
        if (equal)
        {
            fvm_value_number(out, (double)i);
            return FVM_OK;
        }
        if (backwards)
        {
            if (!i)
                break;
            --i;
        }
        else
            ++i;
    }
    fvm_value_number(out, -1);
    return FVM_OK;
}

/* Compute and return index of. */
FVM_Result index_of(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a, FVM_Value *o,
                    void *)
{
    return search(c, s, n, a, o, false);
}

/* Compute and return last index. */
FVM_Result last_index(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a,
                      FVM_Value *o, void *)
{
    return search(c, s, n, a, o, true);
}

/* Concatenate Array arguments and append non-Array arguments as single values. */
FVM_Result concat(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args,
                  FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    Units joined;
    FVM_Result result = receiver(c, self, joined);
    if (result != FVM_OK)
        return result;
    for (unsigned int i = 0; i < count; ++i)
    {
        FVM_Value text;
        result = fvm_value_to_string(c, args + i, &text);
        if (result != FVM_OK)
            return result;
        result = decode(text, joined);
        if (result != FVM_OK)
            return result;
    }
    return encode(c, joined, 0, joined.size(), out);
}

/* Resolve from char code. */
FVM_Result from_char_code(FVM_Context *c, FVM_Object *, unsigned int count, const FVM_Value *args,
                          FVM_Value *out, void *)
{
    Units units;
    for (unsigned int i = 0; i < count; ++i)
    {
        double number = 0;
        FVM_Result result = argument(c, count, args, i, 0, number);
        if (result != FVM_OK)
            return result;
        if (std::fabs(number) == std::numeric_limits<double>::infinity())
            number = 0;
        number = std::fmod(number, 65536.0);
        if (number < 0)
            number += 65536.0;
        units.push_back((unsigned int)number);
    }
    return encode(c, units, 0, units.size(), out);
}

/* Compute and return split. */
FVM_Result split(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *a,
                 FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    Units input, separator;
    FVM_Result r = receiver(c, self, input);
    if (r != FVM_OK)
        return r;
    unsigned long limit = 0xffffffffUL;
    if (n > 1 && a[1].type != FVM_VALUE_UNDEFINED)
    {
        double value = 0;
        r = fvm_value_to_number(c, a + 1, &value);
        if (r != FVM_OK)
            return r;
        if (!(value == value) || value == std::numeric_limits<double>::infinity() ||
            value == -std::numeric_limits<double>::infinity())
            value = 0;
        value = std::fmod(value < 0 ? -std::floor(-value) : std::floor(value), 4294967296.0);
        if (value < 0)
            value += 4294967296.0;
        limit = (unsigned long)value;
    }
    bool absent = !n || a[0].type == FVM_VALUE_UNDEFINED;
    if (!absent)
    {
        FVM_Value text;
        r = fvm_value_to_string(c, a, &text);
        if (r == FVM_OK)
            r = decode(text, separator);
        if (r != FVM_OK)
            return r;
    }
    FVM_Object *result = 0;
    r = fvm_array_create_like(c, 0, 0, &result, true);
    if (r != FVM_OK)
        return r;

    /* Initialization / Инициализация локального состояния. */
    unsigned long count = 0;
    size_t start = 0;
    /* Walk UTF-16 boundaries, retaining empty fields for nonempty separators. */
    for (size_t position = 0; count < limit && position <= input.size();)
    {
        size_t end = position;
        bool matched = false;
        if (absent)
        {
            end = input.size();
            matched = true;
        }
        else if (separator.empty())
        {
            if (position == input.size())
                break;
            end = position + 1;
            matched = true;
        }
        else if (position == input.size())
            matched = true;
        else if (separator.size() <= input.size() - position)
        {
            matched = true;
            for (size_t j = 0; j < separator.size(); ++j)
                if (separator[j] != input[position + j])
                {
                    matched = false;
                    break;
                }
        }
        if (!matched)
        {
            ++position;
            continue;
        }
        if (count >= 65536)
            return FVM_OUT_OF_MEMORY;
        FVM_Value part;
        r = encode(c, input, start, end, &part);
        if (r == FVM_OK)
            r = fvm_array_store_own(c, result, count++, part);
        if (r != FVM_OK)
            return r;
        if (absent || position == input.size())
            break;
        position = separator.empty() ? end : position + separator.size();
        start = position;
    }
    /* Own result definitions do not assign through length/index descriptors. */
    result->resource_result.number = count;
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    out->type = FVM_VALUE_OBJECT;
    out->object = result;
    return FVM_OK;
}

/* Compute and return in ranges. */
template <size_t N> bool in_ranges(unsigned long cp, const FVM_CaseRange (&ranges)[N])
{
    size_t first = 0, last = N;
    while (first < last)
    {
        size_t middle = first + (last - first) / 2;
        if (cp < ranges[middle].first)
            last = middle;
        else if (cp > ranges[middle].last)
            first = middle + 1;
        else
            return true;
    }
    return false;
}

/* Compute and return case text. */
FVM_Result case_text(FVM_Context *c, const FVM_Value &text, FVM_Value *out, bool upper)
{

    /* Initialization / Инициализация локального состояния. */
    Units units;
    FVM_Result r = decode(text, units);
    if (r != FVM_OK)
        return r;
    if (c->swf_loaded)
    {
        /* Flash maps UCS2 units independently, without expansions or scalar casing. */
        const unsigned short (*table)[2] = upper ? fvm_flash_uppercase : fvm_flash_lowercase;
        size_t count = upper ? sizeof(fvm_flash_uppercase) / sizeof(fvm_flash_uppercase[0])
                             : sizeof(fvm_flash_lowercase) / sizeof(fvm_flash_lowercase[0]);
        for (size_t i = 0; i < units.size(); ++i)
        {
            size_t first = 0, last = count;
            while (first < last)
            {
                size_t middle = first + (last - first) / 2;
                if (table[middle][0] < units[i])
                    first = middle + 1;
                else
                    last = middle;
            }
            if (first < count && table[first][0] == units[i])
                units[i] = table[first][1];
        }
        return encode(c, units, 0, units.size(), out);
    }
    fvm_detail::vector<unsigned long> scalars;
    for (size_t i = 0; i < units.size(); ++i)
    {
        unsigned long cp = units[i];
        if (cp >= 0xd800 && cp <= 0xdbff && i + 1 < units.size() && units[i + 1] >= 0xdc00 &&
            units[i + 1] <= 0xdfff)
            cp = 0x10000 + ((cp - 0xd800) << 10) + units[++i] - 0xdc00;
        scalars.push_back(cp);
    }

    /* Initialization / Инициализация локального состояния. */
    fvm_detail::vector<char> bytes;
    bool preceding_cased = false;
    for (size_t i = 0; i < scalars.size(); ++i)
    {
        unsigned long cp = scalars[i];
        size_t first = 0, last = sizeof(fvm_case_mappings) / sizeof(fvm_case_mappings[0]);
        while (first < last)
        {
            size_t middle = first + (last - first) / 2;
            if (fvm_case_mappings[middle].cp < cp)
                first = middle + 1;
            else
                last = middle;
        }
        /* Unicode default casing uses the original surrounding text. Locale
         * specific mappings do not participate in AS's nonlocale methods. */
        bool final_sigma = false;
        if (!upper && cp == 0x3a3 && preceding_cased)
        {
            size_t next = i + 1;
            while (next < scalars.size() && in_ranges(scalars[next], fvm_case_ignored))
                ++next;
            final_sigma = next == scalars.size() || !in_ranges(scalars[next], fvm_case_cased);
        }
        if (final_sigma)
            r = fvm_string_append(0x3c2, bytes);
        else if (first < sizeof(fvm_case_mappings) / sizeof(fvm_case_mappings[0]) &&
                 fvm_case_mappings[first].cp == cp)
        {
            const FVM_CaseMapping &mapping = fvm_case_mappings[first];
            const unsigned long *target = upper ? mapping.upper : mapping.lower;
            unsigned int count = upper ? mapping.nu : mapping.nl;
            for (unsigned int j = 0; j < count && r == FVM_OK; ++j)
                r = fvm_string_append(target[j], bytes);
        }
        else
            r = fvm_string_append(cp, bytes);
        if (r != FVM_OK)
            return r;
        if (!in_ranges(cp, fvm_case_ignored))
            preceding_cased = in_ranges(cp, fvm_case_cased);
    }
    size_t size = bytes.size();
    bytes.push_back(0);
    FVM_Value temporary;
    r = fvm_value_string_bytes(&temporary, bytes.begin(), size);
    return r == FVM_OK ? fvm_value_to_string(c, &temporary, out) : r;
}

/* Compute and return lower case. */
FVM_Result lower_case(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *,
                      FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    Units units;
    FVM_Result r = receiver(c, o, units);
    FVM_Value text;
    if (r == FVM_OK)
        r = encode(c, units, 0, units.size(), &text);
    return r == FVM_OK ? case_text(c, text, out, false) : r;
}

/* Compute and return upper case. */
FVM_Result upper_case(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *,
                      FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    Units units;
    FVM_Result r = receiver(c, o, units);
    FVM_Value text;
    if (r == FVM_OK)
        r = encode(c, units, 0, units.size(), &text);
    return r == FVM_OK ? case_text(c, text, out, true) : r;
}

/* Define Method struct and its representation. */
struct Method
{
    const char *name;
    FVM_NativeFunction callback;
};
const Method methods[] = {
    {"valueOf", value},         {"toString", value},      {"charAt", char_at},
    {"charCodeAt", char_code},  {"substring", substring}, {"slice", slice},
    {"substr", substr},         {"indexOf", index_of},    {"lastIndexOf", last_index},
    {"concat", concat},         {"split", split},         {"toLowerCase", lower_case},
    {"toUpperCase", upper_case}};

/* Implement attach length for this subsystem. */
void attach_length(FVM_Object *object)
{
    FVM_AccessorEntry accessor;

    /* Computation / Вычисление и обновление состояния. */
    accessor.getter = length;
    object->accessors["length"] = accessor;
}

/* Compute and return constructor. */
FVM_Result constructor(FVM_Context *c, FVM_Object *, unsigned int count, const FVM_Value *args,
                       FVM_Value *out, void *userdata)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value text, prototype;
    FVM_Result result = count ? fvm_value_to_string(c, args, &text) : fvm_value_string(&text, "");
    if (result != FVM_OK)
        return result;
    Units units;
    result = decode(text, units);
    if (result != FVM_OK)
        return result;
    result = fvm_object_get(c, (FVM_Object *)userdata, "prototype", &prototype);
    if (result != FVM_OK)
        return result;
    if (prototype.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *object = 0;
    result = fvm_object_create(c, &object);
    if (result != FVM_OK)
        return result;

    /* Computation / Вычисление и обновление состояния. */
    object->resource_kind = STRING_OBJECT;
    object->resource_result_valid = true;
    object->resource_result = text;
    object->prototype = prototype.object;
    attach_length(object);
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    out->type = FVM_VALUE_OBJECT;
    out->object = object;
    return FVM_OK;
}
} // namespace
/* Compute and return string case. */
FVM_Result fvm_string_case(FVM_Context *c, const FVM_Value &text, FVM_Value *out, bool upper)
{
    try
    {
        return case_text(c, text, out, upper);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Build constructor and prototype for string. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
fvm_string_stage(FVM_Context *c, fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *klass = 0, *prototype = 0;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result result = fvm_object_create(c, &klass);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (result != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return result;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    result = fvm_object_create(c, &prototype);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (result != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return result;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    klass->resource_kind = STRING_CLASS;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    prototype->resource_kind = STRING_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    prototype->resource_result_valid = true;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_string(&prototype->resource_result, "");
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    attach_length(prototype);

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value object;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_undefined(&object);

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    object.type = FVM_VALUE_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    object.object = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    globals["String"] = object;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    prototype->properties["constructor"] = object;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    object.object = prototype;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    klass->properties["prototype"] = object;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeEntry entry;
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        entry.callback = methods[i].callback;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        prototype->methods[methods[i].name] = entry;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    entry.callback = from_char_code;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    klass->methods["fromCharCode"] = entry;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    entry.callback = constructor;
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    entry.root = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    entry.userdata = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    constructors["String"] = entry;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Validate string. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result fvm_string_validate(FVM_Context *c)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value klass, prototype;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (fvm_get_global(c, "String", &klass) != FVM_OK || klass.type != FVM_VALUE_OBJECT ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        klass.object->resource_kind != STRING_CLASS ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_object_get(c, klass.object, "prototype", &prototype) != FVM_OK ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        prototype.type != FVM_VALUE_OBJECT || prototype.object->resource_kind != STRING_OBJECT)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->constructors.find("String");
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (ctor == c->constructors.end() || ctor->second.callback != constructor ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        ctor->second.root != klass.object || ctor->second.userdata != klass.object)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeEntry entry;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (fvm_lookup_native_method(prototype.object, methods[i].name, &entry) != FVM_OK ||
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            entry.callback != methods[i].callback)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeEntry static_method;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (fvm_lookup_native_method(klass.object, "fromCharCode", &static_method) != FVM_OK ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        static_method.callback != from_char_code)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor =
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        prototype.object->accessors.find("length");
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (accessor == prototype.object->accessors.end() || accessor->second.getter != length ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        accessor->second.setter)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Map a native string callback to its stable serialization identifier. */
unsigned long fvm_string_callback_id(FVM_NativeFunction callback)
{
    if (callback == constructor)
        return 503;
    if (callback == length)
        return 501;
    if (callback == from_char_code)
        return 502;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
        if (callback == methods[i].callback)
            return 510 + (unsigned long)i;
    return 0;
}

/* Restore a native string callback from its stable serialization identifier. */
FVM_NativeFunction fvm_string_callback_from_id(unsigned long id)
{
    if (id == 503)
        return constructor;
    if (id == 501)
        return length;
    if (id == 502)
        return from_char_code;
    return id >= 510 && id < 510 + sizeof(methods) / sizeof(methods[0]) ? methods[id - 510].callback
                                                                        : 0;
}
