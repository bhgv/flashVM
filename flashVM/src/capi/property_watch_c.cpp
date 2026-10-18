/* FlashVM: C embedding API: property watch.
 * File: src/capi/property_watch_c.cpp.
 */

#include "../core/property_watch.hpp"
#include "../standard/base_state.hpp"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include <cstring>
#include "../core/property_key.hpp"
#include "../core/property_lookup.hpp"
#include "../core/property_watch_constants.h"
#include "../core/call_scope.hpp"
#include <new>
/* Compute and return property watchable. */
bool fvm_property_watchable(FVM_Object *object, const fvm_detail::string &name)
{
    if (object->resource_kind == FVM_RESOURCE_MOVIECLIP)
    {
        const char *fields[] = {"_x",     "_y",       "_xscale", "_yscale", "_rotation",
                                "_alpha", "_visible", "_name",   "enabled", "useHandCursor"};
        for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
            if (name == fvm_detail::string(fields[i]))
                return false;
    }

    /* Loaded script objects accept watchers on absent and virtual properties. */
    if (object->owner->swf_loaded)
        return true;

    /* Initialization / Инициализация локального состояния. */
    unsigned long index = 0;
    const bool indexed = fvm_property_terminated(name) && fvm_as_index(name.c_str(), &index);
    unsigned int depth = 0;
    for (FVM_Object *o = object; o && depth++ < FVM_PROTOTYPE_MAX_DEPTH; o = o->prototype)
    {
        if (indexed && o->indexed.find(index) != o->indexed.end())
            return true;
        if (o->accessors.find(name) != o->accessors.end())
            return false;
        if (o->properties.find(name) != o->properties.end())
            return true;
    }
    return depth < FVM_PROTOTYPE_MAX_DEPTH;
}

namespace
{
/* Read stored watcher data along the prototype chain without getter or metadata execution. */
FVM_Result stored_watch_value(FVM_Context *c, FVM_Object *object, const fvm_detail::string &name,
                              FVM_Value *out)
{
    fvm_value_undefined(out);
    unsigned long index = 0;
    bool indexed = fvm_property_terminated(name) && fvm_as_index(name.c_str(), &index);
    unsigned int depth = 0;
    for (FVM_Object *o = object; o && depth++ < FVM_PROTOTYPE_MAX_DEPTH; o = o->prototype)
    {
        /* Hidden storage does not supply a watcher's old value; continue through prototypes
         * without invoking getters or changing raw own-property membership. */
        if (!fvm_property_available(c, o, name))
            continue;
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator property = fvm_property_find(c, o->properties, name);
        if (property != o->properties.end())
        {
            *out = property->second;
            return FVM_OK;
        }
        if (o->resource_kind == FVM_RESOURCE_ARRAY && o->resource_result_valid &&
            fvm_avm1_keyword_name(name, FVM_WATCH_ARRAY_LENGTH_NAME,
                                   FVM_WATCH_ARRAY_LENGTH_UNITS,
                                   c->swf_info.version >= FVM_SWF_VERSION_7))
        {
            *out = o->resource_result;
            return FVM_OK;
        }
        fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor = fvm_property_find(c, o->accessors, name);
        if (accessor != o->accessors.end())
        {
            fvm_base_accessor_data(accessor->second, *out);
            return FVM_OK;
        }
        if (indexed && (o->resource_kind == FVM_RESOURCE_ARRAY || o->resource_kind == FVM_RESOURCE_OBJECT))
        {
            fvm_detail::map<unsigned long, FVM_Value>::iterator slot = o->indexed.find(index);
            if (slot != o->indexed.end())
            {
                *out = slot->second;
                return FVM_OK;
            }
        }
        /* Builtin lazy method handles are data; materializing one does not invoke its callback. */
        if (fvm_property_find(c, o->methods, name) != o->methods.end())
            return fvm_object_get_bytes(c, o, name.data(), name.size(), out);
    }
    return depth > FVM_PROTOTYPE_MAX_DEPTH ? FVM_ERROR : FVM_OK;
}
}

/* Compute and return property watch prepare. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result fvm_property_watch_prepare(
    FVM_Context *c, FVM_Object *object, const fvm_detail::string &name,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const FVM_Value *requested, FVM_Value *out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!c || !object || object->owner != c || !out ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_validate_value(c, requested) != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    *out = *requested;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator entry =
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_property_find(c, object->watchers, name);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (entry == object->watchers.end() || !fvm_property_watchable(object, name))
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OK;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *descriptor = entry->second;
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (size_t i = 0; i < c->watch_running.size(); ++i)
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (c->watch_running[i] == descriptor)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_OK;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::string key(name);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::vector<FVM_Value> roots;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value root;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_undefined(&root);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        root.type = FVM_VALUE_OBJECT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        root.object = descriptor;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        roots.push_back(root);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        roots.push_back(*requested);
        /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
        FVM_GCRootFrame frame;
        /* [GC:] Сохраняем предыдущий корень для восстановления цепочки после выхода. */
        frame.previous = c->gc_frames;
        /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
        frame.stack = &roots;
        /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
        frame.locals = 0;
        /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
        frame.receiver = object;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->watch_running.push_back(descriptor);
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
            FVM_GCRootFrame *previous;
            /* Release the resources owned by this Pop instance. */
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            ~Pop()
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {

                /* Computation / Вычисление и обновление состояния. */
                /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен быть LIFO.
                 */
                /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен быть LIFO.
                 */
                c->gc_frames = previous;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                c->watch_running.pop_back();
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        } pop;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        pop.c = c;
        /* [GC:] Сохраняем предыдущий корень для восстановления цепочки после выхода. */
        pop.previous = frame.previous;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value args[4], handler, result;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_string_bytes(args, key.data(), key.size());
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Result r = c->swf_loaded ? stored_watch_value(c, object, key, args + 1)
                                     : fvm_object_get_bytes(c, object, key.data(), key.size(), args + 1);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        roots.push_back(args[1]);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        args[2] = *requested;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        r = fvm_object_get(c, descriptor, "userData", args + 3);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        roots.push_back(args[3]);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        r = fvm_object_get(c, descriptor, "handler", &handler);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (handler.type != FVM_VALUE_OBJECT || !handler.object->invocable.callback)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        if (c->swf_loaded)
        {
            /* [GC:] Retain callback/receiver identities while fixing watcher super depth;
             * the surrounding watcher frame separately retains its descriptor and values. */
            FVM_CallScope scope(c, 0, handler.object, object, FVM_WATCH_CALLBACK_SUPER_DEPTH);
            r = fvm_function_invoke(c, handler.object, object, 4, args, &result);
        }
        else
            r = fvm_function_invoke(c, handler.object, object, 4, args, &result);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (result.type == FVM_VALUE_STRING)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            r = fvm_value_to_string(c, &result, out);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (r == FVM_OK)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                out->metatable = result.metatable;
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        *out = result;
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
