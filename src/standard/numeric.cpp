/* FlashVM: Built-in ActionScript objects: numeric.
 * File: src/standard/numeric.cpp.
 */

#include "../core/string_internal.hpp"
#include "primitive_state.hpp"
#include "movieclip_state.hpp"
#include "geometry_state.hpp"
#include "function_state.hpp"
#include "date_state.hpp"
#include "base_state.hpp"
#include "string_state.hpp"
#include "global_parsing.hpp"
#include "number_state.hpp"
#include "error_state.hpp"
#include "../core/runtime.hpp"
#include "flashvm/standard.h"
#include <cmath>
#include <limits>
#include <cstring>

namespace
{
/* Define anonymous enum and its representation. */
enum
{
    BOOLEAN_OBJECT = 20,
    BOOLEAN_CLASS = 21
};

/* Construct a quiet not-a-number value using the portable numeric helper. */
double nan_value()
{
    return std::numeric_limits<double>::quiet_NaN();
}

/* Construct signed infinity using the portable numeric helper. */
double infinity()
{
    return std::numeric_limits<double>::infinity();
}

/* Recognize the whitespace accepted by numeric string conversion. */
bool whitespace(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}

/* Decode an ASCII hexadecimal digit, or report an invalid character. */
int hex_digit(unsigned char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}
/* This bounded parser accepts decimal/hexadecimal primitive numeric strings.
 * It deliberately does not delegate syntax or decimal separators to locale. */
/* Parse number. */
double parse_number(const char *text)
{
    const unsigned char *p = (const unsigned char *)text;
    while (whitespace(*p))
        ++p;
    if (!*p)
        return 0;
    bool negative = false, sign = false;
    if (*p == '+' || *p == '-')
    {
        sign = true;
        negative = *p == '-';
        ++p;
    }
    if (std::strncmp((const char *)p, "Infinity", 8) == 0)
    {
        p += 8;
        while (whitespace(*p))
            ++p;
        return *p ? nan_value() : (negative ? -infinity() : infinity());
    }
    if (!sign && p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
    {
        p += 2;
        double value = 0;
        unsigned int digits = 0;
        while (hex_digit(*p) >= 0)
        {
            value = value * 16 + hex_digit(*p++);
            ++digits;
        }
        while (whitespace(*p))
            ++p;
        return !digits || *p ? nan_value() : value;
    }

    /* Initialization / Инициализация локального состояния. */
    double value = 0;
    unsigned int significant = 0;
    long fraction = 0, dropped = 0;
    bool digits = false, dot = false;
    while ((*p >= '0' && *p <= '9') || (!dot && *p == '.'))
    {
        if (*p == '.')
        {
            dot = true;
            ++p;
            continue;
        }
        digits = true;
        if (dot && fraction < 100000)
            ++fraction;
        unsigned int digit = *p++ - '0';
        if (!significant && digit == 0)
            continue;
        if (significant < 19)
        {
            value = value * 10 + digit;
            ++significant;
        }
        else if (dropped < 100000)
            ++dropped;
    }
    if (!digits)
        return nan_value();
    long exponent = 0;
    if (*p == 'e' || *p == 'E')
    {
        ++p;
        bool minus = false;
        if (*p == '+' || *p == '-')
        {
            minus = *p == '-';
            ++p;
        }
        if (*p < '0' || *p > '9')
            return nan_value();
        while (*p >= '0' && *p <= '9')
        {
            if (exponent < 100000)
                exponent = exponent * 10 + (*p - '0');
            if (exponent > 100000)
                exponent = 100000;
            ++p;
        }
        if (minus)
            exponent = -exponent;
    }
    while (whitespace(*p))
        ++p;
    if (*p)
        return nan_value();
    exponent += dropped - fraction;
    if (value != 0)
    {
        if (exponent > 1000)
            value = infinity();
        else if (exponent < -1000)
            value = 0;
        else
        {
            /* Split scaling so pow itself cannot underflow before multiplication. */
            while (exponent > 300)
            {
                value *= 1e300;
                exponent -= 300;
            }
            while (exponent < -300)
            {
                value *= 1e-300;
                exponent += 300;
            }
            value *= std::pow(10.0, (double)exponent);
        }
    }
    return negative ? -value : value;
}

/* Detect a zero with the IEEE negative sign bit. */
bool negative_zero(double value)
{
    return value == 0 && 1.0 / value < 0;
}

/* Compute and return argument. */
FVM_Result argument(FVM_Context *c, unsigned int count, const FVM_Value *args, unsigned int index,
                    double *out)
{
    if (index >= count)
    {
        *out = nan_value();
        return FVM_OK;
    }
    return fvm_value_to_number(c, &args[index], out);
}

/* Define Operation enum and its representation. */
enum Operation
{
    ABS,
    ACOS,
    ASIN,
    ATAN,
    ATAN2,
    CEIL,
    COS,
    EXP,
    FLOOR,
    LOG,
    MAX,
    MIN,
    POW,
    RANDOM,
    ROUND,
    SIN,
    SQRT,
    TAN
};

/* Coerce arguments and execute the selected built-in Math operation. */
FVM_Result math_operation(FVM_Context *c, unsigned int count, const FVM_Value *args, FVM_Value *out,
                          Operation op)
{
    double x = 0, y = 0, result = 0;
    if (op == RANDOM)
    {
        /* Context-local 32-bit LCG; no shared libc rand state between workers. */
        c->random_state = (c->random_state * 1664525UL + 1013904223UL) & 0xffffffffUL;
        fvm_value_number(out, (double)c->random_state / 4294967296.0);
        return FVM_OK;
    }
    FVM_Result status = argument(c, count, args, 0, &x);
    if (status != FVM_OK)
        return status;
    if (op == MAX || op == MIN)
    {
        result = op == MAX ? -infinity() : infinity();
        for (unsigned int i = 0; i < count; ++i)
        {
            status = argument(c, count, args, i, &x);
            if (status != FVM_OK)
                return status;
            if (x != x)
            {
                result = x;
                break;
            }
            if ((op == MAX && x > result) || (op == MIN && x < result) ||
                (x == 0 && result == 0 && (op == MAX ? !negative_zero(x) : negative_zero(x))))
                result = x;
        }
    }
    else if (op == POW || op == ATAN2)
    {
        status = argument(c, count, args, 1, &y);
        if (status != FVM_OK)
            return status;
        if (op == ATAN2)
            result = std::atan2(x, y);
        else if (y == 0)
            result = 1;
        else if (std::fabs(x) == 1 && std::fabs(y) == infinity())
            result = nan_value();
        else
            result = std::pow(x, y);
    }
    else
    {
        switch (op)
        {
        case ABS:
            result = std::fabs(x);
            break;
        case ACOS:
            result = std::acos(x);
            break;
        case ASIN:
            result = std::asin(x);
            break;
        case ATAN:
            result = std::atan(x);
            break;
        case CEIL:
            result = std::ceil(x);
            break;
        case COS:
            result = std::cos(x);
            break;
        case EXP:
            result = std::exp(x);
            break;
        case FLOOR:
            result = std::floor(x);
            break;
        case LOG:
            result = std::log(x);
            break;
        case ROUND:
            if (x != x || std::fabs(x) >= 4503599627370496.0 || x == 0)
                result = x;
            else if (x < 0 && x >= -0.5)
                result = -0.0;
            else
                result = std::floor(x) + ((x - std::floor(x)) >= 0.5 ? 1.0 : 0.0);
            break;
        case SIN:
            result = std::sin(x);
            break;
        case SQRT:
            result = std::sqrt(x);
            break;
        case TAN:
            result = std::tan(x);
            break;
        default:
            return FVM_ERROR;
        }
    }
    fvm_value_number(out, result);
    return FVM_OK;
}
#define MATH_WRAPPER(name, operation)                                                              \
    FVM_Result name(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,              \
                    FVM_Value *o, void *)                                                          \
    {                                                                                              \
        return math_operation(c, n, a, o, operation);                                              \
    }
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_abs, ABS)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_acos, ACOS)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_asin, ASIN)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_atan, ATAN)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_atan2, ATAN2)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_ceil, CEIL)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_cos, COS)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_exp, EXP)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_floor, FLOOR)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_log, LOG)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_max, MAX)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_min, MIN)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_pow, POW)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_random, RANDOM)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_round, ROUND)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_sin, SIN)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_sqrt, SQRT)
/* Dispatch this Math operation through the shared numeric implementation. */
MATH_WRAPPER(math_tan, TAN)
#undef MATH_WRAPPER
/* Define Method struct and its representation. */
struct Method
{
    const char *name;
    FVM_NativeFunction callback;
};
const Method math_methods[] = {{"abs", math_abs},   {"acos", math_acos},     {"asin", math_asin},
                               {"atan", math_atan}, {"atan2", math_atan2},   {"ceil", math_ceil},
                               {"cos", math_cos},   {"exp", math_exp},       {"floor", math_floor},
                               {"log", math_log},   {"max", math_max},       {"min", math_min},
                               {"pow", math_pow},   {"random", math_random}, {"round", math_round},
                               {"sin", math_sin},   {"sqrt", math_sqrt},     {"tan", math_tan}};

