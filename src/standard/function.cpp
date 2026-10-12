/* FlashVM: Built-in ActionScript objects: function.
 * File: src/standard/function.cpp.
 */

#include "../core/string_internal.hpp"
#include "function_state.hpp"
#include "base_state.hpp"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/table.h"
#include "flashvm/avm1.h"
#include "../core/native_call_guard.hpp"
#include "../core/exception_internal.hpp"
#include <cstring>
#include <new>
#include <cmath>
namespace
{
/* Define anonymous enum and its representation. */
enum
{
    SCRIPT_FUNCTION = 32,
    FUNCTION_CLASS = 33,
    NATIVE_FUNCTION = 36
};

/* Compute and return function prototype. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Object *function_prototype(FVM_Context *c)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->constructors.find("Function");
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (ctor == c->constructors.end() || !ctor->second.root)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return 0;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator p =
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        ctor->second.root->properties.find("prototype");
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return p != ctor->second.root->properties.end() && p->second.type == FVM_VALUE_OBJECT
               /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
               ? p->second.object
               /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
               : 0;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Create the current value. */
FVM_Result create(FVM_Context *c, FVM_Object **out)
{
    FVM_Result r = fvm_object_create(c, out);
    if (r != FVM_OK)
        return r;

    /* Computation / Вычисление и обновление состояния. */
    (*out)->resource_kind = NATIVE_FUNCTION;
    (*out)->prototype = function_prototype(c);
    return FVM_OK;
}
/* Report whether the container has no elements. */
FVM_Result empty(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *out,
                 void *)
{
    fvm_value_undefined(out);
    return FVM_OK;
}

/* Compute and return function text. */
FVM_Result function_text(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *)
{
    return fvm_value_string(out, "[type Function]");
}

/* Compute and return box receiver. */
FVM_Result box_receiver(FVM_Context *c, const FVM_Value *value, FVM_Object *bundle,
                        FVM_Object **out)
{
    const char *type = value->type == FVM_VALUE_STRING    ? "String"
                       : value->type == FVM_VALUE_BOOLEAN ? "Boolean"
                                                          : "Number";
    if (bundle)
    {
        FVM_Value constructor;
        FVM_Result r = fvm_object_get(c, bundle, type, &constructor);
        if (r != FVM_OK)
            return r;
        if (constructor.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        return fvm_function_construct(c, constructor.object, 1, value, out);
    }
    /* Older wire prototypes did not capture the boxing constructors. */
    return fvm_construct(c, type, 1, value, out);
}

/* Call the current value. */
FVM_Result call(FVM_Context *c, FVM_Object *fn, unsigned int n, const FVM_Value *a, FVM_Value *out,
                void *userdata)
{
    FVM_Object *receiver = n && a[0].type == FVM_VALUE_OBJECT ? a[0].object : c->global;
    if (n && a[0].type != FVM_VALUE_OBJECT && a[0].type != FVM_VALUE_UNDEFINED &&
        a[0].type != FVM_VALUE_NULL)
    {
        FVM_Result r = box_receiver(c, a, (FVM_Object *)userdata, &receiver);
        if (r != FVM_OK)
            return r;
    }
    return fvm_function_invoke(c, fn, receiver, n ? n - 1 : 0, n ? a + 1 : 0, out);
}

/* Compute and return apply. */
FVM_Result apply(FVM_Context *c, FVM_Object *fn, unsigned int n, const FVM_Value *a, FVM_Value *out,
                 void *userdata)
{
    FVM_Object *receiver = n && a[0].type == FVM_VALUE_OBJECT ? a[0].object : c->global;
    /* apply and call use the same primitive receiver boxing rules. */
    if (n && a[0].type != FVM_VALUE_OBJECT && a[0].type != FVM_VALUE_UNDEFINED &&
        a[0].type != FVM_VALUE_NULL)
    {
        FVM_Result r = box_receiver(c, a, (FVM_Object *)userdata, &receiver);
        if (r != FVM_OK)
            return r;
    }
    if (n > 1 && a[1].type != FVM_VALUE_NULL && a[1].type != FVM_VALUE_UNDEFINED)
    {
        if (a[1].type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        FVM_Value length;
        FVM_Result r = fvm_object_get(c, a[1].object, "length", &length);
        if (r != FVM_OK)
            return r;
        double count = 0;
        r = fvm_value_to_number(c, &length, &count);
        if (r != FVM_OK)
            return r;
        if (!(count >= 0 && count <= 65536) || count != std::floor(count))
            return FVM_INVALID_ARGUMENT;
        fvm_detail::vector<FVM_Value> args((size_t)count);
        for (unsigned int i = 0; i < (unsigned int)count; ++i)
        {
            r = fvm_table_get_index(c, a[1].object, i, &args[i]);
            if (r != FVM_OK)
                return r;
        }
        return fvm_function_invoke(c, fn, receiver, (unsigned int)count, args.begin(), out);
    }
    return fvm_function_invoke(c, fn, receiver, 0, 0, out);
}

/* Compute and return constructor. */
FVM_Result constructor(FVM_Context *c, FVM_Object *, unsigned int, const FVM_Value *,
                       FVM_Value *out, void *ud)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *fn = 0;
    FVM_ScriptFunction body;
    body.body.push_back(0);
    FVM_Result r = fvm_function_make_script(c, body, 0, &fn);
    if (r != FVM_OK)
        return r;
    FVM_Value proto;
    r = fvm_object_get(c, (FVM_Object *)ud, "prototype", &proto);
    if (r != FVM_OK)
        return r;
    if (proto.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    fn->prototype = proto.object;
    fn->invocable.callback = empty;
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    out->type = FVM_VALUE_OBJECT;
    out->object = fn;
    return FVM_OK;
}

/* Compute and return hex. */
int hex(unsigned char c)
{
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
}

/* Compute and return script. */
FVM_Result script(FVM_Context *c, FVM_Object *receiver, unsigned int n, const FVM_Value *a,
                  FVM_Value *out, void *ud)
{
    FVM_Object *fn = (FVM_Object *)ud;
    if (!fn || fn->resource_kind != SCRIPT_FUNCTION || !fn->resource_result_valid ||
        fn->resource_result.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = fn->resource_result.object;
    FVM_Value code, params, constants, closure;
    FVM_Result r = fvm_object_get(c, data, "code", &code);
    if (r != FVM_OK)
        return r;
    r = fvm_object_get(c, data, "parameters", &params);
    if (r != FVM_OK)
        return r;
    r = fvm_object_get(c, data, "constants", &constants);
    if (r != FVM_OK)
        return r;
    r = fvm_object_get(c, data, "closure", &closure);
    if (r != FVM_OK)
        return r;
    if (code.type != FVM_VALUE_STRING || params.type != FVM_VALUE_OBJECT ||
        constants.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    size_t bytes = fvm_string_size(code);
    if (bytes % 2 || bytes > 131072)
        return FVM_INVALID_ARGUMENT;
    FVM_ScriptFunction body;
    for (size_t i = 0; i < bytes; i += 2)
    {
        int high = hex((unsigned char)code.string[i]), low = hex((unsigned char)code.string[i + 1]);
        if (high < 0 || low < 0)
            return FVM_INVALID_ARGUMENT;
        body.body.push_back((unsigned char)(high * 16 + low));
    }
    FVM_Value length;
    r = fvm_object_get(c, params.object, "length", &length);
    if (r != FVM_OK)
        return r;
    if (length.type != FVM_VALUE_NUMBER || !(length.number >= 0 && length.number <= 65535) ||
        length.number != std::floor(length.number))
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < (unsigned int)length.number; ++i)
    {
        FVM_Value v;
        r = fvm_table_get_index(c, params.object, i, &v);
        if (r != FVM_OK)
            return r;
        if (v.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
        body.parameters.push_back(fvm_detail::string(v.string));
    }
    r = fvm_object_get(c, constants.object, "length", &length);
    if (r != FVM_OK)
        return r;
    if (length.type != FVM_VALUE_NUMBER || !(length.number >= 0 && length.number <= 65535) ||
        length.number != std::floor(length.number))
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < (unsigned int)length.number; ++i)
    {
        FVM_Value v;
        r = fvm_table_get_index(c, constants.object, i, &v);
        if (r != FVM_OK)
            return r;
        if (v.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
        body.constants.push_back(fvm_detail::string(v.string));
    }
    FVM_Value name;
    r = fvm_object_get(c, data, "name", &name);
    if (r != FVM_OK)
        return r;
    if (name.type == FVM_VALUE_STRING)
        body.name = name.string;
    return fvm_avm1_invoke_script(c, body, closure.type == FVM_VALUE_OBJECT ? closure.object : 0,
                                  receiver, n, a, out, fn);
}
} // namespace
/* Build constructor and prototype for function. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
fvm_function_stage(FVM_Context *c,
                   /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                   fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
                   /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                   fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *klass = 0, *proto = 0;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result r = fvm_object_create(c, &klass);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_object_create(c, &proto);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    klass->resource_kind = FUNCTION_CLASS;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    proto->resource_kind = NATIVE_FUNCTION;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    proto->invocable.callback = empty;

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_undefined(&v);

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.type = FVM_VALUE_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.object = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    globals["Function"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    proto->properties["constructor"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.object = proto;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    klass->properties["prototype"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeEntry e;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    e.callback = call;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    e.collector_entry = true;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    proto->methods["call"] = e;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    e.callback = apply;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    proto->methods["apply"] = e;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    e.callback = function_text;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    e.collector_entry = false;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    proto->methods["toString"] = e;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    e.callback = constructor;
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    e.root = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    e.userdata = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    constructors["Function"] = e;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Validate function. */
FVM_Result fvm_function_validate(FVM_Context *c)
{
    FVM_Value v;
    return fvm_get_global(c, "Function", &v) == FVM_OK && v.type == FVM_VALUE_OBJECT &&
                   v.object->resource_kind == FUNCTION_CLASS
               ? FVM_OK
               : FVM_INVALID_ARGUMENT;
}

/* Compute and return function wrap entry. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result fvm_function_wrap_entry(
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Context *c, FVM_Object *owner, const char *name,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const FVM_NativeEntry &entry, FVM_Value *out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_Object *> &cache =
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        owner ? owner->method_values : c->function_objects;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator old = cache.find(name);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *fn = old == cache.end() ? 0 : old->second;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!fn || fn->invocable.callback != entry.callback ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fn->invocable.userdata != entry.userdata || fn->invocable.root != entry.root)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Result r = create(c, &fn);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fn->invocable = entry;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        cache[name] = fn;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    out->type = FVM_VALUE_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    out->object = fn;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Compute and return function make script. */
FVM_Result fvm_function_make_script(FVM_Context *c, const FVM_ScriptFunction &body,
                                    FVM_Object *closure, FVM_Object **out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *fn = 0, *data = 0, *params = 0, *constants = 0;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result r = create(c, &fn);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_object_create(c, &data);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_array_create_like(c, 0, (unsigned long)body.parameters.size(), &params);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_array_create_like(c, 0, (unsigned long)body.constants.size(), &constants);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (size_t i = 0; i < body.parameters.size(); ++i)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value v;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_string(&v, body.parameters[i].c_str());
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        r = fvm_table_set_index(c, params, (unsigned long)i, &v);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (size_t i = 0; i < body.constants.size(); ++i)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value v;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_string(&v, body.constants[i].c_str());
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        r = fvm_table_set_index(c, constants, (unsigned long)i, &v);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::vector<char> code;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const char *digits = "0123456789abcdef";
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (size_t i = 0; i < body.body.size(); ++i)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        code.push_back(digits[body.body[i] >> 4]);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        code.push_back(digits[body.body[i] & 15]);
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    code.push_back(0);

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_string(&v, code.begin());
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_object_set(c, data, "code", &v);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_undefined(&v);

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.type = FVM_VALUE_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.object = params;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["parameters"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.object = constants;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["constants"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.type = closure ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.object = closure;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["closure"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_string(&v, body.name.c_str());
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_object_set(c, data, "name", &v);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fn->resource_kind = SCRIPT_FUNCTION;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fn->resource_result_valid = true;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.type = FVM_VALUE_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.object = data;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fn->resource_result = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fn->invocable.callback = script;
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    fn->invocable.root = fn;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fn->invocable.userdata = fn;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fn->invocable.collector_entry = true;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_number(&v, (double)body.parameters.size());

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fn->properties["length"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fn->attributes["length"] = 7;

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *instance_proto = 0;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_object_create(c, &instance_proto);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.type = FVM_VALUE_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.object = instance_proto;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fn->properties["prototype"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.object = fn;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    instance_proto->properties["constructor"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    instance_proto->attributes["constructor"] = 1;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    *out = fn;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Create function native. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_function_create_native(
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Context *c, FVM_NativeFunction cb, void *ud,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *capture, FVM_Object **out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!out)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    *out = 0;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!c || !cb || (capture && capture->owner != c))
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Result r = create(c, out);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        (*out)->invocable.callback = cb;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        (*out)->invocable.userdata = ud;
        /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
        (*out)->invocable.root = capture;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    catch (const std::bad_alloc &)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        *out = 0;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OUT_OF_MEMORY;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Create function avm1. */
extern "C" FVM_Result fvm_function_create_avm1(FVM_Context *c, const void *actions, size_t size,
                                               unsigned int count, const char *const *parameters,
                                               FVM_Object *env, FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || (!actions && size) || (count && !parameters) || (env && env->owner != c) ||
        size > 65536 || count > 65535)
        return FVM_INVALID_ARGUMENT;
    FVM_AVM1Info info;
    info.struct_size = sizeof(info);
    FVM_Result r = fvm_avm1_inspect(actions, size, &info);
    if (r != FVM_OK)
        return r;
    try
    {
        FVM_ScriptFunction body;
        body.body.assign((const unsigned char *)actions, (const unsigned char *)actions + size);
        for (unsigned int i = 0; i < count; ++i)
        {
            if (!parameters[i])
                return FVM_INVALID_ARGUMENT;
            body.parameters.push_back(fvm_detail::string(parameters[i]));
        }
        return fvm_function_make_script(c, body, env, out);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Invoke function. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result
fvm_function_invoke(FVM_Context *c, FVM_Object *fn, FVM_Object *self,
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    unsigned int n, const FVM_Value *args, FVM_Value *out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!out)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_undefined(out);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!c || !fn || fn->owner != c || (self && self->owner != c) || (n && !args) || c->destroying)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (unsigned int i = 0; i < n; ++i)
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (fvm_validate_value(c, args + i) != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!fn->invocable.callback)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_UNSUPPORTED;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::vector<FVM_Value> roots;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value function;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_undefined(&function);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        function.type = FVM_VALUE_OBJECT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        function.object = fn;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        roots.push_back(function);
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (unsigned int i = 0; i < n; ++i)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            roots.push_back(args[i]);
        /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
        FVM_GCRootFrame frame;
        /* [GC:] Сохраняем предыдущий корень для восстановления цепочки после выхода. */
        frame.previous = c->gc_frames;
        /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
        frame.stack = &roots;
        /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
        frame.locals = 0;
        /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
        frame.receiver = self ? self : c->global;
        /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен быть LIFO. */
        c->gc_frames = &frame;
        /* Define Pop struct and its representation. */
        /* [GC:] Объявляем представление состояния сборщика или корней. */
        struct Pop
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Context *c;
            /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
            FVM_GCRootFrame *old;
            /* Release the resources owned by this Pop instance. */
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            ~Pop()
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен быть LIFO.
                 */
                /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен быть LIFO.
                 */
                c->gc_frames = old;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        } pop;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        pop.c = c;
        /* [GC:] Сохраняем предыдущий корень для восстановления цепочки после выхода. */
        pop.old = frame.previous;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeEntry entry = fn->invocable;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_resource_fault_begin(c);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (entry.collector_entry)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return fvm_exception_complete(
                /* [GC:] Подключаем стек, локальные значения или получателя к активному корню
                   вызова. */
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                c,
                /* [GC:] Подключаем стек, локальные значения или получателя к активному корню
                   вызова. */
                frame.receiver, entry.callback(c, frame.receiver, n, args, out, entry.userdata));
        /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
        FVM_NativeCallGuard guard(c);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return fvm_exception_complete(
            /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
            c, frame.receiver, entry.callback(c, frame.receiver, n, args, out, entry.userdata));
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    catch (const std::bad_alloc &)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_undefined(out);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OUT_OF_MEMORY;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    catch (...)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_undefined(out);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_ERROR;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Compute and return function construct. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_function_construct(
    FVM_Context *c, FVM_Object *fn, unsigned int n,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const FVM_Value *a, FVM_Object **out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!out)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    *out = 0;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!c || !fn || fn->owner != c || (n && !a))
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (unsigned int i = 0; i < n; ++i)
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (fvm_validate_value(c, a + i) != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (fn->constructable.callback)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value v;
            /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик.
             */
            /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик.
             */
            FVM_NativeCallGuard guard(c);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_NativeEntry e = fn->constructable;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Result r = fvm_exception_complete(
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                c, c->global,
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                e.callback(c, c->global, n, a, &v, e.userdata));
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (r != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return r;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (v.type != FVM_VALUE_OBJECT)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_ERROR;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            *out = v.object;
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OK;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!fn->invocable.callback || fn->resource_kind != SCRIPT_FUNCTION)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_UNSUPPORTED;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *instance = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Result r = fvm_object_create(c, &instance);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        instance->resource_kind = 30;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value proto;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        r = fvm_object_get(c, fn, "prototype", &proto);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (proto.type == FVM_VALUE_OBJECT)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            instance->prototype = proto.object;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value result;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        r = fvm_function_invoke(c, fn, instance, n, a, &result);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        *out = result.type == FVM_VALUE_OBJECT ? result.object : instance;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
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
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Map a native function callback to its stable serialization identifier. */
unsigned long fvm_function_callback_id(FVM_NativeFunction cb)
{
    return cb == script          ? 800
           : cb == call          ? 801
           : cb == apply         ? 802
           : cb == function_text ? 803
           : cb == empty         ? 804
           : cb == constructor   ? 805
                                 : 0;
}

/* Restore a native function callback from its stable serialization identifier. */
FVM_NativeFunction fvm_function_callback_from_id(unsigned long id)
{
    return id == 800   ? script
           : id == 801 ? call
           : id == 802 ? apply
           : id == 803 ? function_text
           : id == 804 ? empty
           : id == 805 ? constructor
                       : 0;
}

/* Compute and return function collector callback. */
bool fvm_function_collector_callback(FVM_NativeFunction cb)
{
    return cb == script || cb == call || cb == apply;
}
