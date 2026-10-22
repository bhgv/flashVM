/* FlashVM: C embedding API: serialization.
 * File: src/capi/serialization_c.cpp.
 */

#include "graph_constants.h"
#include "../core/property_version_constants.h"
#include "../core/numeric_constants.h"
#include "../core/string_internal.hpp"
#include "../core/property_key.hpp"
#include "../standard/movieclip_state.hpp"
#include "../standard/geometry_state.hpp"
#include "../standard/color_transform_state.hpp"
#include "../standard/filter_state.hpp"
#include "../standard/blur.hpp"
#include "../standard/convolution.hpp"
#include "../standard/bitmap_state.hpp"
#include "../standard/function_state.hpp"
#include "../standard/date_state.hpp"
#include "../standard/base_state.hpp"
#include "../standard/broadcaster.hpp"
#include "../standard/url_encoding.hpp"
#include "../standard/timers.hpp"
#include "../standard/inheritance.hpp"
#include "flashvm/utf8.h"
#include "../standard/string_state.hpp"
#include "../standard/number_state.hpp"
#include "../standard/error_state.hpp"
#include "../core/runtime.hpp"
#include "../core/transfer_resource.hpp"
#include "../core/capability_internal.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/serialization.h"
#include "../standard/text_state.hpp"
#include "../standard/text_constants.h"
#include "zlib.h"
#include <cstring>
#include <cstdlib>
#include <climits>
#include <cmath>
#include <limits>
#include <new>

