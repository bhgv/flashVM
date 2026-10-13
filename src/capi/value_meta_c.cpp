/* FlashVM: C embedding API: value meta.
 * File: src/capi/value_meta_c.cpp.
 */

#include "../core/runtime.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include "flashvm/function.h"
#include <new>

/* Update value metatable. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_value_set_metatable(
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Context *context, FVM_Value *value,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *metatable)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!context || fvm_validate_value(context, value) != FVM_OK ||
        /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
        (metatable && metatable->owner != context) || context->destroying || context->gc_releasing)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (value->type == FVM_VALUE_OBJECT)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return fvm_table_set_metatable(context, value->object, metatable);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    value->metatable = metatable;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Read value metatable. */
extern "C" FVM_Result fvm_value_get_metatable(FVM_Context *context, const FVM_Value *value,
                                              FVM_Object **out)
{
    if (!context || !out || fvm_validate_value(context, value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    *out = value->type == FVM_VALUE_OBJECT ? value->object->metatable : value->metatable;
    return FVM_OK;
}

/* Invoke value meta. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_value_meta_invoke(
    FVM_Context *context, const FVM_Value *value,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const char *name, unsigned int count,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const FVM_Value *args, FVM_Value *out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!context || !name || !*name || !out || (count && !args) || count > 65533 ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_validate_value(context, value) != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (unsigned int i = 0; i < count; ++i)
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (fvm_validate_value(context, args + i) != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    /* Snapshot owner before clearing out: callers may replace a value in place. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value owner = *value;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *meta = owner.type == FVM_VALUE_OBJECT ? owner.object->metatable : owner.metatable;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (unsigned int depth = 0; meta && depth < 1024; ++depth, meta = meta->metatable)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator hook =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                meta->metamethods.find(name);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator script =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                meta->properties.find(name);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Object *function =
                script != meta->properties.end() &&
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                           объектов. */
                        script->second.type == FVM_VALUE_OBJECT &&
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                           объектов. */
                        script->second.object->invocable.callback
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    ? script->second.object
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    : 0;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (hook == meta->metamethods.end() && !function)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                continue;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::vector<FVM_Value> parameters;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            parameters.push_back(owner);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value defining;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_value_undefined(&defining);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            defining.type = FVM_VALUE_OBJECT;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            defining.object = meta;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            parameters.push_back(defining);
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (unsigned int i = 0; i < count; ++i)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                parameters.push_back(args[i]);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value result;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_value_undefined(&result);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Result status;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (hook == meta->metamethods.end())
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                status = fvm_function_invoke(
                    context, function,
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    owner.type == FVM_VALUE_OBJECT ? owner.object : 0,
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    count + 2, parameters.begin(), &result);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            else
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_NativeEntry callback = hook->second;
                /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в
                 * сборщик. */
                /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в
                 * сборщик. */
                FVM_NativeCallGuard guard(context);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                status =
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    callback.callback(context, owner.type == FVM_VALUE_OBJECT ? owner.object : 0,
                                      /* [GC:] Операция в блоке управления достижимостью и временем
                                         жизни объектов. */
                                      /* [GC:] Операция в блоке управления достижимостью и временем
                                         жизни объектов. */
                                      count + 2, parameters.begin(), &result, callback.userdata);
                /* [GC:] Retain primitive metamethod string results before capture roots unwind. */
                status = fvm_native_result(context, status, &result);
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (status == FVM_OK && fvm_validate_value(context, &result) != FVM_OK)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                status = FVM_INVALID_ARGUMENT;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (status == FVM_OK)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                *out = result;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            else
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_undefined(out);
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return status;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_undefined(out);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return meta ? FVM_ERROR : FVM_NOT_FOUND;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    catch (...)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_undefined(out);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OUT_OF_MEMORY;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Read value property. */
extern "C" FVM_Result fvm_value_get_property(FVM_Context *context, const FVM_Value *value,
                                             const char *name, FVM_Value *out)
{
    if (!context || !name || !out || fvm_validate_value(context, value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    if (value->type == FVM_VALUE_OBJECT)
        return fvm_object_get(context, value->object, name, out);

    /* Initialization / Инициализация локального состояния. */
    FVM_Value key;
    fvm_value_string(&key, name);
    FVM_Result result = fvm_value_meta_invoke(context, value, "__index", 1, &key, out);
    if (result == FVM_NOT_FOUND)
    {
        if (context->standard_library &&
            (value->type == FVM_VALUE_STRING || value->type == FVM_VALUE_NUMBER ||
             value->type == FVM_VALUE_BOOLEAN))
        {
            const char *type = value->type == FVM_VALUE_STRING    ? "String"
                               : value->type == FVM_VALUE_BOOLEAN ? "Boolean"
                                                                  : "Number";
            FVM_Object *boxed = 0;
            result = fvm_construct(context, type, 1, value, &boxed);
            return result == FVM_OK ? fvm_object_get(context, boxed, name, out) : result;
        }
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return result;
}

/* Update value property. */
extern "C" FVM_Result fvm_value_set_property(FVM_Context *context, const FVM_Value *value,
                                             const char *name, const FVM_Value *assigned)
{
    if (!context || !name || fvm_validate_value(context, value) != FVM_OK ||
        fvm_validate_value(context, assigned) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    if (value->type == FVM_VALUE_OBJECT)
        return fvm_object_set(context, value->object, name, assigned);

    /* Initialization / Инициализация локального состояния. */
    FVM_Value args[2], ignored;
    fvm_value_string(args, name);
    args[1] = *assigned;
    FVM_Result result = fvm_value_meta_invoke(context, value, "__newindex", 2, args, &ignored);
    return result == FVM_NOT_FOUND ? FVM_UNSUPPORTED : result;
}
