/* FlashVM: C embedding API: value.
 * File: src/capi/value_c.cpp.
 */

#include "../standard/function_state.hpp"
#include "flashvm/function.h"
#include "../standard/base_state.hpp"
#include "../core/property_watch.hpp"
#include "flashvm/table.h"
#include "flashvm/property.h"
#include "../core/native_call_guard.hpp"
#include "../core/runtime.hpp"
#include "../core/exception_internal.hpp"
#include "flashvm/stack.h"
#include <cstring>
#include "../core/string_internal.hpp"
#include <new>
/* Validate value. */
FVM_Result fvm_validate_value(FVM_Context *context, const FVM_Value *value)
{
    if (!value)
        return FVM_INVALID_ARGUMENT;
    bool valid_type = false;
    for (int i = FVM_VALUE_UNDEFINED; i <= FVM_VALUE_OBJECT; ++i)
    {
        FVM_ValueType candidate = (FVM_ValueType)i;
        if (std::memcmp(&value->type, &candidate, sizeof(candidate)) == 0)
            valid_type = true;
    }
    if (!valid_type)
        return FVM_INVALID_ARGUMENT;
    if (value->type == FVM_VALUE_OBJECT && (!value->object || value->object->owner != context))
        return FVM_INVALID_ARGUMENT;
    if (value->metatable && (value->type == FVM_VALUE_OBJECT || value->metatable->owner != context))
        return FVM_INVALID_ARGUMENT;
    if (value->type == FVM_VALUE_STRING && !value->string)
        return FVM_INVALID_ARGUMENT;
    return FVM_OK;
}

/* Copy value. */
static FVM_Result copy_value(FVM_Context *context, const FVM_Value *src, FVM_Value *dst)
{
    FVM_Result r = fvm_validate_value(context, src);
    if (r != FVM_OK)
        return r;
    *dst = *src;
    if (src->type == FVM_VALUE_STRING)
    {
        size_t n = fvm_string_size(*src);
        char *p = new (std::nothrow) char[n + 1];
        if (!p)
            return FVM_OUT_OF_MEMORY;
        std::memcpy(p, src->string, n);
        p[n] = 0;
        try
        {
            context->strings.push_back(p);
        }
        catch (...)
        {
            delete[] p;
            return FVM_OUT_OF_MEMORY;
        }
        dst->string = p;
    }
    return FVM_OK;
}

/* Look up native method. */
FVM_Result fvm_lookup_native_method(FVM_Object *object, const char *name, FVM_NativeEntry *out)
{
    if (!object || !name || !out)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::string key(name);
        for (unsigned int depth = 0; object && depth < 1024; ++depth)
        {
            /* Function-valued data/accessor dispatch is still unsupported. A data
             * assignment must nevertheless shadow a previously registered method. */
            if (object->properties.find(key) != object->properties.end() ||
                object->accessors.find(key) != object->accessors.end())
                return FVM_UNSUPPORTED;
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method =
                object->methods.find(key);
            if (method != object->methods.end())
            {
                *out = method->second;
                return FVM_OK;
            }
            object = object->prototype;
        }
        return object ? FVM_ERROR : FVM_NOT_FOUND;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Read object no index. */
FVM_Result fvm_object_get_no_index(FVM_Context *c, FVM_Object *o, const char *name, FVM_Value *out)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *receiver = o;
    unsigned int depth = 0;
    if (!c || !o || o->owner != c || !name || !out)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    try
    {
        while (o && depth++ < 1024)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator it = o->properties.find(name);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (it != o->properties.end())
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                *out = it->second;
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_OK;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                o->accessors.find(name);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (accessor != o->accessors.end())
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_AccessorEntry entry = accessor->second;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_undefined(out);
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (!entry.getter)
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return FVM_OK;
                /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
                try
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                {
                    /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в
                     * сборщик. */
                    /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в
                     * сборщик. */
                    FVM_NativeCallGuard guard(c);
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    fvm_resource_fault_begin(c);
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return fvm_exception_complete(
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                           объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                           объектов. */
                        c,
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                           объектов. */
                        receiver, entry.getter(c, receiver, 0, 0, out, entry.userdata));
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
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return FVM_ERROR;
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                }
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                o->methods.find(name);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (method != o->methods.end())
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return fvm_function_wrap_entry(c, o, name, method->second, out);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            o = o->prototype;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        if (o)
            return FVM_ERROR;
        o = receiver;
        for (depth = 0; o && depth < 1024; ++depth)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (!o->metatable)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                break;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator m =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                o->metatable->properties.find("__index");
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator fn =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                o->metatable->metamethods.find("__index");
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (fn != o->metatable->metamethods.end())
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Value args[3];
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Value result;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_undefined(&args[0]);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[0].type = FVM_VALUE_OBJECT;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[0].object = receiver;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[1] = args[0];
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[1].object = o->metatable;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_string(&args[2], name);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_undefined(&result);
                /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
                try
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                {
                    /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в
                     * сборщик. */
                    /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в
                     * сборщик. */
                    FVM_NativeCallGuard native_guard(c);
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    FVM_Result status =
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                           объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                           объектов. */
                        fn->second.callback(c, receiver, 3, args, &result, fn->second.userdata);
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    if (status == FVM_OK)
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        *out = result;
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return fvm_exception_complete(c, receiver, status);
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
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return FVM_ERROR;
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                }
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (m == o->metatable->properties.end())
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                break;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (m->second.type != FVM_VALUE_OBJECT || !m->second.object)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                break;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (m->second.object->invocable.callback)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Value args[3];
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_undefined(args);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[0].type = FVM_VALUE_OBJECT;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[0].object = receiver;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[1] = args[0];
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[1].object = o->metatable;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_string(args + 2, name);
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return fvm_function_invoke(c, m->second.object, receiver, 3, args, out);
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            o = m->second.object;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator it = o->properties.find(name);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (it != o->properties.end())
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                *out = it->second;
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_OK;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        if (depth == 1024)
            return FVM_ERROR;
        fvm_value_undefined(out);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        fvm_value_undefined(out);
        return FVM_OUT_OF_MEMORY;
    }
}

