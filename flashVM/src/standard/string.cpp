/* FlashVM: Built-in ActionScript objects: string.
 * File: src/standard/string.cpp.
 */

#include "string_state.hpp"
#include "constructor_identity.hpp"
#include "string_constants.h"
#include "../core/unicode_constants.h"
#include "../core/numeric_constants.h"
#include "attribute_stage.hpp"
#include "../core/string_internal.hpp"
#include "../core/execution_work_scope.hpp"
#include "../core/object_root_scope.hpp"
#include "../../external/unicode/case_table.hpp"
#include "../../external/flash_case/case_table.hpp"
#include "../avm1/coercion.hpp"
#include "base_state.hpp"
#include "flashvm/table.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/utf8.h"
#include <cmath>
#include <cstring>
#include <limits>
namespace
{
/* Define anonymous enum and its representation. */
enum
{
    STRING_OBJECT = FVM_RESOURCE_STRING,
    STRING_CLASS = FVM_RESOURCE_STRING_CLASS,
    STRING_GENERIC_PROTOTYPE =
        FVM_RESOURCE_OBJECT /* Loaded prototype is an ordinary Object, without native text. */
};
typedef fvm_detail::vector<unsigned int> Units;

/* Decode a counted string into UTF16 units, including lone surrogates. */
FVM_Result decode(const FVM_Value &text, Units &units)
{
    return fvm_string_units(text, units);
}

/* Encode counted units, optionally charging native work before each emitted UTF-16 unit. */
FVM_Result encode(FVM_Context *c, const Units &units, size_t start, size_t end, FVM_Value *out,
                  FVM_ExecutionWorkScope *work = 0)
{
    fvm_detail::vector<char> text;
    for (size_t i = start; i < end; ++i)
    {
        if (work)
        {
            FVM_Result status = work->step();
            if (status != FVM_OK)
                return status;
        }
        unsigned long cp = units[i];
        if (cp >= FVM_UTF16_HIGH_SURROGATE_BASE && cp <= FVM_UTF16_HIGH_SURROGATE_MAX &&
            i + 1 < end && units[i + 1] >= FVM_UTF16_LOW_SURROGATE_BASE &&
            units[i + 1] <= FVM_UTF16_LOW_SURROGATE_MAX)
        {
            if (work)
            {
                FVM_Result status = work->step();
                if (status != FVM_OK)
                    return status;
            }
            cp = FVM_UTF16_SUPPLEMENTARY_OFFSET +
                 ((cp - FVM_UTF16_HIGH_SURROGATE_BASE) << FVM_UTF16_PAIR_PAYLOAD_BITS) +
                 units[++i] - FVM_UTF16_LOW_SURROGATE_BASE;
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

/* Return a boxed String's stored value; loaded calls on other objects yield undefined. */
FVM_Result value(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *, FVM_Value *out,
                 void *)
{
    if (!self || self->owner != c)
        return FVM_INVALID_ARGUMENT;
    if (self->resource_kind != STRING_OBJECT)
    {
        fvm_value_undefined(out);
        return c->swf_loaded ? FVM_OK : FVM_INVALID_ARGUMENT;
    }
    if (!self->resource_result_valid || self->resource_result.type != FVM_VALUE_STRING)
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

/* Wrap a finite numeric index through AVM1 signed Int32, truncating toward zero. */
double loaded_index(double number)
{
    if (number != number || std::fabs(number) == std::numeric_limits<double>::infinity())
        return 0;
    number = number < 0 ? std::ceil(number) : std::floor(number);
    number = std::fmod(number, FVM_UINT32_MODULUS);
    if (number < 0)
        number += FVM_UINT32_MODULUS;
    return number >= (-FVM_INT32_MIN_NUMBER) ? number - FVM_UINT32_MODULUS : number;
}

/* Coerce a present index using loaded AVM1 rules or the unloaded host contract. */
FVM_Result argument(FVM_Context *c, unsigned int count, const FVM_Value *args, unsigned int index,
                    double fallback, double &number)
{
    if (index >= count)
    {
        number = fallback;
        return FVM_OK;
    }
    FVM_Result result = c->swf_loaded ? fvm_avm1_number(c, args + index, &number)
                                      : fvm_value_to_number(c, args + index, &number);
    if (result != FVM_OK)
        return result;
    if (c->swf_loaded)
    {
        number = loaded_index(number);
        return FVM_OK;
    }
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
    size_t unit_count = 0;
    result = fvm_string_unit_count(text, unit_count);
    if (result == FVM_OK)
        fvm_value_number(out, (double)unit_count);
    return result;
}

/* Compute and return character. */
FVM_Result character(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args,
                     FVM_Value *out, bool code)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_ExecutionWorkScope work(c, c->swf_loaded);
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
            fvm_value_number(out,
                             c->swf_loaded && c->swf_info.version == FVM_SWF_VERSION_5 && index >= 0
                                 ? 0
                                 : std::numeric_limits<double>::quiet_NaN());
        else
            fvm_value_string(out, "");
        return FVM_OK;
    }
    size_t i = (size_t)index;
    if (code)
    {
        result = work.step();
        if (result != FVM_OK)
            return result;
        fvm_value_number(out, units[i]);
        return FVM_OK;
    }
    return encode(c, units, i, i + 1, out, &work);
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
    /* Loaded slice/substr/substring reject missing arguments before receiver coercion. */
    if (c->swf_loaded && !count)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }

    /* Keep receiver and argument callbacks inside the same loaded execution budget. */
    FVM_ExecutionWorkScope work(c, c->swf_loaded);
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
        end = c->swf_loaded ? clamp(loaded_index((double)start + last), units.size(), true)
                            : start + clamp(last, units.size() - start, false);
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
    return encode(c, units, start, end, out, &work);
}

/* Compute and return substring. */
FVM_Result substring(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a,
                     FVM_Value *o, void *)
{
    return portion(c, s, n, a, o, 0);
}

/* Copy the selected String range using versioned UTF-16 indices. */
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
    FVM_ExecutionWorkScope work(c, c->swf_loaded);
    Units haystack, needle;
    FVM_Result result = receiver(c, self, haystack);
    if (result != FVM_OK)
        return result;
    /* Unlike portions, missing search arguments are checked after receiver coercion. */
    if (c->swf_loaded && !count)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }

