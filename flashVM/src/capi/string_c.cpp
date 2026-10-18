/* FlashVM: C embedding API: string.
 * File: src/capi/string_c.cpp.
 */

#include "../core/runtime.hpp"
#include "../core/string_internal.hpp"
#include "flashvm/standard.h"
#include <new>
#include <cstring>
/* Compute and return value string bytes. */
extern "C" FVM_Result fvm_value_string_bytes(FVM_Value *out, const char *text, size_t size)
{
    if (!out || (!text && size) || size == (size_t)-1)
        return FVM_INVALID_ARGUMENT;
    size_t at = 0;
    while (at < size)
    {
        unsigned long cp = 0;
        FVM_Result r = fvm_string_read(text, size, at, cp);
        if (r != FVM_OK)
            return r;
    }
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    out->type = FVM_VALUE_STRING;
    out->string = text ? text : "";
    out->string_size = size;
    return FVM_OK;
}

/* Compute and return value string size. */
extern "C" size_t fvm_value_string_size(const FVM_Value *value)
{
    return value && value->type == FVM_VALUE_STRING && value->string ? fvm_string_size(*value) : 0;
}

/* Compute and return value string utf16. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_value_string_utf16(
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Context *c, const unsigned short *units,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    size_t length, FVM_Value *out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!out)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_undefined(out);
    /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
    if (!c || c->destroying || c->gc_releasing || (!units && length) ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        length > ((size_t)-1 - 1) / 3)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::vector<char> bytes;
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (size_t i = 0; i < length; ++i)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            unsigned long cp = units[i];
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (cp > 65535)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_INVALID_ARGUMENT;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (cp >= 0xd800 && cp <= 0xdbff && i + 1 < length && units[i + 1] >= 0xdc00 &&
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                units[i + 1] <= 0xdfff)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                cp = 0x10000 + ((cp - 0xd800) << 10) + units[++i] - 0xdc00;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Result r = fvm_string_append(cp, bytes);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (r != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return r;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        size_t size = bytes.size();
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        bytes.push_back(0);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value temporary;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Result r = fvm_value_string_bytes(&temporary, bytes.begin(), size);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r == FVM_OK ? fvm_value_to_string(c, &temporary, out) : r;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    catch (const std::bad_alloc &)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OUT_OF_MEMORY;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    catch (...)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_ERROR;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Query or copy the UTF-16 code units represented by this string value. */
extern "C" FVM_Result fvm_value_get_utf16(const FVM_Value *value, unsigned short *out,
                                          size_t capacity, size_t *length)
{
    if (!value || !length || (!out && capacity))
        return FVM_INVALID_ARGUMENT;
    *length = 0;
    try
    {
        /* Validate before checking capacity; malformed input publishes no partial count.
         * Short buffers are rejected without allocating or writing any output. */
        FVM_Result r = fvm_string_unit_count(*value, *length);
        if (r != FVM_OK || (!out && !capacity))
            return r;
        if (capacity < *length)
            return FVM_INVALID_ARGUMENT;
        fvm_detail::vector<unsigned int> units;
        /* The validated size is exact: retain one overlap-safe snapshot allocation. */
        units.reserve(*length);
        r = fvm_string_units(*value, units);
        if (r != FVM_OK)
            return r;
        for (size_t i = 0; i < units.size(); ++i)
            out[i] = (unsigned short)units[i];
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
