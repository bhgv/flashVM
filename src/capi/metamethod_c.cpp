/* FlashVM: C embedding API: metamethod.
 * File: src/capi/metamethod_c.cpp.
 */

#include "../core/native_call_guard.hpp"
#include "../core/runtime.hpp"
#include "flashvm/metamethod.h"
#include "flashvm/avm1.h"
#include "flashvm/table.h"
#include "flashvm/function.h"
#include <cstring>
#include <cstdio>
#include <new>
extern "C"
{
    /* Register meta. */
    FVM_Result fvm_meta_register(FVM_Context *c, FVM_Object *m, const char *name,
                                 FVM_NativeFunction cb, void *user)
    {
        if (!c || !m || m->owner != c || !name || !cb)
            return FVM_INVALID_ARGUMENT;
        if (std::strcmp(name, "__index") && std::strcmp(name, "__newindex") &&
            std::strcmp(name, "__iterate") && std::strcmp(name, "__gc") &&
            std::strcmp(name, "__call") && std::strcmp(name, "__add") &&
            std::strcmp(name, "__sub") && std::strcmp(name, "__mul") &&
            std::strcmp(name, "__div") && std::strcmp(name, "__mod") &&
            std::strcmp(name, "__pow") && std::strcmp(name, "__unm") && std::strcmp(name, "__eq") &&
            std::strcmp(name, "__lt") && std::strcmp(name, "__le") &&
            std::strcmp(name, "__concat") && std::strcmp(name, "__tostring"))
            return FVM_UNSUPPORTED;
        FVM_NativeEntry entry;

        /* Computation / Вычисление и обновление состояния. */
        entry.callback = cb;
        entry.userdata = user;
        try
        {
            m->metamethods[name] = entry;
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }
    /* Compute and return meta finalize. */
    /* [GC:] Вызываем финализатор; он может сделать объект снова достижимым. */
    FVM_Result fvm_meta_finalize(FVM_Context *c, FVM_Object *o)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !o || o->owner != c)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (o->finalized)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OK;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->finalized = true;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *m = o->metatable;
        /* [GC:] A missing metatable needs neither key construction nor finalizer dispatch. */
        if (!m)
            return FVM_OK;

        /* [GC:] Resolve the reserved key without allocating a temporary fvm_detail::string. */
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator it = m->metamethods.begin();
        /* [GC:] Match the counted key exactly, excluding embedded-NUL or suffix lookalikes. */
        while (it != m->metamethods.end() &&
               (it->first.size() != 4 || std::memcmp(it->first.c_str(), "__gc", 4)))
            ++it;
        /* [GC:] Script finalizer resolution must also be safe when the allocator rejects every
         * request. */
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator script = m->properties.begin();
        /* [GC:] Inspect existing property metadata without allocating or invoking user code. */
        while (script != m->properties.end() &&
               (script->first.size() != 4 || std::memcmp(script->first.c_str(), "__gc", 4)))
            ++script;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (it == m->metamethods.end() &&
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            (script == m->properties.end() ||
             (script->second.type != FVM_VALUE_STRING &&
              /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
              (script->second.type != FVM_VALUE_OBJECT ||
               /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
               !script->second.object->invocable.callback))))
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OK;

        /* Initialization / Инициализация локального состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value args[3], result;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_undefined(&args[0]);

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        args[0].type = FVM_VALUE_OBJECT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        args[0].object = o;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        args[1] = args[0];
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        args[1].object = m;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        args[2] = args[0];
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_undefined(&result);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (it == m->metamethods.end())
        {
            /* [GC:] C API teardown must not leak C++ allocation exceptions from script preparation.
             */
            try
            {
                /* [GC:] Dispatch the previously resolved script finalizer without another key
                 * lookup. */
                return script->second.type == FVM_VALUE_OBJECT
                           /* [GC:] Операция в блоке управления достижимостью и временем жизни
                            * объектов.
                            */
                           ? fvm_function_invoke(c, script->second.object, o, 3, args, &result)
                           /* [GC:] Операция в блоке управления достижимостью и временем жизни
                            * объектов.
                            */
                           : fvm_avm1_call(c, script->second.string, o, 3, args, &result);
            }
            /* [GC:] Report preparation allocation failure through the C result contract. */
            catch (const std::bad_alloc &)
            {
                return FVM_OUT_OF_MEMORY;
            }
            /* [GC:] Translate unexpected dispatch exceptions rather than unwinding through C
             * callers. */
            catch (...)
            {
                return FVM_ERROR;
            }
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeEntry entry = it->second;
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик.
             */
            /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик.
             */
            FVM_NativeCallGuard native_guard(c);
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return entry.callback(c, o, 3, args, &result, entry.userdata);
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
    /* Compute and return iterate. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    static FVM_Result
    iterate(FVM_Context *c, FVM_Object *o, unsigned long index, FVM_Value *key,
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value *value)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !o || o->owner != c || !key || !value)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;

        /* Initialization / Инициализация локального состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_undefined(key);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_undefined(value);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *m = o->metatable;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (m)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator it =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                m->metamethods.find("__iterate");
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (it != m->metamethods.end())
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Value args[3];
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_undefined(&args[0]);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[0].type = FVM_VALUE_OBJECT;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[0].object = o;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[1] = args[0];
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[1].object = m;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_number(&args[2], (double)index);
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
                    return fvm_native_result(
                        c, it->second.callback(c, o, 3, args, value, it->second.userdata), value);
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
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* Default own iteration keeps numeric keys and skips DONT_ENUM slots. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        unsigned long position = 0;
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = o->indexed.begin();
             /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
             i != o->indexed.end(); ++i)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            char name[32];
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            std::sprintf(name, "%lu", i->first);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, unsigned int>::iterator flag =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                o->attributes.find(name);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (flag != o->attributes.end() && (flag->second & 1))
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                continue;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (position++ == index)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_number(key, (double)i->first);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                *value = i->second;
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_OK;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i = o->properties.begin();
             /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
             i != o->properties.end(); ++i)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, unsigned int>::iterator flag =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                o->attributes.find(i->first);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (flag != o->attributes.end() && (flag->second & 1))
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                continue;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (position++ == index)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_string(key, i->first.c_str());
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                *value = i->second;
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_OK;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
}

/* Compute and return meta iterate. */
extern "C" FVM_Result fvm_meta_iterate(FVM_Context *c, FVM_Object *o, unsigned long index,
                                       FVM_Value *key, FVM_Value *value)
{
    try
    {
        return iterate(c, o, index, key, value);
    }
    catch (const std::bad_alloc &)
    {
        if (key)
            fvm_value_undefined(key);
        if (value)
            fvm_value_undefined(value);
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        if (key)
            fvm_value_undefined(key);
        if (value)
            fvm_value_undefined(value);
        return FVM_ERROR;
    }
}

/* [GC:] Register meta rooted. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_meta_register_rooted(
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Context *c, FVM_Object *m, const char *name,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeFunction cb, void *ud, FVM_Object *root)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!c || (root && root->owner != c))
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result r = fvm_meta_register(c, m, name, cb, ud);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r == FVM_OK)
        /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
        m->metamethods.find(name)->second.root = root;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return r;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}