/* Update object no index. */
FVM_Result fvm_object_set_no_index(FVM_Context *c, FVM_Object *o, const char *name,
                                   const FVM_Value *v)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value copy;
    unsigned int depth = 0;
    if (!c || !o || o->owner != c || !name || fvm_validate_value(c, v) != FVM_OK)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    unsigned int flag_depth = 0;
    FVM_Object *flag_cursor = o;
    for (; flag_cursor && flag_depth++ < 1024; flag_cursor = flag_cursor->prototype)
    {
        unsigned int flags = 0;
        FVM_Result flag_result = fvm_object_property_flags(c, flag_cursor, name, &flags);
        if (flag_result != FVM_OK)
            return flag_result;
        if (flags & 4)
            return FVM_OK;
    }
    if (flag_cursor)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    FVM_Value watched;
    FVM_Result r = fvm_property_watch_prepare(c, o, name, v, &watched);
    if (r != FVM_OK)
        return r;
    r = copy_value(c, &watched, &copy);
    if (r != FVM_OK)
        return r;

    /* Initialization / Инициализация локального состояния. */
    /* Own data shadows inherited accessors. An own accessor always wins
     * over prototype data, and setters bind the original receiver. */
    FVM_Object *cursor = o;
    unsigned int accessor_depth = 0;
    while (cursor && accessor_depth++ < 1024)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (cursor->properties.find(name) != cursor->properties.end())
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            break;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator found =
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            cursor->accessors.find(name);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (found != cursor->accessors.end())
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_AccessorEntry entry = found->second;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (!entry.setter)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_OK;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value result;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_value_undefined(&result);
            /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
            try
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в
                 * сборщик. */
                /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в
                 * сборщик. */
                FVM_NativeCallGuard guard(c);
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return fvm_exception_complete(
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    c, o, entry.setter(c, o, 1, &copy, &result, entry.userdata));
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
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        cursor = cursor->prototype;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    if (accessor_depth > 1024)
        return FVM_ERROR;
    /* An existing own property is always written directly. */
    if (o->properties.find(name) == o->properties.end())
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        while (o->metatable && depth++ < 1024)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator m =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                o->metatable->properties.find("__newindex");
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator fn =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                o->metatable->metamethods.find("__newindex");
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (fn != o->metatable->metamethods.end())
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Value args[4], result;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_undefined(&args[0]);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[0].type = FVM_VALUE_OBJECT;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[0].object = o;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[1] = args[0];
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[1].object = o->metatable;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_string(&args[2], name);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[3] = copy;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_undefined(&result);
                /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
                try
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                {
                    /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в
                     * сборщик. */
                    /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в
                     * сборщик. */
                    FVM_NativeCallGuard native_guard(c);
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return fn->second.callback(c, o, 4, args, &result, fn->second.userdata);
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
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (m == o->metatable->properties.end() || m->second.type != FVM_VALUE_OBJECT ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                !m->second.object)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                break;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (m->second.object->invocable.callback)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Value args[4], result;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_undefined(args);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[0].type = FVM_VALUE_OBJECT;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[0].object = o;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[1] = args[0];
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[1].object = o->metatable;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_string(args + 2, name);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[3] = copy;
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return fvm_function_invoke(c, m->second.object, o, 4, args, &result);
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            o = m->second.object;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (o->properties.find(name) != o->properties.end())
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                break;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (depth >= 1024)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_ERROR;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    try
    {
        o->properties[name] = copy;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
    return FVM_OK;
}
extern "C"
{
    /* Initialize every value field to the undefined representation. */
    void fvm_value_undefined(FVM_Value *v)
    {
        if (v)
        {
            std::memset(v, 0, sizeof(*v));
            v->type = FVM_VALUE_UNDEFINED;
            v->string_size = (size_t)-1;
        }
    }
    /* Initialize a numeric value with the supplied double payload. */
    void fvm_value_number(FVM_Value *v, double n)
    {
        if (v)
        {
            fvm_value_undefined(v);
            v->type = FVM_VALUE_NUMBER;
            v->number = n;
        }
    }
    /* Initialize a Boolean value from the supplied truth value. */
    void fvm_value_boolean(FVM_Value *v, int b)
    {
        if (v)
        {
            fvm_value_undefined(v);
            v->type = FVM_VALUE_BOOLEAN;
            v->boolean = !!b;
        }
    }
    /* Compute and return value string. */
    FVM_Result fvm_value_string(FVM_Value *v, const char *s)
    {
        if (!v || !s)
            return FVM_INVALID_ARGUMENT;
        fvm_value_undefined(v);

        /* Computation / Вычисление и обновление состояния. */
        v->type = FVM_VALUE_STRING;
        v->string = s;
        v->string_size = std::strlen(s);
        return FVM_OK;
    }
    /* Create object. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result fvm_object_create(FVM_Context *c, FVM_Object **out)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *o;
        /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
        if (!c || !out || c->destroying || c->gc_releasing)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        *out = 0;
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            o = new (std::nothrow) FVM_Object;
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
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!o)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OUT_OF_MEMORY;

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->owner = c;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->prototype = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->metatable = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->finalized = false;
        /* [GC:] Сбрасываем метку перед очередным проходом достижимости. */
        o->gc_marked = false;
        /* [GC:] Проверяем или инициализируем количество явных корней объекта. */
        o->gc_pins = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->callable.callback = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->callable.userdata = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->resource_result_valid = false;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_undefined(&o->resource_result);

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->resource_origin = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->resource_kind = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->resource = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->release_resource = 0;
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            c->objects.push_back(o);
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        catch (...)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Удаляем запись или область корней после завершения работы с ней. */
            delete o;
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OUT_OF_MEMORY;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        *out = o;
        /* [GC:] Учитываем выделения для планирования следующего прохода сборки. */
        if (c->gc_allocations != (unsigned long)-1)
            /* [GC:] Учитываем выделения для планирования следующего прохода сборки. */
            ++c->gc_allocations;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Update object prototype. */
    FVM_Result fvm_object_set_prototype(FVM_Context *c, FVM_Object *o, FVM_Object *proto)
    {
        FVM_Object *cursor;
        if (!c || !o || o->owner != c || (proto && proto->owner != c))
            return FVM_INVALID_ARGUMENT;
        cursor = proto;
        unsigned int prototype_depth = 0;
        while (cursor && prototype_depth++ < 1024)
        {
            if (cursor == o)
                return FVM_INVALID_ARGUMENT;
            cursor = cursor->prototype;
        }
        if (cursor)
            return FVM_INVALID_ARGUMENT;
        o->prototype = proto;
        return FVM_OK;
    }
    /* Read object prototype. */
    FVM_Result fvm_object_get_prototype(FVM_Context *c, FVM_Object *o, FVM_Object **out)
    {
        if (!c || !o || o->owner != c || !out)
            return FVM_INVALID_ARGUMENT;
        *out = o->prototype;
        return FVM_OK;
    }
    /* Test presence of object own. */
    FVM_Result fvm_object_has_own(FVM_Context *c, FVM_Object *o, const char *name, int *out)
    {
        if (!c || !o || o->owner != c || !name || !out)
            return FVM_INVALID_ARGUMENT;
        try
        {
            unsigned long index;
            if ((o->resource_kind == 28 || o->resource_kind == 30) && fvm_as_index(name, &index))
            {
                if (o->indexed.find(index) != o->indexed.end())
                {
                    *out = 1;
                    return FVM_OK;
                }
            }
            fvm_detail::string key(name);
            *out = o->properties.find(key) != o->properties.end() ||
                   o->accessors.find(key) != o->accessors.end() ||
                   o->methods.find(key) != o->methods.end();
            return FVM_OK;
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
    }
    /* AVM1 prototype lookup takes precedence over Lua-style __index fallback.
     * Native/function-valued hooks and table-valued fallbacks are supported. */
    /* Read a named property, following accessors and the prototype chain. */
    FVM_Result fvm_object_get(FVM_Context *c, FVM_Object *o, const char *name, FVM_Value *out)
    {
        if (!c || !o || o->owner != c || !name || !out)
            return FVM_INVALID_ARGUMENT;
        fvm_value_undefined(out);
        try
        {
            unsigned long index;
            if ((o->resource_kind == 28 || o->resource_kind == 30) && fvm_as_index(name, &index))
                return fvm_table_get_index(c, o, index, out);
            return fvm_object_get_no_index(c, o, name, out);
        }
        catch (const std::bad_alloc &)
        {
            fvm_value_undefined(out);
            return FVM_OUT_OF_MEMORY;
        }
        catch (...)
        {
            fvm_value_undefined(out);
            return FVM_ERROR;
        }
    }
    /* Assign a named property using attributes, accessors and watchers. */
    FVM_Result fvm_object_set(FVM_Context *c, FVM_Object *o, const char *name, const FVM_Value *v)
    {
        if (!c || !o || o->owner != c || !name || fvm_validate_value(c, v) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
        try
        {
            unsigned long index;
            if ((o->resource_kind == 28 || o->resource_kind == 30) && fvm_as_index(name, &index))
                return fvm_table_set_index(c, o, index, v);
            return fvm_object_set_no_index(c, o, name, v);
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
    /* Read global. */
    FVM_Result fvm_get_global(FVM_Context *c, const char *name, FVM_Value *out)
    {
        if (!c)
            return FVM_INVALID_ARGUMENT;
        FVM_Result result = fvm_object_get(c, c->global, name, out);
        if (result != FVM_OK || out->type != FVM_VALUE_UNDEFINED)
            return result;
        try
        {
            if (c->global->properties.find(name) != c->global->properties.end())
                return FVM_OK;
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator fn =
                c->functions.find(name);
            return fn == c->functions.end() ? FVM_OK
                                            : fvm_function_wrap_entry(c, 0, name, fn->second, out);
        }
        catch (const std::bad_alloc &)
        {
            return FVM_OUT_OF_MEMORY;
        }
    }
    /* Update global. */
    FVM_Result fvm_set_global(FVM_Context *c, const char *name, const FVM_Value *v)
    {
        return c ? fvm_object_set(c, c->global, name, v) : FVM_INVALID_ARGUMENT;
    }
    /* Register function. */
    FVM_Result fvm_register_function(FVM_Context *c, const char *name, FVM_NativeFunction cb,
                                     void *user)
    {
        FVM_NativeEntry entry;
        if (!c || !name || !*name || !cb)
            return FVM_INVALID_ARGUMENT;

        /* Computation / Вычисление и обновление состояния. */
        entry.callback = cb;
        entry.userdata = user;
        try
        {
            c->functions[name] = entry;
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }
    /* [GC:] Register function rooted. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result fvm_register_function_rooted(
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Context *c, const char *name, FVM_NativeFunction cb,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        void *user, FVM_Object *root)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || (root && root->owner != c))
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Result r = fvm_register_function(c, name, cb, user);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r == FVM_OK)
            /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
            c->functions.find(name)->second.root = root;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Call object. */
    FVM_Result fvm_object_call(FVM_Context *c, FVM_Object *self, const char *name,
                               unsigned int argc, const FVM_Value *argv, FVM_Value *out)
    {
        unsigned int i;
        if (!c || !self || self->owner != c || !name || !out || (argc && !argv))
            return FVM_INVALID_ARGUMENT;
        for (i = 0; i < argc; ++i)
            if (fvm_validate_value(c, &argv[i]) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
        fvm_value_undefined(out);
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Object *cursor = self;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            unsigned int levels = 0;
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            while (cursor && levels++ < 1024)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (cursor->properties.find(name) != cursor->properties.end() ||
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    cursor->accessors.find(name) != cursor->accessors.end())
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                {
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    FVM_Value member;
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    FVM_Result r = fvm_object_get(c, self, name, &member);
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    if (r != FVM_OK)
                        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                        return r;
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return member.type == FVM_VALUE_OBJECT
                               /* [GC:] Операция в блоке управления достижимостью и временем жизни
                                  объектов. */
                               ? fvm_function_invoke(c, member.object, self, argc, argv, out)
                               /* [GC:] Операция в блоке управления достижимостью и временем жизни
                                  объектов. */
                               : FVM_UNSUPPORTED;
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                }
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (cursor->methods.find(name) != cursor->methods.end())
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    break;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                cursor = cursor->prototype;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_NativeEntry entry;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Result lookup = fvm_lookup_native_method(self, name, &entry);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (lookup != FVM_OK && lookup != FVM_NOT_FOUND)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return lookup;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (lookup == FVM_NOT_FOUND)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator it =
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    c->functions.find(name);
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (it == c->functions.end())
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return FVM_UNSUPPORTED;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                entry = it->second;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_resource_fault_begin(c);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_value_undefined(out);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (entry.collector_entry)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_detail::vector<FVM_Value> args;
                /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
                for (i = 0; i < argc; ++i)
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    args.push_back(argv[i]);
                /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
                FVM_GCRootFrame frame;
                /* [GC:] Сохраняем предыдущий корень для восстановления цепочки после выхода. */
                frame.previous = c->gc_frames;
                /* [GC:] Подключаем стек, локальные значения или получателя к активному корню
                 * вызова. */
                /* [GC:] Подключаем стек, локальные значения или получателя к активному корню
                 * вызова. */
                frame.stack = &args;
                /* [GC:] Подключаем стек, локальные значения или получателя к активному корню
                 * вызова. */
                /* [GC:] Подключаем стек, локальные значения или получателя к активному корню
                 * вызова. */
                frame.locals = 0;
                /* [GC:] Подключаем стек, локальные значения или получателя к активному корню
                 * вызова. */
                /* [GC:] Подключаем стек, локальные значения или получателя к активному корню
                 * вызова. */
                frame.receiver = self;
                /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен быть LIFO.
                 */
                /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен быть LIFO.
                 */
                c->gc_frames = &frame;
                /* Define Pop struct and its representation. */
                /* [GC:] Объявляем представление состояния сборщика или корней. */
                struct Pop
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                {
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    FVM_Context *c;
                    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
                    FVM_GCRootFrame *f;
                    /* Release the resources owned by this Pop instance. */
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    ~Pop()
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                    {
                        /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен
                         * быть LIFO. */
                        /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен
                         * быть LIFO. */
                        c->gc_frames = f->previous;
                        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней.
                         */
                        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней.
                         */
                    }
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                } pop;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                pop.c = c;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                pop.f = &frame;
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return fvm_exception_complete(
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    c, self, entry.callback(c, self, argc, argv, out, entry.userdata));
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик.
             */
            /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик.
             */
            FVM_NativeCallGuard native_guard(c);
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return fvm_exception_complete(
                c, self,
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                entry.callback(c, self, argc, argv, out, entry.userdata));
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        catch (const std::bad_alloc &)
        {
            fvm_value_undefined(out);
            return FVM_OUT_OF_MEMORY;
        }
        catch (...)
        {
            return FVM_ERROR;
        }
    }
    /* [GC:] Define object method rooted. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result fvm_object_define_method_rooted(
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Context *c, FVM_Object *object, const char *name,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeFunction callback, void *userdata,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *root)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !object || object->owner != c || !name || !*name || !callback ||
            /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
            (root && root->owner != c) || c->destroying || c->gc_releasing)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeEntry entry;

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        entry.callback = callback;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        entry.userdata = userdata;
        /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
        entry.root = root;
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::string key(name);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (object->properties.find(key) != object->properties.end() ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                object->accessors.find(key) != object->accessors.end())
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_INVALID_ARGUMENT;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            object->methods[key] = entry;
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
    /* [GC:] Define object accessor rooted. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result fvm_object_define_accessor_rooted(
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Context *c, FVM_Object *object,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        const char *name, FVM_NativeFunction getter,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeFunction setter, void *userdata,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *root)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !object || object->owner != c || !name || !*name || (!getter && !setter) ||
            /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
            (root && root->owner != c) || c->destroying || c->gc_releasing ||
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            object->properties.find(name) != object->properties.end())
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_AccessorEntry entry;

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        entry.getter = getter;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        entry.setter = setter;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        entry.userdata = userdata;
        /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
        entry.root = root;
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            object->accessors[name] = entry;
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
    /* [GC:] Register constructor rooted. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result fvm_register_constructor_rooted(
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Context *c, const char *name,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeFunction callback, void *userdata,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *root)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !name || !*name || !callback || std::strcmp(name, "Object") == 0 ||
            /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
            (root && root->owner != c) || c->destroying || c->gc_releasing)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeEntry entry;

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        entry.callback = callback;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        entry.userdata = userdata;
        /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
        entry.root = root;
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            c->constructors[name] = entry;
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
    /* Compute and return construct. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result
    fvm_construct(FVM_Context *c, const char *name, unsigned int argc,
                  /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                  const FVM_Value *argv, FVM_Object **out)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!out)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        *out = 0;
        /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
        if (!c || !name || (argc && !argv) || c->destroying || c->gc_releasing)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (unsigned int i = 0; i < argc; ++i)
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (fvm_validate_value(c, &argv[i]) != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_INVALID_ARGUMENT;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (std::strcmp(name, "Object") == 0 && !c->standard_library)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return argc ? FVM_UNSUPPORTED : fvm_object_create(c, out);
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator found =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                c->constructors.find(name);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (found == c->constructors.end())
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Value function;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Result r = fvm_get_global(c, name, &function);
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (r != FVM_OK)
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return r;
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return function.type == FVM_VALUE_OBJECT
                           /* [GC:] Операция в блоке управления достижимостью и временем жизни
                              объектов. */
                           /* [GC:] Операция в блоке управления достижимостью и временем жизни
                              объектов. */
                           ? fvm_function_construct(c, function.object, argc, argv, out)
                           /* [GC:] Операция в блоке управления достижимостью и временем жизни
                              объектов. */
                           /* [GC:] Операция в блоке управления достижимостью и временем жизни
                              объектов. */
                           : FVM_UNSUPPORTED;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_NativeEntry entry = found->second;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value result;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_value_undefined(&result);
            /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик.
             */
            /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик.
             */
            FVM_NativeCallGuard guard(c);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_resource_fault_begin(c);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Result status = fvm_exception_complete(
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                c, c->global, entry.callback(c, c->global, argc, argv, &result, entry.userdata));
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (status != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return status;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_ValueType object_type = FVM_VALUE_OBJECT;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (std::memcmp(&result.type, &object_type, sizeof(object_type)) != 0 ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                !result.object || result.object->owner != c)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_ERROR;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            *out = result.object;
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
    /* Call the current value. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_call(FVM_Context *c, const char *name, unsigned int argc, const FVM_Value *argv,
             /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
             FVM_Value *out)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        unsigned int i;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !name || !out || (argc && !argv))
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (i = 0; i < argc; ++i)
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (fvm_validate_value(c, &argv[i]) != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_undefined(out);
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator it =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                c->functions.find(name);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (it == c->functions.end())
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return fvm_stack_call(c, name, argc, argv, out);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_value_undefined(out);
            /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик.
             */
            /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик.
             */
            FVM_NativeCallGuard native_guard(c);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_resource_fault_begin(c);
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return fvm_exception_complete(
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                c, c->global,
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                it->second.callback(c, c->global, argc, argv, out, it->second.userdata));
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
}