namespace
{
const size_t none = (size_t)-1;
const size_t max_bytes = 64UL * 1024 * 1024;
const size_t max_objects = 4096, max_entries = 65536;

/* Define Failure struct and its representation. */
struct Failure
{
    FVM_Result result;
    /* Initialize Failure and its owned state. */
    explicit Failure(FVM_Result r) : result(r)
    {
    }
};

/* Implement require for this subsystem. */
void require(bool condition, FVM_Result result = FVM_INVALID_ARGUMENT)
{
    if (!condition)
        throw Failure(result);
}

/* Define Value struct and its representation. */
struct Value
{
    FVM_ValueType type;
    double number;
    int boolean;
    fvm_detail::string string;
    size_t object, metatable;
    /* Initialize Value and its owned state. */
    Value() : type(FVM_VALUE_UNDEFINED), number(0), boolean(0), object(none), metatable(none)
    {
    }
};

/* Define Callback struct and its representation. */
/* [GC:] Объявляем представление состояния сборщика или корней. */
struct Callback
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    unsigned long builtin;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::string name;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    size_t root;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    bool root_userdata;
    /* Initialize Callback and its owned state. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    Callback() : builtin(0), root(none), root_userdata(false)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
};

/* Define Accessor struct and its representation. */
struct Accessor
{
    Callback getter, setter;
};

/* Define Object struct and its representation. */
struct Object
{
    unsigned long kind;
    Value state;
    FVM_TextFieldState text;
    size_t prototype, metatable;
    fvm_detail::vector<size_t>
        implemented_interfaces; /* [GC:] Captured class or prototype references, selected by mode. */
    bool interfaces_are_prototypes; /* True preserves installed snapshot lists, including empty ones. */
    fvm_detail::map<fvm_detail::string, Value> properties;
    fvm_detail::map<unsigned long, Value> indexed;
    fvm_detail::map<fvm_detail::string, Callback> methods, metamethods;
    fvm_detail::map<fvm_detail::string, unsigned int> attributes;
    Callback invocable, constructable;
    fvm_detail::map<fvm_detail::string, Value> method_values, watchers;
    fvm_detail::map<fvm_detail::string, Accessor> accessors;
    /* Initialize Object and its owned state. */
    Object() : kind(0), prototype(none), metatable(none), interfaces_are_prototypes(false)
    {
    }
};

/* Define Graph struct and its representation. */
struct Graph
{
    Value value;
    fvm_detail::vector<Object> objects;
};

/* Compute and return known kind. */
bool known_kind(unsigned long kind)
{
    return kind == FVM_GRAPH_REMOTE_FUNCTION || kind == FVM_GRAPH_REMOTE_CHANNEL || kind == FVM_GRAPH_REMOTE_MUTEX || kind == FVM_GRAPH_LOCAL_CHANNEL || kind == FVM_GRAPH_LOCAL_MUTEX || kind == FVM_RESOURCE_PLAIN_OBJECT ||
           kind == FVM_RESOURCE_MATH_NAMESPACE || kind == FVM_RESOURCE_BOOLEAN || kind == FVM_RESOURCE_BOOLEAN_CLASS || kind == FVM_RESOURCE_ERROR || kind == FVM_RESOURCE_ERROR_CLASS || kind == FVM_RESOURCE_NUMBER ||
           kind == FVM_RESOURCE_NUMBER_CLASS || kind == FVM_RESOURCE_STRING || kind == FVM_RESOURCE_STRING_CLASS || kind == FVM_RESOURCE_ARRAY || kind == FVM_RESOURCE_ARRAY_CLASS || kind == FVM_RESOURCE_OBJECT ||
           kind == FVM_RESOURCE_OBJECT_CLASS || kind == FVM_RESOURCE_DATE || kind == FVM_RESOURCE_DATE_CLASS || kind == FVM_RESOURCE_SUPER || kind == FVM_RESOURCE_WITH_SCOPE || kind == FVM_RESOURCE_SCRIPT_FUNCTION || kind == FVM_RESOURCE_FUNCTION_CLASS ||
           kind == FVM_RESOURCE_NATIVE_FUNCTION || kind == FVM_RESOURCE_MOVIECLIP || kind == FVM_RESOURCE_MOVIECLIP_CLASS || kind == FVM_RESOURCE_TEXT_FORMAT || kind == FVM_RESOURCE_TEXT_FORMAT_CLASS || kind == FVM_RESOURCE_TEXT_FIELD_CLASS ||
           kind == FVM_RESOURCE_TEXT_FIELD || kind == FVM_RESOURCE_POINT || kind == FVM_RESOURCE_MATRIX || kind == FVM_RESOURCE_POINT_CLASS || kind == FVM_RESOURCE_MATRIX_CLASS || kind == FVM_RESOURCE_RECTANGLE ||
           kind == FVM_RESOURCE_RECTANGLE_CLASS || kind == FVM_RESOURCE_COLOR_TRANSFORM || kind == FVM_RESOURCE_COLOR_TRANSFORM_CLASS || kind == FVM_RESOURCE_TRANSFORM || kind == FVM_RESOURCE_TRANSFORM_CLASS || kind == FVM_RESOURCE_BITMAP_DATA ||
           kind == FVM_RESOURCE_BITMAP_DATA_CLASS || kind == FVM_RESOURCE_COLOR_MATRIX_FILTER || kind == FVM_RESOURCE_COLOR_MATRIX_FILTER_CLASS || kind == FVM_RESOURCE_BITMAP_FILTER || kind == FVM_RESOURCE_BITMAP_FILTER_CLASS || kind == FVM_RESOURCE_BLUR_FILTER ||
           kind == FVM_RESOURCE_BLUR_FILTER_CLASS || kind == FVM_RESOURCE_CONVOLUTION_FILTER || kind == FVM_RESOURCE_CONVOLUTION_FILTER_CLASS;
}

/* Compute and return identify. */
size_t identify(FVM_Context *c, FVM_Object *object, fvm_detail::vector<FVM_Object *> &seen)
{
    if (!object)
        return none;
    require(c && object->owner == c);
    for (size_t i = 0; i < seen.size(); ++i)
        if (seen[i] == object)
            return i;

    /* Computation / Вычисление и обновление состояния. */
    require(seen.size() < max_objects, FVM_UNSUPPORTED);
    seen.push_back(object);
    return seen.size() - 1;
}

/* Implement charge for this subsystem. */
void charge(size_t &budget, size_t bytes)
{

    /* Computation / Вычисление и обновление состояния. */
    require(bytes <= max_bytes - budget, FVM_UNSUPPORTED);
    budget += bytes;
}

/* Capture value. */
Value capture_value(FVM_Context *c, const FVM_Value &src, fvm_detail::vector<FVM_Object *> &seen,
                    size_t &budget)
{
    bool valid = false;
    for (int i = FVM_VALUE_UNDEFINED; i <= FVM_VALUE_OBJECT; ++i)
    {
        FVM_ValueType candidate = (FVM_ValueType)i;
        if (!std::memcmp(&candidate, &src.type, sizeof(candidate)))
            valid = true;
    }

    /* Computation / Вычисление и обновление состояния. */
    require(valid);
    charge(budget, 13);
    Value dst;

    /* Computation / Вычисление и обновление состояния. */
    dst.type = src.type;
    require(!src.metatable || src.type != FVM_VALUE_OBJECT);
    dst.metatable = identify(c, src.metatable, seen);
    if (src.type == FVM_VALUE_NUMBER)
        dst.number = src.number;
    if (src.type == FVM_VALUE_BOOLEAN)
        dst.boolean = src.boolean != 0;
    if (src.type == FVM_VALUE_STRING)
    {
        require(src.string != 0);
        FVM_Value checked;
        require(fvm_value_string_bytes(&checked, src.string, fvm_string_size(src)) == FVM_OK);
        charge(budget, fvm_string_size(src));
        dst.string = fvm_detail::string(src.string, fvm_string_size(src));
    }
    if (src.type == FVM_VALUE_OBJECT)
    {
        require(src.object != 0);
        dst.object = identify(c, src.object, seen);
    }
    return dst;
}

/* Capture callback. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
Callback
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
capture_callback(FVM_Context *c, FVM_NativeFunction fn, void *userdata, FVM_Object *root,
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 fvm_detail::vector<FVM_Object *> &seen, size_t &budget)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    charge(budget, 13);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    Callback dst;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!fn)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        require(!userdata && !root);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return dst;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const char *selector = fvm_text_callback_selector(fn, userdata);

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    require(!userdata || selector || (root && userdata == root), FVM_UNSUPPORTED);
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    dst.root = identify(c, root, seen);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    dst.root_userdata = userdata != 0 && !selector;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    dst.builtin = fvm_numeric_callback_id(fn);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!dst.builtin)
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        dst.builtin = fvm_text_callback_id(fn);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!dst.builtin)
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        dst.builtin = fvm_error_callback_id(fn);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!dst.builtin)
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        dst.builtin = fvm_number_callback_id(fn);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!dst.builtin)
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        dst.builtin = fvm_string_callback_id(fn);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!dst.builtin)
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        dst.builtin = fvm_base_callback_id(fn);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!dst.builtin)
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        dst.builtin = fvm_date_callback_id(fn);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!dst.builtin)
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        dst.builtin = fvm_function_callback_id(fn);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!dst.builtin)
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        dst.builtin = fvm_movieclip_callback_id(fn);
    /* [GC:] Identify geometry factories whose class captures travel in the same object graph. */
    if (!dst.builtin)
        /* [GC:] Preserve the stable native callback identifier rather than a process address. */
        dst.builtin = fvm_geometry_callback_id(fn);
    /* [GC:] The live listener Array travels as the accessor capture, never native userdata bytes. */
    if (!dst.builtin)
        dst.builtin = fvm_broadcaster_callback_id(fn);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (selector)
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        dst.name = selector;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!dst.builtin)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (fvm_detail::map<fvm_detail::string, FVM_NativeFunction>::iterator i =
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
             c->serialization_callbacks.begin();
             /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
             i != c->serialization_callbacks.end(); ++i)
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (i->second == fn)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                charge(budget, i->first.size());
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                dst.name = i->first;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                break;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        require(!dst.name.empty(), FVM_UNSUPPORTED);
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return dst;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Capture the current value. */
Graph capture(FVM_Context *c, const FVM_Value &value)
{

    /* Initialization / Инициализация локального состояния. */
    Graph graph;
    fvm_detail::vector<FVM_Object *> seen;
    size_t entries = 0, budget = 32;
    graph.value = capture_value(c, value, seen, budget);
    for (size_t i = 0; i < seen.size(); ++i)
    {
        FVM_Object *src = seen[i];
        charge(budget, 40);
        Object dst;
        if (fvm_function_is_proxy(src) || (fvm_function_transfer_enabled(c) &&
            (src->resource_kind == FVM_RESOURCE_NATIVE_FUNCTION ||
             src->resource_kind == FVM_RESOURCE_SCRIPT_FUNCTION)))
        {
            dst.kind = FVM_GRAPH_REMOTE_FUNCTION;
            dst.state.type = FVM_VALUE_STRING;
            FVM_Result result = fvm_function_capability_export(c, src, dst.state.string);
            require(result == FVM_OK, result);
            charge(budget, dst.state.string.size());
            graph.objects.push_back(dst);
            continue;
        }
        if (src->resource_kind == FVM_RESOURCE_CHANNEL || src->resource_kind == FVM_RESOURCE_MUTEX)
        {
            require(src->watchers.empty(), FVM_UNSUPPORTED);
            FVM_Result result;
            if (c->transfer_resources)
            {
                unsigned long token = 0;
                result = fvm_transfer_capture_resource(c, src, &token);
                dst.kind = FVM_GRAPH_LOCAL_CAPABILITY_BASE + (unsigned long)src->resource_kind;
                dst.state.type = FVM_VALUE_NUMBER;
                dst.state.number = (double)token;
            }
            else
            {
                result = fvm_capability_export(c, src, dst.state.string);
                dst.kind = FVM_GRAPH_REMOTE_CAPABILITY_BASE + (unsigned long)src->resource_kind;
                dst.state.type = FVM_VALUE_STRING;
            }
            require(result == FVM_OK, result);
            graph.objects.push_back(dst);
            continue;
        }
        require(src->resource_kind >= 0 && known_kind((unsigned long)src->resource_kind) &&
                    (!src->resource || src->resource_kind == FVM_RESOURCE_TEXT_FIELD || src->resource_kind == FVM_RESOURCE_BITMAP_DATA) &&
                    (!src->release_resource || src->resource_kind == FVM_RESOURCE_TEXT_FIELD ||
                     src->resource_kind == FVM_RESOURCE_BITMAP_DATA) &&
                    !src->callable.callback,
                FVM_UNSUPPORTED);
        dst.kind = (unsigned long)src->resource_kind;
        dst.invocable = capture_callback(
            c, src->invocable.callback, src->invocable.userdata,
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            src->invocable.root, seen, budget);
        dst.constructable = capture_callback(
            c, src->constructable.callback, src->constructable.userdata,
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            src->constructable.root, seen, budget);
        for (fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator method =
                 src->method_values.begin();
             method != src->method_values.end(); ++method)
        {
            FVM_Value v;
            fvm_value_undefined(&v);
            v.type = FVM_VALUE_OBJECT;
            v.object = method->second;
            dst.method_values[method->first] = capture_value(c, v, seen, budget);
            charge(budget, method->first.size() + 12);
            ++entries;
            require(entries <= max_entries, FVM_UNSUPPORTED);
        }
        for (fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator watcher =
                 src->watchers.begin();
             watcher != src->watchers.end(); ++watcher)
        {
            FVM_Value descriptor;
            fvm_value_undefined(&descriptor);
            descriptor.type = FVM_VALUE_OBJECT;
            descriptor.object = watcher->second;
            dst.watchers[watcher->first] = capture_value(c, descriptor, seen, budget);
            charge(budget, watcher->first.size() + 12);
            require(++entries <= max_entries, FVM_UNSUPPORTED);
        }
        dst.attributes = src->attributes;
        for (fvm_detail::map<fvm_detail::string, unsigned int>::iterator flag =
                 dst.attributes.begin();
             flag != dst.attributes.end(); ++flag)
        {
            require(flag->second <= FVM_PROPERTY_AVM1_FLAGS_MASK);
            charge(budget, flag->first.size() + 12);
            ++entries;
            require(entries <= max_entries, FVM_UNSUPPORTED);
        }
        if (dst.kind == FVM_RESOURCE_BITMAP_DATA)
        {
            FVM_Result result = fvm_bitmap_snapshot(src, dst.state.string);
            require(result == FVM_OK, result);
            dst.state.type = FVM_VALUE_STRING;
            charge(budget, dst.state.string.size());
        }
        if (dst.kind == FVM_RESOURCE_TEXT_FIELD)
        {
            charge(budget, fvm_textfield_snapshot_size(src));
            FVM_Result result = fvm_textfield_snapshot(src, dst.text);
            require(result == FVM_OK, result);
        }
        if (dst.kind == FVM_RESOURCE_BOOLEAN || dst.kind == FVM_RESOURCE_NUMBER || dst.kind == FVM_RESOURCE_STRING || dst.kind == FVM_RESOURCE_ARRAY ||
            dst.kind == FVM_RESOURCE_DATE || dst.kind == FVM_RESOURCE_SUPER || dst.kind == FVM_RESOURCE_WITH_SCOPE || dst.kind == FVM_RESOURCE_SCRIPT_FUNCTION || dst.kind == FVM_RESOURCE_MOVIECLIP ||
            dst.kind == FVM_RESOURCE_COLOR_TRANSFORM || dst.kind == FVM_RESOURCE_TRANSFORM ||
            (dst.kind == FVM_RESOURCE_COLOR_MATRIX_FILTER || dst.kind == FVM_RESOURCE_BLUR_FILTER || dst.kind == FVM_RESOURCE_CONVOLUTION_FILTER))
        {
            require(
                src->resource_result_valid &&
                    (dst.kind == FVM_RESOURCE_ARRAY || src->resource_result.type ==
                        (dst.kind == FVM_RESOURCE_BOOLEAN   ? FVM_VALUE_BOOLEAN
                         : dst.kind == FVM_RESOURCE_STRING ? FVM_VALUE_STRING
                         : (dst.kind == FVM_RESOURCE_SCRIPT_FUNCTION || dst.kind == FVM_RESOURCE_SUPER || dst.kind == FVM_RESOURCE_WITH_SCOPE || dst.kind == FVM_RESOURCE_MOVIECLIP || dst.kind == FVM_RESOURCE_COLOR_TRANSFORM ||
                            dst.kind == FVM_RESOURCE_TRANSFORM || (dst.kind == FVM_RESOURCE_COLOR_MATRIX_FILTER || dst.kind == FVM_RESOURCE_BLUR_FILTER || dst.kind == FVM_RESOURCE_CONVOLUTION_FILTER))
                             ? FVM_VALUE_OBJECT
                             : FVM_VALUE_NUMBER)),
                FVM_UNSUPPORTED);
            if (dst.kind == FVM_RESOURCE_COLOR_TRANSFORM)
            {
                double native_channels[8];
                /* [GC:] Reject malformed private color storage before capturing its owned graph. */
                require(fvm_color_transform_values(c, src, native_channels), FVM_UNSUPPORTED);
            }
            if (dst.kind == FVM_RESOURCE_COLOR_MATRIX_FILTER)
            {
                double coefficients[20];
                /* [GC:] Reject invalid private filter storage before capturing its managed edge. */
                require(fvm_filter_matrix(c, src, coefficients), FVM_UNSUPPORTED);
            }
            if (dst.kind == FVM_RESOURCE_BLUR_FILTER)
            {
                FVM_Convolution convolution;
                /* [GC:] Reject malformed convolution data before capturing its managed private
                 * edge. */
                require(fvm_convolution_values(c, src, convolution), FVM_UNSUPPORTED);
            }
            if (dst.kind == FVM_RESOURCE_CONVOLUTION_FILTER)
            {
                double options[3];
                /* [GC:] Reject malformed private blur data before capturing its owned graph. */
                require(fvm_blur_values(c, src, options), FVM_UNSUPPORTED);
            }
            dst.state = capture_value(c, src->resource_result, seen, budget);
        }
        else if (dst.kind == FVM_RESOURCE_MATH_NAMESPACE)
        {
            dst.state.type = FVM_VALUE_NUMBER;
            dst.state.number = c->random_state;
        }
        else
            require(!src->resource_result_valid, FVM_UNSUPPORTED);
        dst.prototype = identify(c, src->prototype, seen);
        dst.metatable = identify(c, src->metatable, seen);
        dst.interfaces_are_prototypes = src->interfaces_are_prototypes;
        /* [GC:] Capture interface edges with the same identity map as other object references. */
        for (size_t j = 0; j < src->implemented_interfaces.size(); ++j)
            dst.implemented_interfaces.push_back(identify(c, src->implemented_interfaces[j], seen));
        size_t count = src->properties.size() + src->indexed.size() + src->methods.size() +
                       src->metamethods.size() + src->accessors.size() +
                       src->implemented_interfaces.size();
        require(count <= max_entries - entries, FVM_UNSUPPORTED);
        entries += count;
        charge(budget, count * 4);
        for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator p = src->properties.begin();
             p != src->properties.end(); ++p)
            charge(budget, p->first.size());
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator p =
                 src->methods.begin();
             p != src->methods.end(); ++p)
            charge(budget, p->first.size());
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator p =
                 src->metamethods.begin();
             p != src->metamethods.end(); ++p)
            charge(budget, p->first.size());
        for (fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator p =
                 src->accessors.begin();
             p != src->accessors.end(); ++p)
            charge(budget, p->first.size());
        for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator p = src->properties.begin();
             p != src->properties.end(); ++p)
            dst.properties[p->first] = capture_value(c, p->second, seen, budget);
        for (fvm_detail::map<unsigned long, FVM_Value>::iterator p = src->indexed.begin();
             p != src->indexed.end(); ++p)
        {
            require(p->first <= 0xffffffffUL, FVM_UNSUPPORTED);
            dst.indexed[p->first] = capture_value(c, p->second, seen, budget);
        }
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator p =
                 src->methods.begin();
             p != src->methods.end(); ++p)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            require(
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                !p->second.collector_entry ||
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    (fvm_function_collector_callback(p->second.callback) ||
                     fvm_broadcaster_callback_id(p->second.callback) != 0),
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_UNSUPPORTED);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst.methods[p->first] = capture_callback(
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                c, p->second.callback, p->second.userdata,
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                p->second.root, seen, budget);
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator p =
                 src->metamethods.begin();
             p != src->metamethods.end(); ++p)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            require(
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                !p->second.collector_entry ||
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    (fvm_function_collector_callback(p->second.callback) ||
                     fvm_broadcaster_callback_id(p->second.callback) != 0),
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_UNSUPPORTED);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst.metamethods[p->first] = capture_callback(
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                c, p->second.callback, p->second.userdata,
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                p->second.root, seen, budget);
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        for (fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator p =
                 src->accessors.begin();
             p != src->accessors.end(); ++p)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            Accessor a;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            a.getter =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                capture_callback(
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    c, p->second.getter, p->second.getter ? p->second.userdata : 0,
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    p->second.getter ? p->second.root : 0, seen, budget);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            a.setter =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                capture_callback(
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    c, p->second.setter, p->second.setter ? p->second.userdata : 0,
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    p->second.setter ? p->second.root : 0, seen, budget);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst.accessors[p->first] = a;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        graph.objects.push_back(dst);
    }
    return graph;
}

