/* FlashVM: Built-in ActionScript objects: string conversion.
 * File: src/standard/string_conversion.cpp.
 */

#include "primitive_state.hpp"
#include "../avm1/number_text.hpp"
#include "../core/string_internal.hpp"
#include "../core/runtime.hpp"
#include "flashvm/standard.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <clocale>
#include <new>
#include <limits>

namespace
{
/* Compute and return owned string. */
FVM_Result owned_string(FVM_Context *c, const char *text, FVM_Value *out,
                        size_t length = (size_t)-1)
{
    if (length == (size_t)-1)
        length = std::strlen(text);
    char *copy = new (std::nothrow) char[length + 1];
    if (!copy)
        return FVM_OUT_OF_MEMORY;

    /* Computation / Вычисление и обновление состояния. */
    std::memcpy(copy, text, length);
    copy[length] = 0;
    try
    {
        /* [GC:] Register owned bytes and their counted length together; failed insertion leaves
         * caller cleanup active. */
        fvm_context_adopt_string(c, copy, length);
    }
    catch (...)
    {
        delete[] copy;
        return FVM_OUT_OF_MEMORY;
    }
    return fvm_value_string_bytes(out, copy, length);
}

/* Compute and return number string. */
FVM_Result number_string(FVM_Context *c, double number, FVM_Value *out)
{
    if (c->swf_loaded)
        return fvm_avm1_number_text(c, number, out);
    if (number != number)
        return fvm_value_string(out, "NaN");
    if (number == 0)
        return fvm_value_string(out, "0");
    if (number == std::numeric_limits<double>::infinity() ||
        number == -std::numeric_limits<double>::infinity())
        return fvm_value_string(out, number < 0 ? "-Infinity" : "Infinity");

    /* Initialization / Инициализация локального состояния. */
    char candidate[128];
    const char *point = std::localeconv()->decimal_point;
    if (!point || std::strlen(point) > 16)
        return FVM_UNSUPPORTED;
    /* Probe increasing significant precision until libc parses the same double.
     * This is a bounded decimal conversion, not a complete Flash dtoa oracle.
     * Neither conversion changes the process locale. Hosts must not race
     * setlocale against VM execution. The output below always uses ASCII '.'. */
    bool roundtrip = false;
    for (int precision = 1; precision <= 17; ++precision)
    {
        std::sprintf(candidate, "%.*g", precision, number);
        char *end = 0;
        double parsed = std::strtod(candidate, &end);
        if (end && !*end && parsed == number)
        {
            roundtrip = true;
            break;
        }
    }
    if (!roundtrip)
        return FVM_UNSUPPORTED;

    /* Initialization / Инициализация локального состояния. */
    const char *p = candidate;
    bool negative = *p == '-';
    if (negative)
        ++p;

    /* Initialization / Инициализация локального состояния. */
    char digits[32];
    size_t count = 0;
    int position = 0;
    bool fraction = false;
    while (*p && *p != 'e' && *p != 'E')
    {
        if (*p >= '0' && *p <= '9')
        {
            if (count >= sizeof(digits) - 1)
                return FVM_ERROR;
            digits[count++] = *p;
            if (!fraction)
                ++position;
        }
        else
            fraction = true; /* Single- or multibyte locale decimal separator. */
        ++p;
    }
    if (*p)
    {
        ++p;
        bool minus = *p == '-';
        if (*p == '+' || *p == '-')
            ++p;
        int exponent = 0;
        while (*p >= '0' && *p <= '9')
            exponent = exponent * 10 + (*p++ - '0');
        if (*p || exponent > 400)
            return FVM_ERROR;
        position += minus ? -exponent : exponent;
    }
    if (!count)
        return FVM_ERROR;
    while (count > 1 && digits[0] == '0')
    {
        std::memmove(digits, digits + 1, --count);
        --position;
    }
    while (count > 1 && digits[count - 1] == '0')
        --count;

    /* Initialization / Инициализация локального состояния. */
    /* AS decimal notation uses fixed form from 1e-6 through values below 1e21.
     * Bounds include the largest fixed expansion plus a terminator. */
    char text[128];
    size_t used = 0;
    if (negative)
        text[used++] = '-';
    if (position > 0 && position <= 21)
    {
        for (int i = 0; i < position; ++i)
            text[used++] = (size_t)i < count ? digits[i] : '0';
        if ((size_t)position < count)
        {
            text[used++] = '.';
            for (size_t i = (size_t)position; i < count; ++i)
                text[used++] = digits[i];
        }
    }
    else if (position <= 0 && position > -6)
    {
        text[used++] = '0';
        text[used++] = '.';
        for (int i = 0; i < -position; ++i)
            text[used++] = '0';
        for (size_t i = 0; i < count; ++i)
            text[used++] = digits[i];
    }
    else
    {
        text[used++] = digits[0];
        if (count > 1)
        {
            text[used++] = '.';
            for (size_t i = 1; i < count; ++i)
                text[used++] = digits[i];
        }
        text[used++] = 'e';
        int exponent = position - 1;
        text[used++] = exponent < 0 ? '-' : '+';
        if (exponent < 0)
            exponent = -exponent;
        std::sprintf(text + used, "%d", exponent);
        return owned_string(c, text, out);
    }
    text[used] = 0;
    return owned_string(c, text, out);
}

/* Compute and return primitive string. */
FVM_Result primitive_string(FVM_Context *c, const FVM_Value &value, FVM_Value *out)
{
    switch (value.type)
    {
    case FVM_VALUE_UNDEFINED:
        return fvm_value_string(out, "undefined");
    case FVM_VALUE_NULL:
        return fvm_value_string(out, "null");
    case FVM_VALUE_BOOLEAN:
        return fvm_value_string(out, value.boolean ? "true" : "false");
    case FVM_VALUE_NUMBER:
        return number_string(c, value.number, out);
    case FVM_VALUE_STRING:
        return owned_string(c, value.string, out, fvm_string_size(value));
    default:
        return FVM_INVALID_ARGUMENT;
    }
}

/* Define ConversionScope struct and its representation. */
/* [GC:] Объявляем представление состояния сборщика или корней. */
struct ConversionScope
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Context *context;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::vector<FVM_Value> values;
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_GCRootFrame frame;
    /* Initialize Conversion Scope and its owned state. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    ConversionScope(FVM_Context *c, const FVM_Value &value) : context(c)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        values.push_back(value);
        /* [GC:] Сохраняем предыдущий корень для восстановления цепочки после выхода. */
        frame.previous = c->gc_frames;
        /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
        frame.stack = &values;
        /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
        frame.locals = 0;
        /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
        frame.receiver = 0;
        /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен быть LIFO. */
        c->gc_frames = &frame;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        ++c->string_depth;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Release the resources owned by this ConversionScope instance. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    ~ConversionScope()
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        --context->string_depth;
        /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен быть LIFO. */
        context->gc_frames = frame.previous;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
};
} // namespace

/* Compute and return value to string. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_value_to_string(FVM_Context *c, const FVM_Value *input, FVM_Value *out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!out)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    /* Permit in-place conversion without erasing the input before validation. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value value;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_undefined(&value);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (input)
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        value = *input;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_undefined(out);
    /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
    if (!c || !input || c->destroying || c->gc_releasing || fvm_validate_value(c, &value) != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (c->string_depth >= 64)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_UNSUPPORTED;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (value.type != FVM_VALUE_OBJECT)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return primitive_string(c, value, out);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        ConversionScope scope(c, value);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value primitive;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Result status = fvm_to_primitive(c, &value, true, &primitive);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return status == FVM_OK ? primitive_string(c, primitive, out) : status;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    catch (...)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OUT_OF_MEMORY;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}