    /* Initialization / Инициализация локального состояния. */
    FVM_Value absent, text;
    fvm_value_undefined(&absent);
    result = c->swf_loaded ? fvm_avm1_string(c, args[0], &text)
                           : fvm_value_to_string(c, count ? args : &absent, &text);
    if (result != FVM_OK)
        return result;
    result = decode(text, needle);
    if (result != FVM_OK)
        return result;
    double position = backwards ? (double)haystack.size() : 0;
    if (count > 1 && (!c->swf_loaded || args[1].type != FVM_VALUE_UNDEFINED))
    {
        result = c->swf_loaded ? fvm_avm1_number(c, args + 1, &position)
                               : fvm_value_to_number(c, args + 1, &position);
        if (result != FVM_OK)
            return result;
        if (c->swf_loaded)
            position = loaded_index(position);
        else if (position != position)
            position = backwards ? (double)haystack.size() : 0;
        else
            position = position < 0 ? -std::floor(-position) : std::floor(position);
    }
    /* Preserve the reference's rejected ranges rather than clamping them to an endpoint. */
    if (c->swf_loaded &&
        ((backwards && position < 0) || (!backwards && position > (double)haystack.size())))
    {
        fvm_value_number(out, -1);
        return FVM_OK;
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
        result = work.step();
        if (result != FVM_OK)
            return result;
        bool equal = true;
        for (size_t j = 0; j < needle.size(); ++j)
        {
            result = work.step();
            if (result != FVM_OK)
                return result;
            if (haystack[i + j] != needle[j])
            {
                equal = false;
                break;
            }
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

/* Capture the receiver, then append versioned string conversions of arguments in order. */
FVM_Result concat(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args,
                  FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_ExecutionWorkScope work(c, c->swf_loaded);
    Units joined;
    FVM_Result result = receiver(c, self, joined);
    if (result != FVM_OK)
        return result;
    for (unsigned int i = 0; i < count; ++i)
    {
        result = work.step();
        if (result != FVM_OK)
            return result;
        FVM_Value text;
        result = c->swf_loaded ? fvm_avm1_string(c, args[i], &text)
                               : fvm_value_to_string(c, args + i, &text);
        if (result != FVM_OK)
            return result;
        result = decode(text, joined);
        if (result != FVM_OK)
            return result;
    }
    return encode(c, joined, 0, joined.size(), out, &work);
}

/* Resolve from char code. */
FVM_Result from_char_code(FVM_Context *c, FVM_Object *, unsigned int count, const FVM_Value *args,
                          FVM_Value *out, void *)
{
    FVM_ExecutionWorkScope work(c, c->swf_loaded);
    Units units;
    for (unsigned int i = 0; i < count; ++i)
    {
        FVM_Result result = work.step();
        if (result != FVM_OK)
            return result;
        double number = 0;
        result = argument(c, count, args, i, 0, number);
        if (result != FVM_OK)
            return result;
        if (std::fabs(number) == std::numeric_limits<double>::infinity())
            number = 0;
        number = std::fmod(number, FVM_UTF16_UNIT_MODULUS);
        if (number < 0)
            number += FVM_UTF16_UNIT_MODULUS;
        units.push_back((unsigned int)number);
    }
    return encode(c, units, 0, units.size(), out, &work);
}

/* Compute and return split. */
FVM_Result split(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *a,
                 FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_ExecutionWorkScope work(c, c->swf_loaded);
    Units input, separator;
    FVM_Result r = receiver(c, self, input);
    if (r != FVM_OK)
        return r;
    unsigned long limit = FVM_UINT32_MASK;
    if (n > 1 && a[1].type != FVM_VALUE_UNDEFINED)
    {
        double value = 0;
        r = c->swf_loaded ? fvm_avm1_number(c, a + 1, &value)
                          : fvm_value_to_number(c, a + 1, &value);
        if (r != FVM_OK)
            return r;
        if (c->swf_loaded)
        {
            value = loaded_index(value);
            limit = value > 0 ? (unsigned long)value : 0;
        }
        else
        {
            if (!(value == value) || value == std::numeric_limits<double>::infinity() ||
                value == -std::numeric_limits<double>::infinity())
                value = 0;
            value =
                std::fmod(value < 0 ? -std::floor(-value) : std::floor(value), FVM_UINT32_MODULUS);
            if (value < 0)
                value += FVM_UINT32_MODULUS;
            limit = (unsigned long)value;
        }
    }
    bool absent = !n || a[0].type == FVM_VALUE_UNDEFINED;
    if (absent && c->swf_loaded && c->swf_info.version == FVM_SWF_VERSION_5)
    {
        /* SWF5 defaults to comma, while its empty explicit delimiter returns the whole string. */
        separator.push_back(',');
        absent = false;
    }
    else if (!absent)
    {
        FVM_Value text;
        r = c->swf_loaded ? fvm_avm1_string(c, a[0], &text) : fvm_value_to_string(c, a, &text);
        if (r == FVM_OK)
            r = decode(text, separator);
        if (r != FVM_OK)
            return r;
        if (c->swf_loaded && c->swf_info.version == FVM_SWF_VERSION_5 && separator.empty())
            absent = true;
    }
    /* A missing effective delimiter returns one whole value even when limit is zero. */
    if (c->swf_loaded && absent)
        limit = FVM_UINT32_MASK;
    FVM_Object *result = 0;
    r = fvm_array_create_like(c, 0, 0, &result, true);
    if (r != FVM_OK)
        return r;

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Retain the unfinished result until all charged copies and own stores complete. */
    FVM_ObjectRootScope roots(c, result);
    unsigned long count = 0;
    size_t start = 0;
    /* Walk UTF-16 boundaries, retaining empty fields for nonempty separators. */
    for (size_t position = 0; count < limit && position <= input.size();)
    {
        r = work.step();
        if (r != FVM_OK)
            return r;
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
            {
                r = work.step();
                if (r != FVM_OK)
                    return r;
                if (separator[j] != input[position + j])
                {
                    matched = false;
                    break;
                }
            }
        }
        if (!matched)
        {
            ++position;
            continue;
        }
        if (!c->swf_loaded && count >= FVM_STRING_HOST_SPLIT_MAX_FIELDS)
            return FVM_OUT_OF_MEMORY;
        FVM_Value part;
        r = encode(c, input, start, end, &part, &work);
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
FVM_Result case_text(FVM_Context *c, const FVM_Value &text, FVM_Value *out, bool upper,
                     FVM_ExecutionWorkScope *work = 0)
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
            /* Public loaded methods charge each mapping, then each output unit. */
            if (work && (r = work->step()) != FVM_OK)
                return r;
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
        return encode(c, units, 0, units.size(), out, work);
    }
    fvm_detail::vector<unsigned long> scalars;
    for (size_t i = 0; i < units.size(); ++i)
    {
        unsigned long cp = units[i];
        if (cp >= FVM_UTF16_HIGH_SURROGATE_BASE && cp <= FVM_UTF16_HIGH_SURROGATE_MAX &&
            i + 1 < units.size() && units[i + 1] >= FVM_UTF16_LOW_SURROGATE_BASE &&
            units[i + 1] <= FVM_UTF16_LOW_SURROGATE_MAX)
            cp = FVM_UTF16_SUPPLEMENTARY_OFFSET +
                 ((cp - FVM_UTF16_HIGH_SURROGATE_BASE) << FVM_UTF16_PAIR_PAYLOAD_BITS) +
                 units[++i] - FVM_UTF16_LOW_SURROGATE_BASE;
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
        if (!upper && cp == FVM_UNICODE_CAPITAL_SIGMA && preceding_cased)
        {
            size_t next = i + 1;
            while (next < scalars.size() && in_ranges(scalars[next], fvm_case_ignored))
                ++next;
            final_sigma = next == scalars.size() || !in_ranges(scalars[next], fvm_case_cased);
        }
        if (final_sigma)
            r = fvm_string_append(FVM_UNICODE_FINAL_SIGMA, bytes);
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
    FVM_ExecutionWorkScope work(c, c->swf_loaded);
    Units units;
    FVM_Result r = receiver(c, o, units);
    FVM_Value text;
    if (r == FVM_OK)
        r = encode(c, units, 0, units.size(), &text);
    return r == FVM_OK ? case_text(c, text, out, false, &work) : r;
}

/* Compute and return upper case. */
FVM_Result upper_case(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *,
                      FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_ExecutionWorkScope work(c, c->swf_loaded);
    Units units;
    FVM_Result r = receiver(c, o, units);
    FVM_Value text;
    if (r == FVM_OK)
        r = encode(c, units, 0, units.size(), &text);
    return r == FVM_OK ? case_text(c, text, out, true, &work) : r;
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

/* Prepare a loaded String receiver before coercion; preserve host lookup order. */
FVM_Result constructor(FVM_Context *c, FVM_Object *, unsigned int count, const FVM_Value *args,
                       FVM_Value *out, void *userdata)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value text, prototype;
    FVM_Object *object = 0;
    FVM_Result result = FVM_OK;
    if (c->swf_loaded)
    {
        result = fvm_function_allocate_instance(c, (FVM_Object *)userdata, STRING_OBJECT, &object);
        if (result != FVM_OK)
            return result;
    }
    /* [GC:] toString can detach the constructor while the selected receiver is pending. */
    FVM_ObjectRootScope object_root(c, object);
    if (count)
        result = c->swf_loaded ? fvm_avm1_string(c, args[0], &text)
                                : fvm_value_to_string(c, args, &text);
    else
        result = fvm_value_string(&text, "");
    if (result != FVM_OK)
        return result;
    size_t unit_count = 0;
    result = fvm_string_unit_count(text, unit_count);
    if (result != FVM_OK)
        return result;
    if (!c->swf_loaded)
    {
        result = fvm_object_get(c, (FVM_Object *)userdata, "prototype", &prototype);
        if (result != FVM_OK)
            return result;
        if (prototype.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        result = fvm_object_create(c, &object);
        if (result != FVM_OK)
            return result;
        object->resource_kind = STRING_OBJECT;
        object->prototype = prototype.object;
    }
    object->resource_result_valid = true;
    object->resource_result = text;
    if (c->swf_loaded)
    {
        /* Boxed String length is own data; changing it does not change stored text. */
        FVM_Value count_value;
        fvm_value_number(&count_value, (double)unit_count);
        object->properties["length"] = count_value;
        object->attributes["length"] = FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE;
    }
    else
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
    prototype->resource_kind = c->swf_loaded ? STRING_GENERIC_PROTOTYPE : STRING_OBJECT;
    if (!c->swf_loaded)
    {
        /* [GC:] Unloaded host prototypes retain the original boxed empty text. */
        prototype->resource_result_valid = true;
        fvm_value_string(&prototype->resource_result, "");
        attach_length(prototype);
    }

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
        if (c->swf_loaded)
            prototype->attributes[methods[i].name] = 3;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    entry.callback = from_char_code;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    klass->methods["fromCharCode"] = entry;
    if (c->swf_loaded)
        klass->attributes["fromCharCode"] = 3;
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
        prototype.type != FVM_VALUE_OBJECT ||
        (prototype.object->resource_kind != STRING_OBJECT &&
         prototype.object->resource_kind != STRING_GENERIC_PROTOTYPE))
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
    if (prototype.object->resource_kind == STRING_OBJECT &&
        (accessor == prototype.object->accessors.end() || accessor->second.getter != length ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
         accessor->second.setter))
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* [GC:] Constructor capture retains the class while its static flags are staged.
 * Public prototype replacement must not disable an untouched fromCharCode method. */
void fvm_string_prepare_class_loaded(FVM_Context *c, FVM_MethodAttributeActivation &out)
{
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor = c->constructors.find("String");
    if (ctor == c->constructors.end() || ctor->second.callback != constructor || !ctor->second.root)
        return;
    FVM_Object *klass = ctor->second.root;
    out.attributes = klass->attributes;
    fvm_stage_default_method_flags(klass, "fromCharCode", from_char_code,
                                  FVM_PROPERTY_BUILTIN_FLAGS, out.attributes);
    out.prototype = klass;
}

/* [GC:] Prepare per-method flags before optionally dropping default native text/length.
 * Class roots keep the candidate alive; this operation performs no user callbacks. */
FVM_Result fvm_string_activate_loaded(FVM_Context *c)
{
    if (!c->standard_library)
        return FVM_OK;
    try
    {
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor = c->constructors.find("String");
        if (ctor == c->constructors.end() || ctor->second.callback != constructor || !ctor->second.root)
            return FVM_OK;
        FVM_Object *klass = ctor->second.root;
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator stored = klass->properties.find("prototype");
        if (stored == klass->properties.end() || stored->second.type != FVM_VALUE_OBJECT)
            return FVM_OK;
        FVM_Object *proto = stored->second.object;
        if (!proto || proto->resource_kind != STRING_OBJECT)
            return FVM_OK;
        fvm_detail::map<fvm_detail::string, unsigned int> prototype_attributes = proto->attributes;
        for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
            fvm_stage_default_method_flags(proto, methods[i].name, methods[i].callback,
                                          FVM_PROPERTY_BUILTIN_FLAGS, prototype_attributes);
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method = proto->methods.find("charAt");
        fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor = proto->accessors.find("length");
        fvm_detail::map<fvm_detail::string, unsigned int>::iterator flags = proto->attributes.find("length");
        const bool activate_layout = proto->resource_result_valid &&
            proto->resource_result.type == FVM_VALUE_STRING && !fvm_string_size(proto->resource_result) &&
            method != proto->methods.end() && method->second.callback == char_at &&
            accessor != proto->accessors.end() && accessor->second.getter == length && !accessor->second.setter &&
            proto->properties.find("length") == proto->properties.end() &&
            (flags == proto->attributes.end() || !flags->second);
        if (activate_layout && flags != proto->attributes.end())
            prototype_attributes.erase(flags->first);
        /* All fallible work precedes both the independent flags and optional layout commit. */
        if (activate_layout)
        {
            proto->accessors.erase(accessor->first);
            if (flags != proto->attributes.end())
                proto->attributes.erase(flags->first);
        }
        proto->attributes.swap(prototype_attributes);
        if (activate_layout)
        {
            proto->resource_kind = STRING_GENERIC_PROTOTYPE;
            proto->resource_result_valid = false;
            fvm_value_undefined(&proto->resource_result);
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Map a native string callback to its stable serialization identifier. */
unsigned long fvm_string_callback_id(FVM_NativeFunction callback)
{
    if (callback == constructor)
        return FVM_STRING_CALLBACK_CONSTRUCTOR;
    if (callback == length)
        return FVM_STRING_CALLBACK_LENGTH;
    if (callback == from_char_code)
        return FVM_STRING_CALLBACK_FROM_CHAR_CODE;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
        if (callback == methods[i].callback)
            return FVM_STRING_CALLBACK_METHOD_BASE + (unsigned long)i;
    return 0;
}

/* Restore a native string callback from its stable serialization identifier. */
FVM_NativeFunction fvm_string_callback_from_id(unsigned long id)
{
    if (id == FVM_STRING_CALLBACK_CONSTRUCTOR)
        return constructor;
    if (id == FVM_STRING_CALLBACK_LENGTH)
        return length;
    if (id == FVM_STRING_CALLBACK_FROM_CHAR_CODE)
        return from_char_code;
    return id >= FVM_STRING_CALLBACK_METHOD_BASE &&
                   id < FVM_STRING_CALLBACK_METHOD_BASE + sizeof(methods) / sizeof(methods[0])
               ? methods[id - FVM_STRING_CALLBACK_METHOD_BASE].callback
               : 0;
}
