/* FlashVM: Built-in ActionScript objects: number.
 * File: src/standard/number.cpp.
 */

#include "number_state.hpp"
#include "constructor_identity.hpp"
#include "../core/object_root_scope.hpp"
#include "number_radix_constants.h"
#include "../core/numeric_constants.h"
#include "../avm1/coercion.hpp"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include <cmath>
#include <limits>

namespace
{
/* Define anonymous enum and its representation. */
enum
{
    NUMBER_OBJECT = FVM_RESOURCE_NUMBER,
    NUMBER_CLASS = FVM_RESOURCE_NUMBER_CLASS,
    NUMBER_GENERIC_PROTOTYPE = FVM_RESOURCE_OBJECT /* Ordinary loaded prototype has no boxed primitive. */
};

/* Read stored Number data; loaded value methods do not coerce other receivers. */
FVM_Result number_value(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *,
                        FVM_Value *out, void *)
{
    if (!self || self->owner != c)
        return FVM_INVALID_ARGUMENT;
    if (self->resource_kind != NUMBER_OBJECT)
    {
        fvm_value_undefined(out);
        return c->swf_loaded ? FVM_OK : FVM_INVALID_ARGUMENT;
    }
    if (!self->resource_result_valid || self->resource_result.type != FVM_VALUE_NUMBER)
        return FVM_INVALID_ARGUMENT;
    *out = self->resource_result;
    return FVM_OK;
}

/* Format a finite double exactly in a power-of-two radix without host integer-width assumptions. */
FVM_Result binary_radix(FVM_Context *c, double value, unsigned int radix, FVM_Value *out)
{
    const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    /* A double needs at most 1024 binary integer digits or 1074 fractional digits. */
    char integer[1076], text[2156];
    size_t position = sizeof(integer), used = 0;
    double magnitude = value < 0 ? -value : value;
    double whole = std::floor(magnitude), fraction = magnitude - whole;
    do
    {
        double quotient = std::floor(whole / radix);
        unsigned int digit = (unsigned int)(whole - quotient * radix);
        if (digit >= radix || !position)
            return FVM_ERROR;
        integer[--position] = digits[digit];
        whole = quotient;
    } while (whole != 0);
    if (value < 0)
        text[used++] = '-';
    while (position < sizeof(integer))
        text[used++] = integer[position++];
    if (fraction)
    {
        text[used++] = '.';
        while (fraction)
        {
            fraction *= radix;
            unsigned int digit = (unsigned int)fraction;
            if (digit >= radix || used >= sizeof(text) - 1)
                return FVM_ERROR;
            text[used++] = digits[digit];
            fraction -= digit;
        }
    }
    text[used] = 0;
    FVM_Value temporary;
    FVM_Result result = fvm_value_string_bytes(&temporary, text, used);
    return result == FVM_OK ? fvm_value_to_string(c, &temporary, out) : result;
}

/* Reproduce loaded Int32/sentinel digit formatting without signed overflow or negative division.
 * Out-of-range/NaN values select INT32_MIN; its wrapped negation remains negative,
 * producing the pinned AVM1 policy's punctuation/control bytes rather than a magnitude. */
FVM_Result loaded_radix(FVM_Context *c, double value, unsigned int radix, FVM_Value *out)
{
    double whole = FVM_INT32_MIN_NUMBER;
    if (value >= FVM_INT32_MIN_NUMBER && value <= FVM_INT32_MAX_NUMBER)
        whole = value < 0 ? std::ceil(value) : std::floor(value);
    bool negative = whole < 0;
    if (negative && whole != FVM_INT32_MIN_NUMBER)
        whole = -whole;

    const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    char buffer[FVM_NUMBER_RADIX_TEXT_CAPACITY];
    size_t position = sizeof(buffer) - 1;
    buffer[position] = 0;
    do
    {
        double quotient = whole < 0 ? -std::floor(-whole / radix) : std::floor(whole / radix);
        long remainder = (long)(whole - quotient * radix);
        if (!position)
            return FVM_ERROR;
        buffer[--position] = remainder < 0 ? (char)(FVM_NUMBER_RADIX_ASCII_ZERO + remainder)
                                          : digits[(unsigned int)remainder];
        whole = quotient;
    } while (whole != 0);
    if (negative)
        buffer[--position] = '-';

    FVM_Value text;
    FVM_Result status = fvm_value_string_bytes(&text, buffer + position,
                                               sizeof(buffer) - position - 1);
    return status == FVM_OK ? fvm_value_to_string(c, &text, out) : status;
}

/* Format stored Number data; loaded radix coercion defaults invalid values to decimal. */
FVM_Result number_string(FVM_Context *c, FVM_Object *self, unsigned int count,
                         const FVM_Value *args, FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value primitive;
    FVM_Result result = number_value(c, self, 0, 0, &primitive, 0);
    if (result != FVM_OK)
        return result;
    if (primitive.type == FVM_VALUE_UNDEFINED)
    {
        *out = primitive;
        return FVM_OK;
    }
    double radix = 10;
    if (count && (c->swf_loaded || args[0].type != FVM_VALUE_UNDEFINED))
    {
        result = c->swf_loaded ? fvm_avm1_number(c, &args[0], &radix)
                               : fvm_value_to_number(c, &args[0], &radix);
        if (result != FVM_OK)
            return result;
        if (radix != radix || radix < 2 || radix >= 37)
        {
            if (!c->swf_loaded)
                return FVM_INVALID_ARGUMENT;
            radix = 10;
        }
        /* Valid radices are positive: floor matches truncation without an unsafe cast.
         * Out-of-range values fall back rather than wrapping through ToInt32. */
        radix = std::floor(radix);
    }
    if (radix == 10)
        return fvm_value_to_string(c, &primitive, out);
    double value = primitive.number;
    if (c->swf_loaded)
        return loaded_radix(c, value, (unsigned int)radix, out);
    if (value != value || value == std::numeric_limits<double>::infinity() ||
        value == -std::numeric_limits<double>::infinity())
        return fvm_value_to_string(c, &primitive, out);
    if (radix == 2 || radix == 4 || radix == 8 || radix == 16 || radix == 32)
        return binary_radix(c, value, (unsigned int)radix, out);
    /* Other nondecimal radices retain the bounded exact-integer path. Their recurring
     * fractional expansions need a separately verified rounding algorithm. */
    double magnitude = value < 0 ? -value : value;
    if (magnitude > 9007199254740991.0 || magnitude != std::floor(magnitude))
        return FVM_UNSUPPORTED;

    /* Initialization / Инициализация локального состояния. */
    const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    char buffer[64];
    unsigned int position = sizeof(buffer) - 1;
    buffer[position] = 0;
    do
    {
        double quotient = std::floor(magnitude / radix);
        unsigned int remainder = (unsigned int)(magnitude - quotient * radix);
        if (remainder >= (unsigned int)radix || !position)
            return FVM_ERROR;
        buffer[--position] = digits[remainder];
        magnitude = quotient;
    } while (magnitude != 0);
    if (value < 0)
        buffer[--position] = '-';

    /* Initialization / Инициализация локального состояния. */
    FVM_Value text;
    fvm_value_string(&text, buffer + position);
    return fvm_value_to_string(c, &text, out);
}

/* Construct boxed Number data with the loaded SWF numeric conversion rules. */
FVM_Result number_constructor(FVM_Context *c, FVM_Object *, unsigned int count,
                              const FVM_Value *args, FVM_Value *out, void *userdata)
{

    FVM_Object *object = 0;
    FVM_Result result = FVM_OK;
    if (c->swf_loaded)
    {
        result = fvm_function_allocate_instance(c, (FVM_Object *)userdata, NUMBER_OBJECT, &object);
        if (result != FVM_OK)
            return result;
    }
    /* [GC:] valueOf may detach the class and collect before boxed data is installed. */
    FVM_ObjectRootScope object_root(c, object);
    double value = 0;
    result = count ? fvm_avm1_number(c, &args[0], &value) : FVM_OK;
    if (result != FVM_OK)
        return result;
    if (!c->swf_loaded)
    {
        FVM_Value prototype;
        result = fvm_object_get(c, (FVM_Object *)userdata, "prototype", &prototype);
        if (result != FVM_OK)
            return result;
        if (prototype.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        result = fvm_object_create(c, &object);
        if (result != FVM_OK)
            return result;
        object->resource_kind = NUMBER_OBJECT;
        object->prototype = prototype.object;
    }
    object->resource_result_valid = true;

    /* Initialization / Инициализация локального состояния. */
    fvm_value_number(&object->resource_result, value);
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    out->type = FVM_VALUE_OBJECT;
    out->object = object;
    return FVM_OK;
}
} // namespace
/* Build constructor and prototype for number. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result
fvm_number_stage(FVM_Context *c, fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
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
    klass->resource_kind = NUMBER_CLASS;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    prototype->resource_kind = c->swf_loaded ? NUMBER_GENERIC_PROTOTYPE : NUMBER_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    prototype->resource_result_valid = !c->swf_loaded;

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    if (c->swf_loaded)
        fvm_value_undefined(&prototype->resource_result);
    else
        fvm_value_number(&prototype->resource_result, 0);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value value;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_undefined(&value);

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    value.type = FVM_VALUE_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    value.object = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    globals["Number"] = value;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    prototype->properties["constructor"] = value;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    value.object = prototype;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    klass->properties["prototype"] = value;

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const char *names[] = {
        "MAX_VALUE", "MIN_VALUE", "NaN", "NEGATIVE_INFINITY",
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        "POSITIVE_INFINITY"};
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const double values[] = {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        std::numeric_limits<double>::max(), std::numeric_limits<double>::denorm_min(),
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        std::numeric_limits<double>::quiet_NaN(), -std::numeric_limits<double>::infinity(),
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        std::numeric_limits<double>::infinity()};
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (unsigned int i = 0; i < 5; ++i)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_number(&value, values[i]);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        klass->properties[names[i]] = value;
        if (c->swf_loaded)
            klass->attributes[names[i]] = 7;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeEntry entry;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    entry.callback = number_value;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    prototype->methods["valueOf"] = entry;
    if (c->swf_loaded)
        prototype->attributes["valueOf"] = 3;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    entry.callback = number_string;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    prototype->methods["toString"] = entry;
    if (c->swf_loaded)
        prototype->attributes["toString"] = 3;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    entry.callback = number_constructor;
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    entry.root = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    entry.userdata = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    constructors["Number"] = entry;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Validate number. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result fvm_number_validate(FVM_Context *c)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value klass, prototype;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (fvm_get_global(c, "Number", &klass) != FVM_OK || klass.type != FVM_VALUE_OBJECT ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        klass.object->resource_kind != NUMBER_CLASS ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_object_get(c, klass.object, "prototype", &prototype) != FVM_OK ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        prototype.type != FVM_VALUE_OBJECT ||
        (prototype.object->resource_kind != NUMBER_OBJECT &&
         prototype.object->resource_kind != NUMBER_GENERIC_PROTOTYPE))
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->constructors.find("Number");
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (ctor == c->constructors.end() || ctor->second.callback != number_constructor ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        ctor->second.root != klass.object || ctor->second.userdata != klass.object)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeEntry method;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (fvm_lookup_native_method(prototype.object, "valueOf", &method) != FVM_OK ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        method.callback != number_value ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_lookup_native_method(prototype.object, "toString", &method) != FVM_OK ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        method.callback != number_string)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* [GC:] Prepare the unchanged zero-valued prototype and numeric attributes before
 * any mutation. Constructor capture retains both objects throughout preparation. */
void fvm_number_prepare_loaded(FVM_Context *c, FVM_NumberPrototypeActivation &out)
{
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor = c->constructors.find("Number");
    if (ctor == c->constructors.end() || ctor->second.callback != number_constructor || !ctor->second.root)
        return;
    FVM_Object *klass = ctor->second.root;
    /* Class constants are independent of a replaced or edited prototype. */
    out.class_attributes = klass->attributes;
    const char *constant_names[] = {"MAX_VALUE", "MIN_VALUE", "NaN", "NEGATIVE_INFINITY", "POSITIVE_INFINITY"};
    const double constants[] = {std::numeric_limits<double>::max(), std::numeric_limits<double>::denorm_min(),
                               std::numeric_limits<double>::quiet_NaN(), -std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::infinity()};
    for (unsigned int i = 0; i < sizeof(constant_names) / sizeof(constant_names[0]); ++i)
    {
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator stored = klass->properties.find(constant_names[i]);
        if (stored != klass->properties.end() && stored->second.type == FVM_VALUE_NUMBER && !stored->second.metatable &&
            (stored->second.number == constants[i] || (stored->second.number != stored->second.number && constants[i] != constants[i])) &&
            klass->attributes.find(constant_names[i]) == klass->attributes.end() &&
            klass->accessors.find(constant_names[i]) == klass->accessors.end())
            out.class_attributes[constant_names[i]] = FVM_PROPERTY_BUILTIN_FLAGS | FVM_PROPERTY_READ_ONLY;
    }
    out.klass = klass;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator stored = klass->properties.find("prototype");
    if (stored == klass->properties.end() || stored->second.type != FVM_VALUE_OBJECT)
        return;
    FVM_Object *proto = stored->second.object;
    if (!proto || proto->resource_kind != NUMBER_OBJECT || !proto->resource_result_valid ||
        proto->resource_result.type != FVM_VALUE_NUMBER || proto->resource_result.number != 0)
        return;
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator value = proto->methods.find("valueOf");
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator text = proto->methods.find("toString");
    if (value == proto->methods.end() || value->second.callback != number_value ||
        text == proto->methods.end() || text->second.callback != number_string)
        return;
    out.prototype_attributes = proto->attributes;
    const char *method_names[] = {"valueOf", "toString"};
    for (unsigned int i = 0; i < 2; ++i)
    {
        fvm_detail::map<fvm_detail::string, unsigned int>::iterator flags = out.prototype_attributes.find(method_names[i]);
        if (flags != out.prototype_attributes.end() && flags->second == FVM_PROPERTY_DONT_ENUM &&
            proto->properties.find(method_names[i]) == proto->properties.end() &&
            proto->accessors.find(method_names[i]) == proto->accessors.end())
            flags->second = FVM_PROPERTY_BUILTIN_FLAGS;
    }
    out.prototype = proto;
    out.klass = klass;
}

/* Commit the prepared Number native state and flag maps without a failing operation. */
void fvm_number_commit_loaded(FVM_NumberPrototypeActivation &prepared)
{
    if (prepared.klass)
        prepared.klass->attributes.swap(prepared.class_attributes);
    if (!prepared.prototype)
        return;
    prepared.prototype->resource_kind = NUMBER_GENERIC_PROTOTYPE;
    prepared.prototype->resource_result_valid = false;
    fvm_value_undefined(&prepared.prototype->resource_result);
    prepared.prototype->attributes.swap(prepared.prototype_attributes);
}

/* Map a native number callback to its stable serialization identifier. */
unsigned long fvm_number_callback_id(FVM_NativeFunction callback)
{
    return callback == number_value         ? 401UL
           : callback == number_string      ? 402UL
           : callback == number_constructor ? 403UL
                                            : 0UL;
}

/* Restore a native number callback from its stable serialization identifier. */
FVM_NativeFunction fvm_number_callback_from_id(unsigned long id)
{
    return id == 401   ? number_value
           : id == 402 ? number_string
           : id == 403 ? number_constructor
                       : 0;
}
