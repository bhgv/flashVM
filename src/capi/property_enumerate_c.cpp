/* FlashVM: C embedding API: property enumerate.
 * File: src/capi/property_enumerate_c.cpp.
 */

#include "../core/runtime.hpp"
#include "../core/native_call_guard.hpp"
#include "../standard/base_state.hpp"
#include "flashvm/property.h"
#include "property_enumerate_constants.h"
#include "flashvm/table.h"
#include <cstdio>
#include <new>
namespace
{
/* Add the current value. */
void add(FVM_Object *object, const fvm_detail::string &name,
         fvm_detail::map<fvm_detail::string, bool> &seen,
         fvm_detail::vector<fvm_detail::string> &keys)
{
    if (seen.find(name) != seen.end())
        return;
    /* Nonenumerable own properties still hide inherited names. */
    seen[name] = true;
    fvm_detail::map<fvm_detail::string, unsigned int>::iterator flag =
        object->attributes.find(name);
    if (flag == object->attributes.end() || !(flag->second & FVM_PROPERTY_DONT_ENUM))
        keys.push_back(name);
    if (seen.size() > FVM_PROPERTY_ENUMERATE_MAX_NAMES)
        throw std::bad_alloc();
}
} // namespace
/* Compute and return object enumerate. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_object_enumerate(
    FVM_Context *c, FVM_Object *object,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    int include_prototypes, FVM_Object **out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!out)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    *out = 0;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!c || !object || object->owner != c)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
        FVM_NativeCallGuard snapshot(c);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, bool> seen;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::vector<fvm_detail::string> keys;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        unsigned int depth = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *cursor = object;
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        while (cursor && depth++ < FVM_PROTOTYPE_MAX_DEPTH)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = cursor->indexed.begin();
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 i != cursor->indexed.end(); ++i)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                char name[FVM_PROPERTY_ENUMERATE_INDEX_BYTES];
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                std::sprintf(name, "%lu", i->first);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                add(cursor, fvm_detail::string(name), seen, keys);
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i =
                     /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов.
                      */
                 cursor->properties.begin();
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 i != cursor->properties.end(); ++i)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                add(cursor, i->first, seen, keys);
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator i =
                     /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов.
                      */
                 cursor->methods.begin();
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 i != cursor->methods.end(); ++i)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                add(cursor, i->first, seen, keys);
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator i =
                     /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов.
                      */
                 cursor->accessors.begin();
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                 i != cursor->accessors.end(); ++i)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                add(cursor, i->first, seen, keys);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            cursor = include_prototypes ? cursor->prototype : 0;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (cursor)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_ERROR;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *result = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Result r = fvm_array_create_like(c, 0, (unsigned long)keys.size(), &result, c->swf_loaded);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (size_t i = 0; i < keys.size(); ++i)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value value;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_value_string_bytes(&value, keys[i].data(), keys[i].size());
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            /* Loaded key snapshots define own slots without inherited index setters. */
            r = c->swf_loaded ? fvm_array_store_own(c, result, (unsigned long)i, value)
                              : fvm_table_set_index(c, result, (unsigned long)i, &value);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (r != FVM_OK)
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