/* Compute and return little double. */
bool little_double()
{
    require(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559, FVM_UNSUPPORTED);

    /* Initialization / Инициализация локального состояния. */
    const unsigned char little[] = {0, 0, 0, 0, 0, 0, 0xf0, 0x3f};
    const unsigned char big[] = {0x3f, 0xf0, 0, 0, 0, 0, 0, 0};
    unsigned char bytes[8];
    double one = 1;

    /* Computation / Вычисление и обновление состояния. */
    std::memcpy(bytes, &one, 8);
    require(!std::memcmp(bytes, little, 8) || !std::memcmp(bytes, big, 8), FVM_UNSUPPORTED);
    return !std::memcmp(bytes, little, 8);
}

/* Define Writer struct and its representation. */
struct Writer
{
    fvm_detail::vector<unsigned char> bytes;
    /* Implement byte for this subsystem. */
    void byte(unsigned long v)
    {

        /* Computation / Вычисление и обновление состояния. */
        require(bytes.size() < max_bytes, FVM_UNSUPPORTED);
        bytes.push_back((unsigned char)v);
    }
    /* Implement u32 for this subsystem. */
    void u32(unsigned long v)
    {
        require(v <= 0xffffffffUL, FVM_UNSUPPORTED);
        for (int i = 24; i >= 0; i -= 8)
            byte(v >> i);
    }
    /* Implement ref for this subsystem. */
    void ref(size_t id)
    {
        u32(id == none ? 0xffffffffUL : (unsigned long)id);
    }
    /* Initialize string and its owned state. */
    void string(const fvm_detail::string &s)
    {
        u32((unsigned long)s.size());
        for (size_t i = 0; i < s.size(); ++i)
            byte((unsigned char)s[i]);
    }
    /* Emit only valid canonical property names; never emit an unreadable graph. */
    void key(const fvm_detail::string &name)
    {
        fvm_detail::string canonical;
        require(fvm_property_key(name.data(), name.size(), canonical) == FVM_OK);
        require(name == canonical, FVM_UNSUPPORTED);
        string(name);
    }
    /* Emit a counted value and its metadata reference. */
    void value(const Value &v)
    {
        byte(v.type);
        if (v.type == FVM_VALUE_BOOLEAN)
            byte(v.boolean ? 1 : 0);
        if (v.type == FVM_VALUE_STRING)
            string(v.string);
        if (v.type == FVM_VALUE_OBJECT)
            ref(v.object);
        if (v.type == FVM_VALUE_NUMBER)
        {
            unsigned char raw[8];
            bool little = little_double();
            std::memcpy(raw, &v.number, 8);
            for (size_t i = 0; i < 8; ++i)
                byte(raw[little ? 7 - i : i]);
        }
        ref(v.metatable);
    }
    /* Implement number for this subsystem. */
    void number(double d)
    {
        Value v;

        /* Computation / Вычисление и обновление состояния. */
        v.type = FVM_VALUE_NUMBER;
        v.number = d;
        value(v);
    }
    /* Implement text style for this subsystem. */
    void text_style(const FVM_TextStyleState &s)
    {

        /* Computation / Вычисление и обновление состояния. */
        string(s.font);
        number(s.size);
        number(s.leading);
        u32(s.color);
        u32(s.alignment);
        if (s.alignment & FVM_TEXT_STYLE_HAS_SPACING)
            number(s.letter_spacing);
        if (s.alignment & FVM_TEXT_STYLE_HAS_INSETS)
        {
            number(s.left_margin);
            number(s.right_margin);
            number(s.block_indent);
        }
        if (s.alignment & FVM_TEXT_STYLE_HAS_INDENT)
            number(s.indent);
        if (s.alignment & FVM_TEXT_STYLE_HAS_TABS)
        {
            u32((unsigned long)s.tab_stops.size());
            for (size_t i = 0; i < s.tab_stops.size(); ++i) number(s.tab_stops[i]);
        }
        require(s.source.index >= (-2147483647L - 1L) && s.source.index <= 2147483647L,
                FVM_UNSUPPORTED);
        u32(s.source.index < 0 ? (unsigned long)(s.source.index + 2147483647L) + 1UL
                               : 0x80000000UL + (unsigned long)s.source.index);
        u32((unsigned long)s.source.bytes.size());
        for (size_t i = 0; i < s.source.bytes.size(); ++i)
            byte(s.source.bytes[i]);
    }
    /* Implement text field for this subsystem. */
    void text_field(const FVM_TextFieldState &s)
    {

        /* Computation / Вычисление и обновление состояния. */
        text_style(s.current);
        text_style(s.future);
        string(s.text);
        number(s.width);
        number(s.height);
        byte((s.multiline ? FVM_TEXT_STATE_MULTILINE : 0UL) |
             (s.auto_size_left ? FVM_TEXT_STATE_AUTO_SIZE_LEFT : 0UL) |
             (s.x != 0 || s.y != 0 ? FVM_TEXT_STATE_HAS_POSITION : 0UL) |
             (s.anchor == FVM_TEXT_ANCHOR_CENTER ? FVM_TEXT_STATE_CENTER : 0UL) |
             (s.anchor == FVM_TEXT_ANCHOR_RIGHT ? FVM_TEXT_STATE_RIGHT : 0UL));
        byte(s.wrap ? 1 : 0);
        if (s.x != 0 || s.y != 0) { number(s.x); number(s.y); }
    }
    /* Implement callback for this subsystem. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    void callback(const Callback &v)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        u32(v.builtin);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        string(v.name);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        ref(v.root);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        byte(v.root_userdata ? 1 : 0);
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
};
/* Write schema 8 records or schema 9 records with explicit interface-edge modes. */
void write_graph(const Graph &g, Writer &w, unsigned int version)
{

    /* Computation / Вычисление и обновление состояния. */
    w.value(g.value);
    w.u32((unsigned long)g.objects.size());
    for (size_t i = 0; i < g.objects.size(); ++i)
    {
        const Object &o = g.objects[i];
        w.u32(o.kind);
        w.value(o.state);
        if (o.kind == FVM_RESOURCE_TEXT_FIELD)
            w.text_field(o.text);
        w.ref(o.prototype);
        w.ref(o.metatable);
        if (version >= FVM_GRAPH_SNAPSHOT_INTERFACES_VERSION)
            w.byte(o.interfaces_are_prototypes ? FVM_GRAPH_INTERFACES_PROTOTYPES
                                               : FVM_GRAPH_INTERFACES_CONSTRUCTORS);
        w.u32((unsigned long)o.implemented_interfaces.size());
        for (size_t j = 0; j < o.implemented_interfaces.size(); ++j)
            w.ref(o.implemented_interfaces[j]);
        w.u32((unsigned long)o.properties.size());
        for (fvm_detail::map<fvm_detail::string, Value>::iterator p = o.properties.begin();
             p != o.properties.end(); ++p)
        {
            w.key(p->first);
            w.value(p->second);
        }
        w.u32((unsigned long)o.indexed.size());
        for (fvm_detail::map<unsigned long, Value>::iterator p = o.indexed.begin();
             p != o.indexed.end(); ++p)
        {
            w.u32(p->first);
            w.value(p->second);
        }
        w.u32((unsigned long)o.methods.size());
        for (fvm_detail::map<fvm_detail::string, Callback>::iterator p = o.methods.begin();
             p != o.methods.end(); ++p)
        {
            w.key(p->first);
            w.callback(p->second);
        }
        w.u32((unsigned long)o.metamethods.size());
        for (fvm_detail::map<fvm_detail::string, Callback>::iterator p = o.metamethods.begin();
             p != o.metamethods.end(); ++p)
        {
            w.key(p->first);
            w.callback(p->second);
        }
        w.u32((unsigned long)o.accessors.size());
        for (fvm_detail::map<fvm_detail::string, Accessor>::iterator p = o.accessors.begin();
             p != o.accessors.end(); ++p)
        {
            w.key(p->first);
            w.callback(p->second.getter);
            w.callback(p->second.setter);
        }
        w.u32((unsigned long)o.attributes.size());
        for (fvm_detail::map<fvm_detail::string, unsigned int>::iterator p = o.attributes.begin();
             p != o.attributes.end(); ++p)
        {
            w.key(p->first);
            w.u32(p->second);
        }
        w.callback(o.invocable);
        w.callback(o.constructable);
        w.u32((unsigned long)o.method_values.size());
        for (fvm_detail::map<fvm_detail::string, Value>::iterator p = o.method_values.begin();
             p != o.method_values.end(); ++p)
        {
            w.key(p->first);
            w.value(p->second);
        }
        w.u32((unsigned long)o.watchers.size());
        for (fvm_detail::map<fvm_detail::string, Value>::iterator p = o.watchers.begin();
             p != o.watchers.end(); ++p)
        {
            w.key(p->first);
            w.value(p->second);
        }
    }
}

/* Define Reader struct and its representation. */
struct Reader
{
    const unsigned char *data;
    size_t size, offset, entries;
    unsigned int version;
    /* Initialize Reader and its owned state. */
    Reader(const void *p, size_t n)
        : data((const unsigned char *)p), size(n), offset(0), entries(0), version(1)
    {
    }
    /* Compute and return byte. */
    unsigned long byte()
    {
        require(offset < size);
        return data[offset++];
    }
    /* Compute and return u32. */
    unsigned long u32()
    {
        unsigned long v = 0;
        for (int i = 0; i < 4; ++i)
            v = (v << 8) | byte();
        return v;
    }
    /* Compute and return ref. */
    size_t ref()
    {
        unsigned long v = u32();
        return v == 0xffffffffUL ? none : (size_t)v;
    }
    /* Count the current value. */
    size_t count()
    {
        size_t n = (size_t)u32();

        /* Computation / Вычисление и обновление состояния. */
        require(n <= max_entries - entries && n <= size - offset);
        entries += n;
        return n;
    }
    /* Initialize string and its owned state. */
    fvm_detail::string string(bool value = false)
    {
        size_t n = (size_t)u32();

        /* Computation / Вычисление и обновление состояния. */
        require(n <= size - offset);
        require(value && version >= 4 || !n || !std::memchr(data + offset, 0, n));
        fvm_detail::string s((const char *)data + offset, n);
        offset += n;
        return s;
    }
    /* Graph 8 property names retain NUL and normalize surrogate-pair aliases.
     * Callback identifiers still use the terminated-name record above. */
    fvm_detail::string key()
    {
        fvm_detail::string raw = string(version >= 8);
        /* Legacy graphs still forbid NUL, but valid UTF-16 aliases share identity. */
        fvm_detail::string canonical;
        require(fvm_property_key(raw.data(), raw.size(), canonical) == FVM_OK);
        return canonical;
    }
    /* Compute and return value. */
    Value value(bool native_binary = false)
    {

        /* Initialization / Инициализация локального состояния. */
        Value v;
        unsigned long type = byte();

        /* Computation / Вычисление и обновление состояния. */
        require(type <= FVM_VALUE_OBJECT);
        v.type = (FVM_ValueType)type;
        if (v.type == FVM_VALUE_BOOLEAN)
        {
            unsigned long b = byte();
            require(b <= 1);
            v.boolean = (int)b;
        }
        if (v.type == FVM_VALUE_STRING)
        {
            v.string = string(true);
            if (version >= 4 && !native_binary)
            {
                FVM_Value checked;
                require(fvm_value_string_bytes(&checked, v.string.data(), v.string.size()) ==
                        FVM_OK);
            }
        }
        if (v.type == FVM_VALUE_OBJECT)
            v.object = ref();
        if (v.type == FVM_VALUE_NUMBER)
        {
            unsigned char raw[8];
            bool little = little_double();
            for (size_t i = 0; i < 8; ++i)
                raw[little ? 7 - i : i] = (unsigned char)byte();
            std::memcpy(&v.number, raw, 8);
        }

        /* Computation / Вычисление и обновление состояния. */
        v.metatable = ref();
        require(v.type != FVM_VALUE_OBJECT || v.metatable == none);
        return v;
    }
    /* Compute and return number. */
    double number()
    {
        Value v = value();
        require(v.type == FVM_VALUE_NUMBER && v.metatable == none);
        return v.number;
    }
    /* Compute and return text style. */
    FVM_TextStyleState text_style()
    {
        FVM_TextStyleState s;

        /* Computation / Вычисление и обновление состояния. */
        s.font = string();
        s.size = number();
        s.leading = number();
        s.color = u32();
        s.alignment = u32();
        if (s.alignment & FVM_TEXT_STYLE_HAS_SPACING)
            s.letter_spacing = number();
        if (s.alignment & FVM_TEXT_STYLE_HAS_INSETS)
        {
            s.left_margin = number();
            s.right_margin = number();
            s.block_indent = number();
        }
        if (s.alignment & FVM_TEXT_STYLE_HAS_INDENT)
            s.indent = number();
        if (s.alignment & FVM_TEXT_STYLE_HAS_TABS)
        {
            size_t count = (size_t)u32();
            require(count <= FVM_TEXT_STYLE_MAX_TABS);
            s.tab_stops.resize(count);
            for (size_t i = 0; i < count; ++i) s.tab_stops[i] = number();
        }
        unsigned long index = u32();
        s.source.index =
            index >= 0x80000000UL ? (long)(index - 0x80000000UL) : (long)index - 2147483647L - 1L;
        size_t length = (size_t)u32();

        /* Computation / Вычисление и обновление состояния. */
        require(length <= size - offset);
        s.source.bytes.resize(length);
        if (length)
            std::memcpy(s.source.bytes.begin(), data + offset, length);
        offset += length;
        return s;
    }
    /* Compute and return text field. */
    FVM_TextFieldState text_field()
    {
        FVM_TextFieldState s;

        /* Computation / Вычисление и обновление состояния. */
        s.current = text_style();
        s.future = text_style();
        /* Native TextField content is counted value text, not a terminated selector/key. */
        s.text = string(true);
        s.width = number();
        s.height = number();
        unsigned long multiline = byte(), wrap = byte();

        /* Computation / Вычисление и обновление состояния. */
        require(!(multiline & ~FVM_TEXT_STATE_POLICY_MASK) && wrap <= 1);
        s.multiline = (multiline & FVM_TEXT_STATE_MULTILINE) != 0;
        s.auto_size_left = (multiline & FVM_TEXT_STATE_AUTO_SIZE_LEFT) != 0;
        s.wrap = wrap != 0;
        unsigned long anchor = multiline & (FVM_TEXT_STATE_CENTER | FVM_TEXT_STATE_RIGHT);
        require(anchor != (FVM_TEXT_STATE_CENTER | FVM_TEXT_STATE_RIGHT) && (!anchor || s.auto_size_left));
        s.anchor = anchor == FVM_TEXT_STATE_RIGHT ? FVM_TEXT_ANCHOR_RIGHT :
                   anchor == FVM_TEXT_STATE_CENTER ? FVM_TEXT_ANCHOR_CENTER : FVM_TEXT_ANCHOR_LEFT;
        if (multiline & FVM_TEXT_STATE_HAS_POSITION) { s.x = number(); s.y = number(); }
        return s;
    }
    /* Compute and return callback. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    Callback callback()
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        Callback v;

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        v.builtin = u32();
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        v.name = string();
        /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
        v.root = ref();
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        unsigned long mode = byte();

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        require(mode <= 1);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        v.root_userdata = mode != 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        require(!v.builtin || v.name.empty() ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_text_callback_userdata(v.builtin, v.name.c_str()));
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        require(v.builtin < 201 || v.builtin > 203 || !v.name.empty());
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        require(!v.root_userdata || v.root != none);
        /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
        require(v.builtin || !v.name.empty() || (v.root == none && !v.root_userdata));
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return v;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
};

/* Validate ref. */
void validate_ref(size_t ref, size_t count)
{
    require(ref == none || ref < count);
}

/* Validate value. */
void validate_value(const Value &v, size_t count)
{
    validate_ref(v.metatable, count);
    if (v.type == FVM_VALUE_OBJECT)
        require(v.object < count);
}

/* Validate callback. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
void validate_callback(const Callback &v, size_t count)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    validate_ref(v.root, count);
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Read graph. */
Graph read_graph(Reader &r, unsigned int version)
{
    Graph g;
    g.value = r.value();
    size_t n = (size_t)r.u32();
    require(n <= max_objects && n <= r.size - r.offset);
    for (size_t i = 0; i < n; ++i)
    {
        Object o;
        o.kind = r.u32();
        require(known_kind(o.kind), FVM_UNSUPPORTED);
        /* Bitmap native payload is validated as binary storage, not as ActionScript text. */
        o.state = r.value(o.kind == FVM_RESOURCE_BITMAP_DATA);
        /* [GC:] Array length is retained typed user data; its primitive metatable
         * is validated and restored through the ordinary graph identity map. */
        require(o.kind == FVM_RESOURCE_ARRAY || o.state.metatable == none);
        if (o.kind == FVM_RESOURCE_TEXT_FIELD)
            o.text = r.text_field();
        o.prototype = r.ref();
        o.metatable = r.ref();
        if (version >= FVM_GRAPH_SNAPSHOT_INTERFACES_VERSION)
        {
            unsigned int mode = r.byte();
            require(mode == FVM_GRAPH_INTERFACES_CONSTRUCTORS || mode == FVM_GRAPH_INTERFACES_PROTOTYPES);
            o.interfaces_are_prototypes = mode == FVM_GRAPH_INTERFACES_PROTOTYPES;
        }
        if (version >= 6)
        {
            size_t count = r.count();
            for (size_t j = 0; j < count; ++j)
                o.implemented_interfaces.push_back(r.ref());
        }
        if (o.kind == FVM_RESOURCE_BITMAP_DATA)
            require(o.state.type == FVM_VALUE_STRING && fvm_bitmap_snapshot_valid(o.state.string));
        else if (o.kind == FVM_GRAPH_REMOTE_FUNCTION || o.kind == FVM_GRAPH_REMOTE_CHANNEL || o.kind == FVM_GRAPH_REMOTE_MUTEX)
            require(o.state.type == FVM_VALUE_STRING && o.state.string.size() <= 512);
        else if (o.kind == FVM_RESOURCE_BOOLEAN || o.kind == FVM_RESOURCE_NUMBER || o.kind == FVM_RESOURCE_STRING || o.kind == FVM_RESOURCE_ARRAY || o.kind == FVM_RESOURCE_SCRIPT_FUNCTION ||
                 o.kind == FVM_RESOURCE_DATE || o.kind == FVM_RESOURCE_SUPER || o.kind == FVM_RESOURCE_WITH_SCOPE || o.kind == FVM_RESOURCE_MOVIECLIP || o.kind == FVM_RESOURCE_COLOR_TRANSFORM || o.kind == FVM_RESOURCE_TRANSFORM ||
                 (o.kind == FVM_RESOURCE_COLOR_MATRIX_FILTER || o.kind == FVM_RESOURCE_BLUR_FILTER || o.kind == FVM_RESOURCE_CONVOLUTION_FILTER))
            require(o.kind == FVM_RESOURCE_ARRAY || o.state.type ==
                    (o.kind == FVM_RESOURCE_BOOLEAN   ? FVM_VALUE_BOOLEAN
                     : o.kind == FVM_RESOURCE_STRING ? FVM_VALUE_STRING
                     : (o.kind == FVM_RESOURCE_SCRIPT_FUNCTION || o.kind == FVM_RESOURCE_SUPER || o.kind == FVM_RESOURCE_WITH_SCOPE || o.kind == FVM_RESOURCE_MOVIECLIP || o.kind == FVM_RESOURCE_COLOR_TRANSFORM ||
                        o.kind == FVM_RESOURCE_TRANSFORM || (o.kind == FVM_RESOURCE_COLOR_MATRIX_FILTER || o.kind == FVM_RESOURCE_BLUR_FILTER || o.kind == FVM_RESOURCE_CONVOLUTION_FILTER))
                         ? FVM_VALUE_OBJECT
                         : FVM_VALUE_NUMBER));
        else if (o.kind == FVM_RESOURCE_MATH_NAMESPACE || o.kind == FVM_GRAPH_LOCAL_CHANNEL || o.kind == FVM_GRAPH_LOCAL_MUTEX)
            require(o.state.type == FVM_VALUE_NUMBER && o.state.number >= 0 &&
                    o.state.number <= 4294967295.0 &&
                    o.state.number == (unsigned long)o.state.number);
        else
            require(o.state.type == FVM_VALUE_UNDEFINED);
        if (o.kind == FVM_RESOURCE_STRING)
        {
            FVM_Value text;
            require(fvm_value_string_bytes(&text, o.state.string.data(), o.state.string.size()) ==
                    FVM_OK);
            fvm_detail::vector<unsigned int> units;
            require(fvm_string_units(text, units) == FVM_OK);
        }
        size_t count = r.count();
        for (size_t j = 0; j < count; ++j)
        {
            fvm_detail::string name = r.key();
            require(o.properties.find(name) == o.properties.end());
            o.properties[name] = r.value();
        }
        count = r.count();
        for (size_t j = 0; j < count; ++j)
        {
            unsigned long key = r.u32();
            require(o.indexed.find(key) == o.indexed.end());
            o.indexed[key] = r.value();
        }
        count = r.count();
        for (size_t j = 0; j < count; ++j)
        {
            fvm_detail::string name = r.key();
            require(o.methods.find(name) == o.methods.end());
            Callback cb = r.callback();
            require(cb.builtin || !cb.name.empty());
            o.methods[name] = cb;
        }
        count = r.count();
        for (size_t j = 0; j < count; ++j)
        {
            fvm_detail::string name = r.key();
            require(o.metamethods.find(name) == o.metamethods.end());
            Callback cb = r.callback();
            require(cb.builtin || !cb.name.empty());
            o.metamethods[name] = cb;
        }
        count = r.count();
        for (size_t j = 0; j < count; ++j)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::string name = r.key();
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            require(o.accessors.find(name) == o.accessors.end());
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            Accessor a;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            a.getter = r.callback();
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            a.setter = r.callback();
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            require(a.getter.builtin || !a.getter.name.empty() || a.setter.builtin ||
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    !a.setter.name.empty());
            /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
            require(a.getter.root == none || a.setter.root == none ||
                    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного
                       обработчика. */
                    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного
                       обработчика. */
                    a.getter.root == a.setter.root);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            o.accessors[name] = a;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        if (version >= 2)
        {
            count = r.count();
            for (size_t j = 0; j < count; ++j)
            {
                fvm_detail::string name = r.key();
                unsigned long flags = r.u32();
                require(flags <= FVM_PROPERTY_AVM1_FLAGS_MASK && o.attributes.find(name) == o.attributes.end());
                o.attributes[name] = (unsigned int)flags;
            }
        }
        if (version >= 2)
        {
            o.invocable = r.callback();
            o.constructable = r.callback();
            count = r.count();
            for (size_t j = 0; j < count; ++j)
            {
                fvm_detail::string name = r.key();
                Value method = r.value();
                require(method.type == FVM_VALUE_OBJECT &&
                        o.method_values.find(name) == o.method_values.end());
                o.method_values[name] = method;
            }
        }
        if (version >= 3)
        {
            count = r.count();
            for (size_t j = 0; j < count; ++j)
            {
                fvm_detail::string name = r.key();
                Value descriptor = r.value();
                /* Loaded watch accepts empty names; descriptor/reference validation still applies. */
                require(descriptor.type == FVM_VALUE_OBJECT &&
                        o.watchers.find(name) == o.watchers.end());
                o.watchers[name] = descriptor;
            }
        }
        if (o.kind == FVM_RESOURCE_DATE)
            require(o.state.number != o.state.number ||
                    std::fabs(o.state.number) <= 8640000000000000.0);
        if (o.kind == FVM_RESOURCE_ARRAY)
        {
            for (fvm_detail::map<unsigned long, Value>::iterator slot = o.indexed.begin();
                 slot != o.indexed.end(); ++slot)
                /* Native arrays may retain own slots outside readonly or reduced length. */
                require(slot->first < 0xffffffffUL);
        }
        g.objects.push_back(o);
    }

    /* Computation / Вычисление и обновление состояния. */
    require(r.offset == r.size);
    validate_value(g.value, n);
    for (size_t i = 0; i < n; ++i)
    {
        const Object &o = g.objects[i];
        validate_ref(o.prototype, n);
        validate_ref(o.metatable, n);
        for (size_t j = 0; j < o.implemented_interfaces.size(); ++j)
        {
            validate_ref(o.implemented_interfaces[j], n);
            require(o.implemented_interfaces[j] != none);
            require(o.interfaces_are_prototypes || fvm_class_kind(g.objects[o.implemented_interfaces[j]].kind));
        }
        validate_value(o.state, n);
        validate_callback(o.invocable, n);
        validate_callback(o.constructable, n);
        for (fvm_detail::map<fvm_detail::string, Value>::iterator p = o.method_values.begin();
             p != o.method_values.end(); ++p)
            validate_value(p->second, n);
        for (fvm_detail::map<fvm_detail::string, Value>::iterator p = o.watchers.begin();
             p != o.watchers.end(); ++p)
            validate_value(p->second, n);
        for (fvm_detail::map<fvm_detail::string, Value>::iterator p = o.properties.begin();
             p != o.properties.end(); ++p)
            validate_value(p->second, n);
        for (fvm_detail::map<unsigned long, Value>::iterator p = o.indexed.begin();
             p != o.indexed.end(); ++p)
            validate_value(p->second, n);
        for (fvm_detail::map<fvm_detail::string, Callback>::iterator p = o.methods.begin();
             p != o.methods.end(); ++p)
            validate_callback(p->second, n);
        for (fvm_detail::map<fvm_detail::string, Callback>::iterator p = o.metamethods.begin();
             p != o.metamethods.end(); ++p)
            validate_callback(p->second, n);
        for (fvm_detail::map<fvm_detail::string, Accessor>::iterator p = o.accessors.begin();
             p != o.accessors.end(); ++p)
        {
            validate_callback(p->second.getter, n);
            validate_callback(p->second.setter, n);
        }
    }
    /* [GC:] Private ColorTransform storage is numeric data, never a callback/prototype graph. */
    for (size_t i = 0; i < n; ++i)
    {
        const Object &color = g.objects[i];
        if (color.kind != FVM_RESOURCE_COLOR_TRANSFORM && color.kind != FVM_RESOURCE_COLOR_MATRIX_FILTER)
            continue;
        require(color.state.type == FVM_VALUE_OBJECT && color.state.object != none);
        const Object &data = g.objects[color.state.object];
        require(data.kind == FVM_RESOURCE_PLAIN_OBJECT && data.prototype == none && data.metatable == none &&
                data.state.type == FVM_VALUE_UNDEFINED &&
                data.properties.size() == (color.kind == FVM_RESOURCE_COLOR_MATRIX_FILTER ? 20 : 8) && data.indexed.empty() &&
                data.methods.empty() && data.metamethods.empty() && data.accessors.empty() &&
                data.method_values.empty() && data.watchers.empty() && data.attributes.empty() &&
                data.implemented_interfaces.empty() && !data.interfaces_are_prototypes && !data.invocable.builtin &&
                data.invocable.name.empty() && !data.constructable.builtin &&
                data.constructable.name.empty());
        const char *names[] = {"redMultiplier",   "greenMultiplier", "blueMultiplier",
                               "alphaMultiplier", "redOffset",       "greenOffset",
                               "blueOffset",      "alphaOffset"};
        const char *matrix_names[] = {"0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",
                                      "10", "11", "12", "13", "14", "15", "16", "17", "18", "19"};
        unsigned int channels = color.kind == FVM_RESOURCE_COLOR_MATRIX_FILTER ? 20 : 8;
        for (unsigned int channel = 0; channel < channels; ++channel)
        {
            fvm_detail::map<fvm_detail::string, Value>::const_iterator entry =
                data.properties.find(color.kind == FVM_RESOURCE_COLOR_MATRIX_FILTER ? matrix_names[channel] : names[channel]);
            require(entry != data.properties.end() && entry->second.type == FVM_VALUE_NUMBER &&
                    entry->second.metatable == none);
            if (color.kind == FVM_RESOURCE_COLOR_MATRIX_FILTER)
                require(entry->second.number >= -1000000.0 && entry->second.number <= 1000000.0);
        }
    }
    /* [GC:] Blur state is a detached canonical numeric record with no script dispatch edges. */
    for (size_t i = 0; i < n; ++i)
    {
        const Object &filter = g.objects[i];
        if (filter.kind != FVM_RESOURCE_CONVOLUTION_FILTER)
            continue;
        require(filter.state.type == FVM_VALUE_OBJECT && filter.state.object != none);
        const Object &data = g.objects[filter.state.object];
        require(data.kind == FVM_RESOURCE_PLAIN_OBJECT && data.prototype == none && data.metatable == none &&
                data.state.type == FVM_VALUE_UNDEFINED && data.properties.size() == 3 &&
                data.indexed.empty() && data.methods.empty() && data.metamethods.empty() &&
                data.accessors.empty() && data.method_values.empty() && data.watchers.empty() &&
                data.attributes.empty() && data.implemented_interfaces.empty() && !data.interfaces_are_prototypes &&
                !data.invocable.builtin && data.invocable.name.empty() &&
                !data.constructable.builtin && data.constructable.name.empty());
        const char *names[] = {"blurX", "blurY", "quality"};
        for (unsigned index = 0; index < 3; ++index)
        {
            fvm_detail::map<fvm_detail::string, Value>::const_iterator entry =
                data.properties.find(names[index]);
            require(entry != data.properties.end() && entry->second.type == FVM_VALUE_NUMBER &&
                    entry->second.metatable == none && entry->second.number >= 0 &&
                    entry->second.number <= (index == 2 ? 15 : 255) &&
                    (index != 2 || entry->second.number == std::floor(entry->second.number)));
        }
    }
    /* [GC:] Convolution records contain bounded numeric options and a detached coefficient
     * sequence. */
    for (size_t i = 0; i < n; ++i)
    {
        const Object &filter = g.objects[i];
        if (filter.kind != FVM_RESOURCE_BLUR_FILTER)
            continue;
        require(filter.state.type == FVM_VALUE_OBJECT && filter.state.object != none);
        const Object &data = g.objects[filter.state.object];
        require(data.kind == FVM_RESOURCE_PLAIN_OBJECT && data.prototype == none && data.metatable == none &&
                data.state.type == FVM_VALUE_UNDEFINED && data.indexed.empty() &&
                data.methods.empty() && data.metamethods.empty() && data.accessors.empty() &&
                data.method_values.empty() && data.watchers.empty() && data.attributes.empty() &&
                data.implemented_interfaces.empty() && !data.interfaces_are_prototypes && !data.invocable.builtin &&
                data.invocable.name.empty() && !data.constructable.builtin &&
                data.constructable.name.empty());
        const char *names[] = {"matrixX", "matrixY", "divisor", "bias",  "preserveAlpha",
                               "clamp",   "color",   "alpha",   "length"};
        double parameters[9];
        for (unsigned index = 0; index < 9; ++index)
        {
            fvm_detail::map<fvm_detail::string, Value>::const_iterator entry =
                data.properties.find(names[index]);
            require(entry != data.properties.end() && entry->second.type == FVM_VALUE_NUMBER &&
                    entry->second.metatable == none);
            parameters[index] = entry->second.number;
            require(parameters[index] >= (index == 2 || index == 3 ? -1000000.0 : 0) &&
                    parameters[index] <= (index == 6 ? 16777215.0 : 1000000.0));
        }
        for (unsigned index = 0; index < 2; ++index)
            require(parameters[index] <= 15 && parameters[index] == std::floor(parameters[index]));
        require((parameters[4] == 0 || parameters[4] == 1) &&
                (parameters[5] == 0 || parameters[5] == 1) &&
                parameters[6] == std::floor(parameters[6]) && parameters[7] <= 1 &&
                parameters[8] <= 225 && parameters[8] == std::floor(parameters[8]) &&
                parameters[8] >= parameters[0] * parameters[1]);
        unsigned length = (unsigned)parameters[8];
        require(data.properties.size() == 9 + length);
        for (unsigned index = 0; index < length; ++index)
        {
            char text[4];
            unsigned position = 0;
            if (index >= 100)
                text[position++] = (char)('0' + index / 100);
            if (index >= 10)
                text[position++] = (char)('0' + (index / 10) % 10);
            text[position++] = (char)('0' + index % 10);
            text[position] = 0;
            fvm_detail::map<fvm_detail::string, Value>::const_iterator entry =
                data.properties.find(text);
            require(entry != data.properties.end() && entry->second.type == FVM_VALUE_NUMBER &&
                    entry->second.metatable == none && entry->second.number >= -1000000.0 &&
                    entry->second.number <= 1000000.0);
        }
    }
    /* [GC:] Transform private state contains only an optional reference to an actual MovieClip. */
    for (size_t i = 0; i < n; ++i)
    {
        const Object &transform = g.objects[i];
        if (transform.kind != FVM_RESOURCE_TRANSFORM)
            continue;
        require(transform.state.type == FVM_VALUE_OBJECT && transform.state.object != none);
        const Object &data = g.objects[transform.state.object];
        require(data.kind == FVM_RESOURCE_PLAIN_OBJECT && data.prototype == none && data.metatable == none &&
                data.state.type == FVM_VALUE_UNDEFINED && data.properties.size() == 1 &&
                data.indexed.empty() && data.methods.empty() && data.metamethods.empty() &&
                data.accessors.empty() && data.method_values.empty() && data.watchers.empty() &&
                data.attributes.empty() && data.implemented_interfaces.empty() && !data.interfaces_are_prototypes &&
                !data.invocable.builtin && data.invocable.name.empty() &&
                !data.constructable.builtin && data.constructable.name.empty());
        fvm_detail::map<fvm_detail::string, Value>::const_iterator reference =
            data.properties.find("clip");
        require(reference != data.properties.end() && reference->second.metatable == none &&
                (reference->second.type == FVM_VALUE_UNDEFINED ||
                 (reference->second.type == FVM_VALUE_OBJECT && reference->second.object != none &&
                  g.objects[reference->second.object].kind == FVM_RESOURCE_MOVIECLIP)));
    }
    /* Superclass facade metadata is a private receiver/depth record, never executable data. */
    for (size_t i = 0; i < n; ++i)
    {
        const Object &facade = g.objects[i];
        if (facade.kind != FVM_RESOURCE_SUPER)
            continue;
        require(facade.invocable.builtin == 808 && facade.invocable.name.empty() &&
                /* [GC:] The callback capture must retain exactly this facade identity. */
                facade.invocable.root == i && facade.invocable.root_userdata);
        const Object &data = g.objects[facade.state.object];
        require(data.kind == FVM_RESOURCE_PLAIN_OBJECT && data.prototype == none && data.metatable == none &&
                data.state.type == FVM_VALUE_UNDEFINED && data.properties.size() == 2 &&
                data.indexed.empty() && data.methods.empty() && data.metamethods.empty() &&
                data.accessors.empty() && data.method_values.empty() && data.watchers.empty() &&
                data.attributes.empty() && data.implemented_interfaces.empty() && !data.interfaces_are_prototypes &&
                !data.invocable.builtin && data.invocable.name.empty() &&
                !data.constructable.builtin && data.constructable.name.empty());
        fvm_detail::map<fvm_detail::string, Value>::iterator receiver =
            data.properties.find("receiver");
        fvm_detail::map<fvm_detail::string, Value>::iterator depth = data.properties.find("depth");
        require(receiver != data.properties.end() && receiver->second.type == FVM_VALUE_OBJECT &&
                receiver->second.object != i && receiver->second.metatable == none);
        require(depth != data.properties.end() && depth->second.type == FVM_VALUE_NUMBER &&
                depth->second.metatable == none && depth->second.number >= 0 &&
                depth->second.number <= 1024 &&
                depth->second.number == std::floor(depth->second.number));
    }
    /* Watch descriptors are private data records, never opaque host pointers. */
    for (size_t i = 0; i < n; ++i)
        for (fvm_detail::map<fvm_detail::string, Value>::iterator watcher =
                 g.objects[i].watchers.begin();
             watcher != g.objects[i].watchers.end(); ++watcher)
        {
            const Object &descriptor = g.objects[watcher->second.object];
            require(descriptor.kind == FVM_RESOURCE_PLAIN_OBJECT && descriptor.implemented_interfaces.empty() && !descriptor.interfaces_are_prototypes &&
                    descriptor.prototype == none && descriptor.metatable == none);
            fvm_detail::map<fvm_detail::string, Value>::iterator handler =
                descriptor.properties.find("handler");
            require(handler != descriptor.properties.end() &&
                    handler->second.type == FVM_VALUE_OBJECT);
            const Object &function = g.objects[handler->second.object];
            require(function.invocable.builtin || !function.invocable.name.empty());
            require(descriptor.properties.find("userData") != descriptor.properties.end());
        }
    /* Validate every edge before following any prototype chain. */
    for (size_t i = 0; i < n; ++i)
    {
        size_t cursor = i, depth = 0;
        while (cursor != none && depth++ <= n)
            cursor = g.objects[cursor].prototype;
        require(cursor == none);
    }
    return g;
}
const unsigned char magic[8] = {'F', 'V', 'M', 'G', 0, 0, 0, FVM_GRAPH_BASE_WIRE_VERSION};

/* Decode the current value. */
Graph decode(const void *data, size_t size)
{

    /* Computation / Вычисление и обновление состояния. */
    require(data && size >= 16 && size <= max_bytes);
    require(!std::memcmp(data, magic, FVM_GRAPH_MAGIC_PREFIX_BYTES));
    unsigned int version = ((const unsigned char *)data)[FVM_GRAPH_VERSION_OFFSET];
    require(version >= FVM_GRAPH_MIN_WIRE_VERSION && version <= FVM_GRAPH_WIRE_VERSION);
    Reader header(data, size);
    header.offset = 8;

    /* Initialization / Инициализация локального состояния. */
    size_t body = (size_t)header.u32();
    unsigned long crc = header.u32();
    require(body == size - 16);
    const unsigned char *bytes = (const unsigned char *)data + 16;
    require(crc == (unsigned long)crc32(0, bytes, (uInt)body));
    Reader reader(bytes, body);
    reader.version = version;
    return read_graph(reader, version);
}

/* Encode with the oldest compatible schema, retaining installed empty snapshot lists. */
void encode(FVM_Context *c, const FVM_Value &value, unsigned char **data, size_t *size)
{

    /* Initialization / Инициализация локального состояния. */
    Graph graph = capture(c, value);
    Writer body;

    /* Computation / Вычисление и обновление состояния. */
    unsigned int version = FVM_GRAPH_BASE_WIRE_VERSION;
    for (size_t i = 0; i < graph.objects.size(); ++i)
        if (graph.objects[i].interfaces_are_prototypes)
        {
            version = FVM_GRAPH_SNAPSHOT_INTERFACES_VERSION;
            break;
        }
    write_graph(graph, body, version);
    require(body.bytes.size() <= max_bytes - 16, FVM_UNSUPPORTED);
    Writer header;
    for (size_t i = 0; i < 8; ++i)
        header.byte(i == FVM_GRAPH_VERSION_OFFSET ? version : magic[i]);

    /* Computation / Вычисление и обновление состояния. */
    header.u32((unsigned long)body.bytes.size());
    header.u32((unsigned long)crc32(0, body.bytes.begin(), (uInt)body.bytes.size()));
    unsigned char *p = (unsigned char *)std::malloc(body.bytes.size() + 16);

    /* Computation / Вычисление и обновление состояния. */
    require(p != 0, FVM_OUT_OF_MEMORY);
    std::memcpy(p, header.bytes.begin(), 16);
    std::memcpy(p + 16, body.bytes.begin(), body.bytes.size());
    *data = p;
    *size = body.bytes.size() + 16;
}

/* Restore value. */
FVM_Value restore_value(FVM_Context *c, const Value &src,
                        const fvm_detail::vector<FVM_Object *> &objects)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = src.type;
    if (src.metatable != none)
        v.metatable = objects[src.metatable];
    if (src.type == FVM_VALUE_NUMBER)
        v.number = src.number;
    if (src.type == FVM_VALUE_BOOLEAN)
        v.boolean = src.boolean;
    if (src.type == FVM_VALUE_OBJECT)
        v.object = objects[src.object];
    if (src.type == FVM_VALUE_STRING)
    {
        char *p = new char[src.string.size() + 1];
        std::memcpy(p, src.string.c_str(), src.string.size() + 1);
        try
        {
            /* [GC:] Register owned bytes and their counted length together; failed insertion leaves
             * caller cleanup active. */
            fvm_context_adopt_string(c, p, src.string.size());
        }
        catch (...)
        {
            delete[] p;
            throw;
        }
        v.string = p;
        v.string_size = src.string.size();
    }
    return v;
}

/* Restore callback. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_NativeEntry
restore_callback(FVM_Context *c, const Callback &src,
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 const fvm_detail::vector<FVM_Object *> &objects)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeEntry dst;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (src.builtin)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        dst.callback = fvm_numeric_callback_from_id(src.builtin);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!dst.callback)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst.callback = fvm_text_callback_from_id(src.builtin);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!dst.callback)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst.callback = fvm_error_callback_from_id(src.builtin);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!dst.callback)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst.callback = fvm_number_callback_from_id(src.builtin);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!dst.callback)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst.callback = fvm_string_callback_from_id(src.builtin);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!dst.callback)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst.callback = fvm_base_callback_from_id(src.builtin);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!dst.callback)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst.callback = fvm_date_callback_from_id(src.builtin);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!dst.callback)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst.callback = fvm_function_callback_from_id(src.builtin);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!dst.callback)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst.callback = fvm_movieclip_callback_from_id(src.builtin);
        /* [GC:] Restore geometry callbacks before their captured class roots are rebound. */
        if (!dst.callback)
            /* [GC:] Resolve the native implementation associated with the transported root graph.
             */
            dst.callback = fvm_geometry_callback_from_id(src.builtin);
        /* [GC:] Resolve generic broadcaster bridges before rebinding their cloned list roots. */
        if (!dst.callback)
            dst.callback = fvm_broadcaster_callback_from_id(src.builtin);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        require(dst.callback != 0, FVM_UNSUPPORTED);
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    else if (!src.name.empty())
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, FVM_NativeFunction>::iterator p =
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            c->serialization_callbacks.find(src.name);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        require(p != c->serialization_callbacks.end(), FVM_UNSUPPORTED);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        dst.callback = p->second;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (src.builtin >= 201 && src.builtin <= 203)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        dst.userdata = fvm_text_callback_userdata(src.builtin, src.name.c_str());
        /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
        require(dst.userdata && src.root == none && !src.root_userdata);
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (src.root != none)
        /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
        dst.root = objects[src.root];
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (src.root_userdata)
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        dst.userdata = dst.root;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    /* [GC:] Restored broadcasters keep the same rooted script/GC entry policy as fresh ones. */
    dst.collector_entry = fvm_function_collector_callback(dst.callback) ||
        fvm_broadcaster_callback_id(dst.callback) != 0 || fvm_url_collector_callback(dst.callback) ||
        fvm_timers_collector_callback(dst.callback);
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return dst;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Restore the current value. */
void restore(FVM_Context *c, const Graph &g, FVM_Value *out)
{

    /* Initialization / Инициализация локального состояния. */
    const size_t old_objects = c->objects.size(), old_strings = c->strings.size();
    /* [GC:] Учитываем выделения для планирования следующего прохода сборки. */
    const unsigned long old_allocations = c->gc_allocations;
    fvm_detail::map<fvm_detail::string, FVM_FontSource *> old_fonts = c->fonts;
    fvm_detail::string old_default_font = c->default_font;
    fvm_detail::vector<FVM_Object *> objects;
    try
    {
        for (size_t i = 0; i < g.objects.size(); ++i)
        {
            FVM_Object *o = 0;
            const Object &src = g.objects[i];
            FVM_Result r;
            if (src.kind == FVM_GRAPH_LOCAL_CHANNEL || src.kind == FVM_GRAPH_LOCAL_MUTEX || src.kind == FVM_GRAPH_REMOTE_FUNCTION || src.kind == FVM_GRAPH_REMOTE_CHANNEL || src.kind == FVM_GRAPH_REMOTE_MUTEX)
            {
                require(src.properties.empty() && src.indexed.empty() && src.methods.empty() &&
                        src.metamethods.empty() && src.accessors.empty() && src.prototype == none &&
                        src.metatable == none);
                r = src.kind >= 4000
                        ? fvm_capability_import(c, src.state.string.c_str(), src.kind - 4000, &o)
                        : fvm_transfer_restore_resource(c, src.kind - 3000,
                                                        (unsigned long)src.state.number, &o);
            }
            else
                r = fvm_object_create(c, &o);
            require(r == FVM_OK, r);
            if (src.kind == FVM_GRAPH_REMOTE_FUNCTION || src.kind == FVM_GRAPH_REMOTE_CHANNEL || src.kind == FVM_GRAPH_REMOTE_MUTEX)
                o->resource_path = c->resource_receive_path;
            objects.push_back(o);
        }
        bool random = false;
        unsigned long random_state = 0;
        for (size_t i = 0; i < objects.size(); ++i)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            const Object &src = g.objects[i];
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Object *dst = objects[i];
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (src.kind == FVM_GRAPH_LOCAL_CHANNEL || src.kind == FVM_GRAPH_LOCAL_MUTEX || src.kind == FVM_GRAPH_REMOTE_FUNCTION || src.kind == FVM_GRAPH_REMOTE_CHANNEL || src.kind == FVM_GRAPH_REMOTE_MUTEX)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                continue;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst->resource_kind = (int)src.kind;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (src.kind == FVM_RESOURCE_TEXT_FIELD)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Result result = fvm_textfield_restore(c, dst, src.text);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                require(result == FVM_OK, result);
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (src.kind == FVM_RESOURCE_BITMAP_DATA)
            {
                /* [GC:] Attach a restored bitmap's separately owned pixel allocation. */
                FVM_Result restored = fvm_bitmap_restore(dst, src.state.string);
                require(restored == FVM_OK, restored);
            }
            if (src.kind == FVM_RESOURCE_BOOLEAN || src.kind == FVM_RESOURCE_NUMBER || src.kind == FVM_RESOURCE_STRING || src.kind == FVM_RESOURCE_ARRAY ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                src.kind == FVM_RESOURCE_DATE || src.kind == FVM_RESOURCE_SUPER || src.kind == FVM_RESOURCE_WITH_SCOPE || src.kind == FVM_RESOURCE_SCRIPT_FUNCTION || src.kind == FVM_RESOURCE_MOVIECLIP ||
                src.kind == FVM_RESOURCE_COLOR_TRANSFORM || src.kind == FVM_RESOURCE_TRANSFORM ||
                (src.kind == FVM_RESOURCE_COLOR_MATRIX_FILTER || src.kind == FVM_RESOURCE_BLUR_FILTER || src.kind == FVM_RESOURCE_CONVOLUTION_FILTER))
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                dst->resource_result_valid = true;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                dst->resource_result = restore_value(c, src.state, objects);
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (src.kind == FVM_RESOURCE_MATH_NAMESPACE)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                unsigned long state = (unsigned long)src.state.number;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                require(!random || state == random_state);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                random = true;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                random_state = state;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst->invocable = restore_callback(c, src.invocable, objects);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst->constructable = restore_callback(c, src.constructable, objects);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst->invocable.collector_entry =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_function_collector_callback(dst->invocable.callback) ||
                fvm_broadcaster_callback_id(dst->invocable.callback) != 0 ||
                fvm_url_collector_callback(dst->invocable.callback) ||
                fvm_timers_collector_callback(dst->invocable.callback);
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (fvm_detail::map<fvm_detail::string, Value>::iterator p = src.method_values.begin();
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 p != src.method_values.end(); ++p)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                dst->method_values[p->first] = restore_value(c, p->second, objects).object;
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (fvm_detail::map<fvm_detail::string, Value>::iterator p = src.watchers.begin();
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 p != src.watchers.end(); ++p)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                dst->watchers[p->first] = restore_value(c, p->second, objects).object;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            dst->attributes = src.attributes;
            dst->interfaces_are_prototypes = src.interfaces_are_prototypes;
            /* [GC:] Restore private interface edges using destination graph identities. */
            for (size_t j = 0; j < src.implemented_interfaces.size(); ++j)
                dst->implemented_interfaces.push_back(objects[src.implemented_interfaces[j]]);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (src.prototype != none)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                dst->prototype = objects[src.prototype];
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (src.metatable != none)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                dst->metatable = objects[src.metatable];
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (fvm_detail::map<fvm_detail::string, Value>::iterator p = src.properties.begin();
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 p != src.properties.end(); ++p)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                dst->properties[p->first] = restore_value(c, p->second, objects);
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (fvm_detail::map<unsigned long, Value>::iterator p = src.indexed.begin();
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 p != src.indexed.end(); ++p)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                dst->indexed[p->first] = restore_value(c, p->second, objects);
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (fvm_detail::map<fvm_detail::string, Callback>::iterator p = src.methods.begin();
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 p != src.methods.end(); ++p)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                dst->methods[p->first] = restore_callback(c, p->second, objects);
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (fvm_detail::map<fvm_detail::string, Callback>::iterator p =
                     /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов.
                      */
                 src.metamethods.begin();
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 p != src.metamethods.end(); ++p)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                dst->metamethods[p->first] = restore_callback(c, p->second, objects);
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (fvm_detail::map<fvm_detail::string, Accessor>::iterator p = src.accessors.begin();
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 p != src.accessors.end(); ++p)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_NativeEntry
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    get = restore_callback(c, p->second.getter, objects),
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    set = restore_callback(c, p->second.setter, objects);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                /* Old graphs shared callback 1010 for toString and _target. Preserve the
                 * accessor role while the method now restores the corrected dot-path callback. */
                if (src.kind == FVM_RESOURCE_MOVIECLIP && p->first == fvm_detail::string("_target") &&
                    p->second.getter.builtin == 1010)
                    get.callback = fvm_movieclip_callback_from_id(1005);
                require(!get.callback || !set.callback || get.userdata == set.userdata);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_AccessorEntry a;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                a.getter = get.callback;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                a.setter = set.callback;
                /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика.
                 */
                /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика.
                 */
                a.root = get.root ? get.root : set.root;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                a.userdata = get.callback ? get.userdata : set.userdata;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                /* [GC:] Only retired rootless built-in stubs are upgraded; host captures survive. */
                if (src.kind == FVM_RESOURCE_TEXT_FIELD)
                    fvm_textfield_upgrade_accessor(p->first.c_str(), a);
                dst->accessors[p->first] = a;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        FVM_Value result = restore_value(c, g.value, objects);
        if (random)
            c->random_state = random_state;
        *out = result;
    }
    catch (...)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] No graph was published and no user finalizer may run on failed input. */
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (size_t i = old_objects; i < c->objects.size(); ++i)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (c->objects[i]->release_resource)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                c->objects[i]->release_resource(c->objects[i]->resource);
            /* [GC:] Удаляем запись или область корней после завершения работы с ней. */
            delete c->objects[i];
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (fvm_detail::map<fvm_detail::string, FVM_FontSource *>::iterator i = c->fonts.begin();
             /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
             i != c->fonts.end(); ++i)
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (old_fonts.find(i->first) == old_fonts.end())
                /* [GC:] Удаляем запись или область корней после завершения работы с ней. */
                delete i->second;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->fonts.swap(old_fonts);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->default_font.swap(old_default_font);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->objects.resize(old_objects);
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (size_t i = old_strings; i < c->strings.size(); ++i)
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        {
            /* [GC:] Remove the exact buffer index before freeing unpublished decode storage. */
            fvm_context_forget_string(c, c->strings[i].data);
            /* [GC:] Release the failed record buffer before shortening the descriptor vector. */
            delete[] c->strings[i].data;
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->strings.resize(old_strings);
        /* [GC:] Учитываем выделения для планирования следующего прохода сборки. */
        c->gc_allocations = old_allocations;
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        throw;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
}
} // namespace
/* Register serialization callback. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_serialization_register_callback(
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Context *c, const char *name,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeFunction fn)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!c || !name || !*name || std::strlen(name) > 255 || !fn || c->destroying ||
        /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
        c->gc_collecting || c->gc_releasing || !std::strncmp(name, "flashvm.", 8))
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, FVM_NativeFunction>::iterator p =
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            c->serialization_callbacks.find(name);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (p != c->serialization_callbacks.end() && p->second != fn)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->serialization_callbacks[name] = fn;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
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
/* Serialize value. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
fvm_value_serialize(FVM_Context *c, const FVM_Value *v, unsigned char **data,
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    size_t *size)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!data || !size)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    *data = 0;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    *size = 0;
    /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
    if (!c || !v || c->destroying || c->gc_releasing)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
        FVM_NativeCallGuard guard(c);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        encode(c, *v, data, size);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    catch (const Failure &f)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return f.result;
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
/* Deserialize value. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_value_deserialize(
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Context *c, const void *data, size_t size,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value *out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!out)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_undefined(out);
    /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
    if (!c || c->destroying || c->gc_collecting || c->gc_releasing)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
        FVM_NativeCallGuard guard(c);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        Graph graph = decode(data, size);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        restore(c, graph, out);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    catch (const Failure &f)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return f.result;
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

/* Implement serialized free for this subsystem. */
extern "C" void fvm_serialized_free(void *data)
{
    std::free(data);
}
/* Primitive entry points let the C89 file transport use the identical format. */
/* Encode serialization primitive. */
extern "C" FVM_Result fvm_serialization_encode_primitive(const FVM_Value *v, unsigned char **data,
                                                         size_t *size)
{
    if (!v || !data || !size)
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    *data = 0;
    *size = 0;
    try
    {
        Graph g = capture(0, *v);
        require(g.objects.empty());
        encode(0, *v, data, size);
        return FVM_OK;
    }
    catch (const Failure &f)
    {
        return f.result;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Decode serialization primitive. */
extern "C" FVM_Result fvm_serialization_decode_primitive(const void *data, size_t size,
                                                         FVM_Value *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    try
    {
        Graph g = decode(data, size);
        require(g.objects.empty(), FVM_UNSUPPORTED);
        Value &v = g.value;
        out->type = v.type;
        out->number = v.number;
        out->boolean = v.boolean;
        if (v.type == FVM_VALUE_STRING)
        {
            char *p = (char *)std::malloc(v.string.size() + 1);
            require(p != 0, FVM_OUT_OF_MEMORY);
            std::memcpy(p, v.string.c_str(), v.string.size() + 1);
            out->string = p;
            out->string_size = v.string.size();
        }
        return FVM_OK;
    }
    catch (const Failure &f)
    {
        fvm_value_undefined(out);
        return f.result;
    }
    catch (...)
    {
        fvm_value_undefined(out);
        return FVM_OUT_OF_MEMORY;
    }
}