/* Implement the Number() global conversion call. */
FVM_Result number_call(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                       FVM_Value *o, void *)
{

    /* Initialization / Инициализация локального состояния. */
    double number = 0;
    FVM_Result result = n ? fvm_value_to_number(c, &a[0], &number) : FVM_OK;
    if (result == FVM_OK)
        fvm_value_number(o, number);
    return result;
}

/* Implement the Boolean() global conversion call. */
FVM_Result boolean_call(FVM_Context *, FVM_Object *, unsigned int n, const FVM_Value *a,
                        FVM_Value *o, void *)
{
    bool truth = false;
    if (n)
    {
        const FVM_Value &v = a[0];
        truth = (v.type == FVM_VALUE_BOOLEAN && v.boolean) ||
                (v.type == FVM_VALUE_NUMBER && v.number != 0 && v.number == v.number) ||
                (v.type == FVM_VALUE_STRING && fvm_string_size(v)) || v.type == FVM_VALUE_OBJECT;
    }
    fvm_value_boolean(o, truth);
    return FVM_OK;
}

/* Unbox the receiver for Boolean.prototype.valueOf. */
FVM_Result boolean_value(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *)
{
    if (!self || self->owner != c || self->resource_kind != BOOLEAN_OBJECT ||
        !self->resource_result_valid || self->resource_result.type != FVM_VALUE_BOOLEAN)
        return FVM_INVALID_ARGUMENT;
    *out = self->resource_result;
    return FVM_OK;
}

/* Format a Boolean receiver as the strings true or false. */
FVM_Result boolean_string(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args,
                          FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value value;
    FVM_Result result = boolean_value(c, self, n, args, &value, 0);
    if (result != FVM_OK)
        return result;
    return fvm_value_string(out, value.boolean ? "true" : "false");
}

/* Create a boxed Boolean instance with its native value state. */
FVM_Result boolean_constructor(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *args,
                               FVM_Value *out, void *userdata)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *klass = (FVM_Object *)userdata;
    FVM_Value prototype, value;
    FVM_Result result = fvm_object_get(c, klass, "prototype", &prototype);
    if (result != FVM_OK)
        return result;
    if (prototype.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    result = boolean_call(c, 0, n, args, &value, 0);
    if (result != FVM_OK)
        return result;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *object = 0;
    result = fvm_object_create(c, &object);
    if (result != FVM_OK)
        return result;

    /* Computation / Вычисление и обновление состояния. */
    /* The wrapped primitive is inaccessible to ordinary property assignments.
     * Prototype links and the constructor capture are traced by the existing GC. */
    object->resource_kind = BOOLEAN_OBJECT;
    object->resource_result = value;
    object->resource_result_valid = true;
    object->prototype = prototype.object;
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    out->type = FVM_VALUE_OBJECT;
    out->object = object;
    return FVM_OK;
}

/* Coerce the argument to Number and test whether it is NaN. */
FVM_Result isnan_call(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                      FVM_Value *o, void *)
{

    /* Initialization / Инициализация локального состояния. */
    double number;
    FVM_Result result = argument(c, n, a, 0, &number);
    if (result == FVM_OK)
        fvm_value_boolean(o, number != number);
    return result;
}

/* Coerce the argument to Number and test whether it is finite. */
FVM_Result isfinite_call(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                         FVM_Value *o, void *)
{

    /* Initialization / Инициализация локального состояния. */
    double number;
    FVM_Result result = argument(c, n, a, 0, &number);
    if (result == FVM_OK)
        fvm_value_boolean(o, number == number && std::fabs(number) != infinity());
    return result;
}
const Method global_functions[] = {{"Number", number_call},
                                   {"Boolean", boolean_call},
                                   {"isNaN", isnan_call},
                                   {"isFinite", isfinite_call},
                                   {"parseInt", fvm_builtin_parse_int},
                                   {"parseFloat", fvm_builtin_parse_float},
                                   {"String", fvm_builtin_string}};
} // namespace
/* Map a native numeric callback to its stable serialization identifier. */
unsigned long fvm_numeric_callback_id(FVM_NativeFunction callback)
{
    if (callback == boolean_value)
        return 1;
    if (callback == boolean_string)
        return 2;
    for (size_t i = 0; i < sizeof(math_methods) / sizeof(math_methods[0]); ++i)
        if (callback == math_methods[i].callback)
            return 100 + (unsigned long)i;
    if (callback == boolean_constructor)
        return 3;
    for (size_t i = 0; i < sizeof(global_functions) / sizeof(global_functions[0]); ++i)
        if (callback == global_functions[i].callback)
            return 150 + (unsigned long)i;
    return 0;
}

/* Restore a native numeric callback from its stable serialization identifier. */
FVM_NativeFunction fvm_numeric_callback_from_id(unsigned long id)
{
    if (id == 3)
        return boolean_constructor;
    if (id >= 150 && id < 150 + sizeof(global_functions) / sizeof(global_functions[0]))
        return global_functions[id - 150].callback;
    if (id == 1)
        return boolean_value;
    if (id == 2)
        return boolean_string;
    if (id >= 100 && id < 100 + sizeof(math_methods) / sizeof(math_methods[0]))
        return math_methods[id - 100].callback;
    return 0;
}

/* Coerce an ActionScript value to Number using its primitive representation. */
extern "C" FVM_Result fvm_value_to_number(FVM_Context *c, const FVM_Value *value, double *out)
{
    if (!c || !value || !out)
        return FVM_INVALID_ARGUMENT;
    if (fvm_validate_value(c, value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    switch (value->type)
    {
    case FVM_VALUE_UNDEFINED:
        *out = nan_value();
        return FVM_OK;
    case FVM_VALUE_NULL:
        *out = 0;
        return FVM_OK;
    case FVM_VALUE_BOOLEAN:
        *out = value->boolean ? 1 : 0;
        return FVM_OK;
    case FVM_VALUE_NUMBER:
        *out = value->number;
        return FVM_OK;
    case FVM_VALUE_STRING:
        if (!value->string)
            return FVM_INVALID_ARGUMENT;
        /* Number consumes the entire string; an embedded NUL is not a terminator. */
        {
            size_t n = fvm_string_size(*value);
            if (std::memchr(value->string, 0, n))
                *out = nan_value();
            else
            {
                try
                {
                    fvm_detail::string terminated(value->string, n);
                    *out = parse_number(terminated.c_str());
                }
                catch (const std::bad_alloc &)
                {
                    return FVM_OUT_OF_MEMORY;
                }
            }
        }
        return FVM_OK;
    case FVM_VALUE_OBJECT:
    {
        FVM_Value primitive;
        FVM_Result r = fvm_to_primitive(c, value, false, &primitive);
        return r == FVM_OK ? fvm_value_to_number(c, &primitive, out) : r;
    }
    default:
        return FVM_INVALID_ARGUMENT;
    }
}

/* Install the supported standard constructors and global numeric functions. */
extern "C" FVM_Result fvm_standard_install(FVM_Context *c)
{
    /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
    if (!c || c->destroying || c->gc_collecting)
        return FVM_INVALID_ARGUMENT;
    try
    {
        const char *names[] = {"Math",
                               "Number",
                               "Boolean",
                               "isNaN",
                               "isFinite",
                               "NaN",
                               "Infinity",
                               "Error",
                               "parseInt",
                               "parseFloat",
                               "String",
                               "Object",
                               "Array",
                               "ASSetPropFlags",
                               "Date",
                               "Function",
                               "MovieClip",
                               "flash",
                               "flash.geom.Point",
                               "flash.geom.Matrix",
                               "flash.geom.Rectangle",
                               "flash.geom.ColorTransform"};
        if (c->standard_library)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value math;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Result result = fvm_get_global(c, "Math", &math);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (result != FVM_OK || math.type != FVM_VALUE_OBJECT ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                math.object->resource_kind != 10)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_INVALID_ARGUMENT;
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (size_t i = 0; i < sizeof(math_methods) / sizeof(math_methods[0]); ++i)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator it =
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    math.object->methods.find(math_methods[i].name);
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (it == math.object->methods.end() ||
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    it->second.callback != math_methods[i].callback)
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return FVM_INVALID_ARGUMENT;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (size_t i = 0; i < sizeof(global_functions) / sizeof(global_functions[0]); ++i)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator it =
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    c->functions.find(global_functions[i].name);
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (it == c->functions.end() || it->second.callback != global_functions[i].callback)
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return FVM_INVALID_ARGUMENT;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value boolean_class, prototype;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (fvm_get_global(c, "Boolean", &boolean_class) != FVM_OK ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                boolean_class.type != FVM_VALUE_OBJECT ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                boolean_class.object->resource_kind != BOOLEAN_CLASS ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_object_get(c, boolean_class.object, "prototype", &prototype) != FVM_OK ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                prototype.type != FVM_VALUE_OBJECT ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                prototype.object->resource_kind != BOOLEAN_OBJECT)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_INVALID_ARGUMENT;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                c->constructors.find("Boolean");
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (ctor == c->constructors.end() || ctor->second.callback != boolean_constructor ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                ctor->second.root != boolean_class.object)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_INVALID_ARGUMENT;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_NativeEntry method;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (fvm_lookup_native_method(prototype.object, "valueOf", &method) != FVM_OK ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                method.callback != boolean_value ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_lookup_native_method(prototype.object, "toString", &method) != FVM_OK ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                method.callback != boolean_string)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_INVALID_ARGUMENT;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Result error_result = fvm_error_validate(c);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (error_result != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return error_result;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            error_result = fvm_number_validate(c);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (error_result != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return error_result;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            error_result = fvm_string_validate(c);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (error_result != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return error_result;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            error_result = fvm_base_validate(c);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (error_result != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return error_result;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            error_result = fvm_date_validate(c);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (error_result != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return error_result;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            error_result = fvm_function_validate(c);
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            if (error_result == FVM_OK)
                error_result = fvm_movieclip_validate(c);
            return error_result == FVM_OK ? fvm_geometry_validate(c) : error_result;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
            if (c->global->properties.find(names[i]) != c->global->properties.end() ||
                c->global->accessors.find(names[i]) != c->global->accessors.end() ||
                c->constructors.find(names[i]) != c->constructors.end() ||
                c->functions.find(names[i]) != c->functions.end() ||
                c->stack_functions.find(names[i]) != c->stack_functions.end() ||
                c->script_functions.find(names[i]) != c->script_functions.end())
                return FVM_INVALID_ARGUMENT;
        /* Build both maps before publishing, so allocation errors cannot leave
         * partially installed globals and function registrations. */
        fvm_detail::map<fvm_detail::string, FVM_Value> globals = c->global->properties;
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry> functions = c->functions;
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry> constructors = c->constructors;
        FVM_Object *math = 0;
        FVM_Result result = fvm_object_create(c, &math);
        if (result != FVM_OK)
            return result;
        math->resource_kind = 10;
        for (size_t i = 0; i < sizeof(math_methods) / sizeof(math_methods[0]); ++i)
        {
            FVM_NativeEntry entry;
            entry.callback = math_methods[i].callback;
            math->methods[math_methods[i].name] = entry;
        }
        const char *constants[] = {"E", "LN10", "LN2", "LOG10E", "LOG2E", "PI", "SQRT1_2", "SQRT2"};
        const double values[] = {std::exp(1.0),      std::log(10.0),    std::log(2.0),
                                 1 / std::log(10.0), 1 / std::log(2.0), std::acos(-1.0),
                                 std::sqrt(0.5),     std::sqrt(2.0)};
        for (size_t i = 0; i < 8; ++i)
        {
            FVM_Value value;
            fvm_value_number(&value, values[i]);
            math->properties[constants[i]] = value;
        }
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = math;
        globals["Math"] = value;
        fvm_value_number(&value, nan_value());
        globals["NaN"] = value;
        fvm_value_number(&value, infinity());
        globals["Infinity"] = value;
        for (size_t i = 0; i < sizeof(global_functions) / sizeof(global_functions[0]); ++i)
        {
            FVM_NativeEntry entry;
            entry.callback = global_functions[i].callback;
            functions[global_functions[i].name] = entry;
        }
        FVM_Object *boolean_class = 0, *prototype = 0;
        result = fvm_object_create(c, &boolean_class);
        if (result != FVM_OK)
            return result;
        result = fvm_object_create(c, &prototype);
        if (result != FVM_OK)
            return result;
        boolean_class->resource_kind = BOOLEAN_CLASS;
        prototype->resource_kind = BOOLEAN_OBJECT;
        prototype->resource_result_valid = true;
        fvm_value_boolean(&prototype->resource_result, 0);
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = prototype;
        boolean_class->properties["prototype"] = value;
        value.object = boolean_class;
        prototype->properties["constructor"] = value;
        globals["Boolean"] = value;
        FVM_NativeEntry constructor, method;
        constructor.callback = boolean_constructor;
        constructor.userdata = boolean_class;
        /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
        constructor.root = boolean_class;
        constructors["Boolean"] = constructor;
        method.callback = boolean_value;
        prototype->methods["valueOf"] = method;
        method.callback = boolean_string;
        prototype->methods["toString"] = method;
        result = fvm_error_stage(c, globals, constructors);
        if (result != FVM_OK)
            return result;
        result = fvm_number_stage(c, globals, constructors);
        if (result != FVM_OK)
            return result;
        result = fvm_string_stage(c, globals, constructors);
        if (result != FVM_OK)
            return result;
        result = fvm_date_stage(c, globals, constructors);
        if (result != FVM_OK)
            return result;
        result = fvm_function_stage(c, globals, constructors);
        if (result != FVM_OK)
            return result;
        result = fvm_movieclip_stage(c, globals, constructors);
        if (result != FVM_OK)
            return result;
        result = fvm_base_stage(c, globals, constructors, functions);
        if (result != FVM_OK)
            return result;
        /* [GC:] Stage geometry namespace/captures before standard globals are published. */
        result = fvm_geometry_stage(c, globals, constructors);
        if (result != FVM_OK)
            return result;
        FVM_Object *function_proto = globals["Function"].object->properties["prototype"].object;
        /* Function.call/apply retain their actual boxing classes in the graph.
         * A restored function can therefore box this without global installation. */
        FVM_Object *boxing = 0;
        result = fvm_object_create(c, &boxing);
        if (result != FVM_OK)
            return result;
        const char *box_names[] = {"Number", "Boolean", "String"};
        for (size_t i = 0; i < 3; ++i)
            boxing->properties[box_names[i]] = globals[box_names[i]];
        const char *call_names[] = {"call", "apply"};
        for (size_t i = 0; i < 2; ++i)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_NativeEntry &entry = function_proto->methods.find(call_names[i])->second;
            /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
            entry.root = boxing;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            entry.userdata = boxing;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }

        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator i =
                 constructors.begin();
             i != constructors.end(); ++i)
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (i->second.root &&
                (i->first == "Object" || i->first == "Array" || i->first == "Boolean" ||
                 i->first == "Number" || i->first == "String" || i->first == "Error" ||
                 i->first == "Date" || i->first == "Function" || i->first == "MovieClip"))
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Object *klass = i->second.root;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                klass->constructable = i->second;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                klass->invocable = i->second;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                klass->prototype = function_proto;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator i = functions.begin();
             i != functions.end(); ++i)
        {
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator g = globals.find(i->first);
            if ((i->first == "Number" || i->first == "Boolean" || i->first == "String" ||
                 i->first == "Object" || i->first == "Array") &&
                g != globals.end() && g->second.type == FVM_VALUE_OBJECT &&
                g->second.object->constructable.callback)
                g->second.object->invocable = i->second;
        }
        /* Prepare all lookups before publication: map key construction may
         * allocate. Post-publication links below are pointer assignments only. */
        FVM_Object *text_classes[2] = {0, 0}, *text_prototypes[2] = {0, 0};
        const char *text_names[] = {"TextFormat", "TextField"};
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *object_proto =
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            constructors.find("Object")->second.root->properties.find("prototype")->second.object;
        for (size_t i = 0; i < 2; ++i)
        {
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator text =
                globals.find(text_names[i]);
            if (text != globals.end() && text->second.type == FVM_VALUE_OBJECT &&
                text->second.object->resource_kind == (i == 0 ? 1003 : 1004))
            {
                text_classes[i] = text->second.object;
                fvm_detail::map<fvm_detail::string, FVM_Value>::iterator prototype =
                    text_classes[i]->properties.find("prototype");
                if (prototype != text_classes[i]->properties.end() &&
                    prototype->second.type == FVM_VALUE_OBJECT)
                    text_prototypes[i] = prototype->second.object;
            }
        }
        c->global->properties.swap(globals);
        c->functions.swap(functions);
        c->constructors.swap(constructors);
        c->standard_library = true;
        for (size_t i = 0; i < 2; ++i)
        {
            if (text_classes[i] && !text_classes[i]->prototype)
                text_classes[i]->prototype = function_proto;
            if (text_prototypes[i] && !text_prototypes[i]->prototype)
                text_prototypes[i]->prototype = object_proto;
        }
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
