/* FlashVM: C embedding API: stack.
 * File: src/capi/stack_c.cpp.
 */

#include "flashvm/function.h"
#include "../core/native_call_guard.hpp"
#include "flashvm/stack.h"
#include "flashvm/auxlib.h"
#include <climits>
#include "flashvm/callable.h"
#include "../core/runtime.hpp"
#include "../core/property_key.hpp"
#include <new>
#include <cstring>
/* Define FVM_Stack struct and its representation. */
struct FVM_Stack
{
    FVM_Context *context;
    fvm_detail::vector<FVM_Value> values;
};

/* Compute and return valid. */
static int valid(FVM_Context *c, const FVM_Value *v)
{
    return fvm_validate_value(c, v) == FVM_OK;
}
extern "C"
{
    /* Create stack. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result fvm_stack_create(FVM_Context *c, FVM_Stack **out)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !out)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        *out = new (std::nothrow) FVM_Stack;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!*out)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OUT_OF_MEMORY;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        (*out)->context = c;
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Регистрируем временный стек значений среди корней контекста. */
            c->gc_stacks.push_back(&(*out)->values);
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        catch (...)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Удаляем запись или область корней после завершения работы с ней. */
            delete *out;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            *out = 0;
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OUT_OF_MEMORY;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Destroy stack. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    void fvm_stack_destroy(FVM_Stack *s)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!s)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return;
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (size_t i = 0; i < s->context->gc_stacks.size(); ++i)
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (s->context->gc_stacks[i] == &s->values)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Регистрируем временный стек значений среди корней контекста. */
                s->context->gc_stacks.erase(s->context->gc_stacks.begin() + i);
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                break;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
        /* [GC:] Удаляем запись или область корней после завершения работы с ней. */
        delete s;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Compute and return stack size. */
    unsigned int fvm_stack_size(const FVM_Stack *s)
    {
        return s ? (unsigned int)s->values.size() : 0;
    }
    /* Read stack. */
    FVM_Result fvm_stack_get(const FVM_Stack *s, int index, FVM_Value *out)
    {
        long n, pos;
        if (!s || !out || index == 0)
            return FVM_INVALID_ARGUMENT;

        /* Computation / Вычисление и обновление состояния. */
        n = (long)s->values.size();
        pos = index > 0 ? (long)index - 1 : n + (long)index;
        if (pos < 0 || pos >= n)
            return FVM_INVALID_ARGUMENT;
        *out = s->values[(size_t)pos];
        return FVM_OK;
    }
    /* Push stack. */
    FVM_Result fvm_stack_push(FVM_Stack *s, const FVM_Value *v)
    {
        if (!s || !valid(s->context, v))
            return FVM_INVALID_ARGUMENT;
        try
        {
            s->values.push_back(*v);
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }
    /* Pop stack. */
    FVM_Result fvm_stack_pop(FVM_Stack *s, unsigned int count)
    {
        if (!s || count > s->values.size())
            return FVM_INVALID_ARGUMENT;
        while (count--)
            s->values.pop_back();
        return FVM_OK;
    }
    /* Clear stack. */
    FVM_Result fvm_stack_clear(FVM_Stack *s)
    {
        if (!s)
            return FVM_INVALID_ARGUMENT;
        return fvm_stack_pop(s, (unsigned int)s->values.size());
    }
    /* Register stack. */
    FVM_Result fvm_stack_register(FVM_Context *c, const char *name, FVM_StackFunction cb,
                                  void *user)
    {
        return name ? fvm_stack_register_rooted_bytes(c, name, std::strlen(name), cb, user, 0)
                    : FVM_INVALID_ARGUMENT;
    }
    /* Call stack. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result
fvm_stack_call(FVM_Context *c, const char *name, unsigned int argc,
                   const FVM_Value *args, FVM_Value *out)
    {
        return name ? fvm_stack_call_bytes(c, name, std::strlen(name), argc, args, out)
                    : FVM_INVALID_ARGUMENT;
    }
    /* [GC:] Invoke the counted stack registry without truncating the selected key. */
    FVM_Result     fvm_stack_call_bytes(FVM_Context *c, const char *name, size_t size, unsigned int argc,
                   /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                   const FVM_Value *args, FVM_Value *out)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {

        /* Initialization / Инициализация локального состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Stack *s = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Result r;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        unsigned int i;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !name || !out || (argc && !args))
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_StackEntry entry;
        try
        {
            fvm_detail::string key;
            r = fvm_property_key(name, size, key);
            if (r != FVM_OK)
                return r;
            fvm_detail::map<fvm_detail::string, FVM_StackEntry>::iterator found =
                c->stack_functions.find(key);
            if (found == c->stack_functions.end())
                return FVM_UNSUPPORTED;
            /* Copy the entry before user code can replace the registration. */
            entry = found->second;
        }
        catch (const std::bad_alloc &)
        {
            return FVM_OUT_OF_MEMORY;
        }
        r = fvm_stack_create(c, &s);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (i = 0; i < argc; ++i)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            r = fvm_stack_push(s, &args[i]);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (r != FVM_OK)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                break;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r == FVM_OK)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
            try
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в
                 * сборщик. */
                /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в
                 * сборщик. */
                FVM_NativeCallGuard native_guard(c);
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (entry.cfunction)
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                {
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    int count = entry.cfunction(s);
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    if (count < 0)
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        r = (FVM_Result)count;
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    else if (count > 1 || (size_t)count > s->values.size())
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        r = FVM_ERROR;
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    else if (count == 0)
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        s->values.clear();
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    else
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                    {
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        FVM_Value value = s->values.back();
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        s->values.clear();
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        s->values.push_back(value);
                        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней.
                         */
                        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней.
                         */
                    }
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                }
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                else
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    r = entry.callback(c, s, entry.userdata);
                /* [GC:] Normalize the single selected result before its native capture barrier
                 * ends. */
                if (r == FVM_OK && s->values.size() == 1)
                    /* [GC:] Stack results may borrow foreign userdata bytes that later GC will
                     * release. */
                    r = fvm_native_result(c, r, &s->values[0]);
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
            catch (...)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                r = FVM_ERROR;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r == FVM_OK)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (s->values.size() > 1)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                r = FVM_ERROR;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            else if (s->values.size() == 1)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                *out = s->values[0];
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            else
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_undefined(out);
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_stack_destroy(s);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
}

extern "C"
{
    /* Create callable. */
    FVM_Result fvm_callable_create(FVM_Context *c, FVM_StackFunction cb, void *user,
                                   FVM_Object **out)
    {

        /* Initialization / Инициализация локального состояния. */
        FVM_Object *obj = 0;
        FVM_StackEntry entry;
        FVM_Result r;
        if (!c || !cb || !out)
            return FVM_INVALID_ARGUMENT;
        *out = 0;
        r = fvm_object_create(c, &obj);
        if (r != FVM_OK)
            return r;

        /* Computation / Вычисление и обновление состояния. */
        entry.callback = cb;
        entry.userdata = user;
        obj->callable = entry;
        *out = obj;
        return FVM_OK;
    }
    /* Test whether callable. */
    FVM_Result fvm_callable_is(FVM_Context *c, FVM_Object *obj, int *out)
    {
        if (!c || !obj || !out || obj->owner != c)
            return FVM_INVALID_ARGUMENT;
        *out = obj->callable.callback != 0 || obj->invocable.callback != 0;
        return FVM_OK;
    }
    /* Invoke callable. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result fvm_callable_invoke(
        FVM_Context *c, FVM_Object *obj, unsigned int argc,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        const FVM_Value *args, FVM_Value *out)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {

        /* Initialization / Инициализация локального состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Stack *stack = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Result r;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        unsigned int i;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !obj || !out || obj->owner != c || (argc && !args))
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (obj->invocable.callback)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return fvm_function_invoke(c, obj, 0, argc, args, out);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_StackEntry entry = obj->callable;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!entry.callback)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_UNSUPPORTED;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        r = fvm_stack_create(c, &stack);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (i = 0; i < argc; ++i)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            r = fvm_stack_push(stack, &args[i]);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (r != FVM_OK)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                break;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r == FVM_OK)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
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
                r = entry.callback(c, stack, entry.userdata);
                /* [GC:] A callable return must not depend on the lifetime of its native capture. */
                if (r == FVM_OK && stack->values.size() == 1)
                    /* [GC:] Copy foreign strings and validate primitive/object ownership before
                     * return. */
                    r = fvm_native_result(c, r, &stack->values[0]);
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
            catch (...)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                r = FVM_ERROR;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r == FVM_OK)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (stack->values.size() > 1)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                r = FVM_ERROR;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            else if (stack->values.size() == 1)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                *out = stack->values[0];
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            else
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_value_undefined(out);
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_stack_destroy(stack);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
}

/* [GC:] Register stack rooted. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_stack_register_rooted(
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Context *c, const char *name,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_StackFunction cb, void *ud, FVM_Object *root)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    return name ? fvm_stack_register_rooted_bytes(c, name, std::strlen(name), cb, ud, root)
                : FVM_INVALID_ARGUMENT;
}
/* [GC:] Publish a counted stack registration with its traced capture atomically. */
extern "C" FVM_Result fvm_stack_register_rooted_bytes(FVM_Context *c, const char *name, size_t size,
                                                      FVM_StackFunction callback, void *userdata,
                                                      FVM_Object *root)
{
    if (!c || !name || !size || !callback || (root && root->owner != c))
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::string key;
        FVM_Result result = fvm_property_key(name, size, key);
        if (result != FVM_OK)
            return result;
        FVM_StackEntry entry;
        entry.callback = callback;
        entry.userdata = userdata;
        entry.root = root;
        c->stack_functions[key] = entry;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* [GC:] Create callable rooted. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_callable_create_rooted(
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Context *c, FVM_StackFunction cb, void *ud,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *root, FVM_Object **out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!c || (root && root->owner != c))
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result r = fvm_callable_create(c, cb, ud, out);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r == FVM_OK)
        /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
        (*out)->callable.root = root;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return r;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

namespace
{
/* Convert signed public indices without overflowing on INT_MIN. */
/* Read position of stack. */
FVM_Result stack_position(FVM_Stack *stack, int index, size_t *position)
{
    if (!stack || !position || index == 0)
        return FVM_INVALID_ARGUMENT;
    size_t count = stack->values.size();
    if (index > 0)
    {
        *position = (size_t)index - 1;
        return *position < count ? FVM_OK : FVM_INVALID_ARGUMENT;
    }
    size_t distance = (size_t)(-(index + 1)) + 1;
    if (distance > count)
        return FVM_INVALID_ARGUMENT;
    *position = count - distance;
    return FVM_OK;
}
} // namespace

extern "C"
{
    /* Compute and return stack context. */
    FVM_Context *fvm_stack_context(FVM_Stack *stack)
    {
        return stack ? stack->context : 0;
    }

    /* Register stack cfunction. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result fvm_stack_register_cfunction(
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Context *context, const char *name,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_CFunction callback, FVM_Object *root)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        return name ? fvm_stack_register_cfunction_bytes(context, name, std::strlen(name), callback, root) : FVM_INVALID_ARGUMENT;
    }
    /* Preserve complete counted input through the existing stack contract. */
    FVM_Result fvm_stack_register_cfunction_bytes(
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Context *context, const char *name, size_t size,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_CFunction callback, FVM_Object *root)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!context || !name || !size || !callback || (root && root->owner != context))
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_StackEntry entry;

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        entry.cfunction = callback;
        /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
        entry.root = root;
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            fvm_detail::string key;
            FVM_Result normalized = fvm_property_key(name, size, key);
            if (normalized != FVM_OK)
                return normalized;

            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            context->stack_functions[key] = entry;
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
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }

    /* Compute and return stack settop. */
    FVM_Result fvm_stack_settop(FVM_Stack *stack, int index)
    {
        if (!stack)
            return FVM_INVALID_ARGUMENT;

        /* Initialization / Инициализация локального состояния. */
        size_t count = stack->values.size();
        size_t target;
        if (index >= 0)
            target = (size_t)index;
        else
        {
            size_t distance = (size_t)(-(index + 1));
            if (distance > count)
                return FVM_INVALID_ARGUMENT;
            target = count - distance;
        }
        try
        {
            /* Default C++ construction does not initialize FVM_Value PODs. */
            stack->values.reserve(target);
            while (stack->values.size() < target)
            {
                FVM_Value value;
                fvm_value_undefined(&value);
                stack->values.push_back(value);
            }
            while (stack->values.size() > target)
                stack->values.pop_back();
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }

    /* Compute and return stack pushvalue. */
    FVM_Result fvm_stack_pushvalue(FVM_Stack *stack, int index)
    {

        /* Initialization / Инициализация локального состояния. */
        FVM_Value value;
        FVM_Result result = fvm_stack_get(stack, index, &value);
        return result == FVM_OK ? fvm_stack_push(stack, &value) : result;
    }

    /* Remove stack. */
    FVM_Result fvm_stack_remove(FVM_Stack *stack, int index)
    {

        /* Initialization / Инициализация локального состояния. */
        size_t position;
        FVM_Result result = stack_position(stack, index, &position);
        if (result != FVM_OK)
            return result;
        stack->values.erase(stack->values.begin() + position);
        return FVM_OK;
    }

    /* Insert stack. */
    FVM_Result fvm_stack_insert(FVM_Stack *stack, int index)
    {

        /* Initialization / Инициализация локального состояния. */
        size_t position;
        FVM_Result result = stack_position(stack, index, &position);
        if (result != FVM_OK)
            return result;
        FVM_Value top = stack->values.back();
        for (size_t i = stack->values.size() - 1; i > position; --i)
            stack->values[i] = stack->values[i - 1];
        stack->values[position] = top;
        return FVM_OK;
    }

    /* Compute and return stack replace. */
    FVM_Result fvm_stack_replace(FVM_Stack *stack, int index)
    {

        /* Initialization / Инициализация локального состояния. */
        size_t position;
        FVM_Result result = stack_position(stack, index, &position);
        if (result != FVM_OK)
            return result;

        /* Computation / Вычисление и обновление состояния. */
        stack->values[position] = stack->values.back();
        stack->values.pop_back();
        return FVM_OK;
    }

    /* Compute and return stack pushnumber. */
    FVM_Result fvm_stack_pushnumber(FVM_Stack *stack, double number)
    {

        /* Initialization / Инициализация локального состояния. */
        FVM_Value value;
        fvm_value_number(&value, number);
        return fvm_stack_push(stack, &value);
    }

    /* Compute and return stack pushboolean. */
    FVM_Result fvm_stack_pushboolean(FVM_Stack *stack, int boolean)
    {

        /* Initialization / Инициализация локального состояния. */
        FVM_Value value;
        fvm_value_boolean(&value, boolean);
        return fvm_stack_push(stack, &value);
    }

    /* Compute and return stack pushundefined. */
    FVM_Result fvm_stack_pushundefined(FVM_Stack *stack)
    {

        /* Initialization / Инициализация локального состояния. */
        FVM_Value value;
        fvm_value_undefined(&value);
        return fvm_stack_push(stack, &value);
    }

    /* Compute and return stack pushstring. */
    FVM_Result fvm_stack_pushstring(FVM_Stack *stack, const char *text)
    {
        return text ? fvm_stack_pushstring_bytes(stack, text, std::strlen(text)) : FVM_INVALID_ARGUMENT;
    }
    /* Preserve complete counted input through the existing stack contract. */
    FVM_Result fvm_stack_pushstring_bytes(FVM_Stack *stack, const char *text, size_t length)
    {
        if (!stack || !text)
            return FVM_INVALID_ARGUMENT;
        FVM_Value checked;
        FVM_Result validated = fvm_value_string_bytes(&checked, text, length);
        if (validated != FVM_OK)
            return validated;
        if (length == (size_t)-1)
            return FVM_OUT_OF_MEMORY;
        char *copy = new (std::nothrow) char[length + 1];
        if (!copy)
            return FVM_OUT_OF_MEMORY;
        std::memcpy(copy, text, length);
        copy[length] = 0;
        try
        {
            /* [GC:] Register owned bytes and their counted length together; failed insertion leaves
             * caller cleanup active. */
            fvm_context_adopt_string(stack->context, copy, length);
        }
        catch (...)
        {
            delete[] copy;
            return FVM_OUT_OF_MEMORY;
        }

        /* Initialization / Инициализация локального состояния. */
        FVM_Value value;
        fvm_value_string_bytes(&value, copy, length);
        return fvm_stack_push(stack, &value);
    }

    /* Compute and return stack checknumber. */
    FVM_Result fvm_stack_checknumber(FVM_Stack *stack, int index, double *out)
    {
        FVM_Value value;
        if (!out)
            return FVM_INVALID_ARGUMENT;
        FVM_Result result = fvm_stack_get(stack, index, &value);
        if (result != FVM_OK)
            return result;
        if (value.type != FVM_VALUE_NUMBER)
            return FVM_INVALID_ARGUMENT;
        *out = value.number;
        return FVM_OK;
    }

    /* Compute and return stack checkstring. */
    FVM_Result fvm_stack_checkstring(FVM_Stack *stack, int index, const char **out)
    {
        FVM_Value value;
        if (!out)
            return FVM_INVALID_ARGUMENT;
        FVM_Result result = fvm_stack_get(stack, index, &value);
        if (result != FVM_OK)
            return result;
        if (value.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
        *out = value.string;
        return FVM_OK;
    }

    /* Return a borrowed string view together with its complete counted length. */
    FVM_Result fvm_stack_checkstring_bytes(FVM_Stack *stack, int index, const char **out, size_t *size)
    {
        if (!out || !size)
            return FVM_INVALID_ARGUMENT;
        FVM_Value value;
        FVM_Result result = fvm_stack_get(stack, index, &value);
        if (result != FVM_OK)
            return result;
        if (value.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
        *out = value.string;
        *size = fvm_value_string_size(&value);
        return FVM_OK;
    }
    /* Compute and return stack newtable. */
    FVM_Result fvm_stack_newtable(FVM_Stack *stack)
    {
        if (!stack)
            return FVM_INVALID_ARGUMENT;

        /* Initialization / Инициализация локального состояния. */
        FVM_Object *object = 0;
        FVM_Result result = fvm_object_create(stack->context, &object);
        if (result != FVM_OK)
            return result;

        /* Initialization / Инициализация локального состояния. */
        FVM_Value value;
        fvm_value_undefined(&value);

        /* Computation / Вычисление и обновление состояния. */
        value.type = FVM_VALUE_OBJECT;
        value.object = object;
        return fvm_stack_push(stack, &value);
    }

    /* Compute and return stack getfield. */
    FVM_Result fvm_stack_getfield(FVM_Stack *stack, int index, const char *name)
    {
        return name ? fvm_stack_getfield_bytes(stack, index, name, std::strlen(name)) : FVM_INVALID_ARGUMENT;
    }
    /* Preserve complete counted input through the existing stack contract. */
    FVM_Result fvm_stack_getfield_bytes(FVM_Stack *stack, int index, const char *name, size_t size)
    {

        /* Initialization / Инициализация локального состояния. */
        FVM_Value object, value;
        FVM_Result result = fvm_stack_get(stack, index, &object);
        if (result != FVM_OK)
            return result;
        if (object.type != FVM_VALUE_OBJECT || !name)
            return FVM_INVALID_ARGUMENT;
        result = fvm_object_get_bytes(stack->context, object.object, name, size, &value);
        return result == FVM_OK ? fvm_stack_push(stack, &value) : result;
    }

    /* Compute and return stack setfield. */
    FVM_Result fvm_stack_setfield(FVM_Stack *stack, int index, const char *name)
    {
        return name ? fvm_stack_setfield_bytes(stack, index, name, std::strlen(name)) : FVM_INVALID_ARGUMENT;
    }
    /* Preserve complete counted input through the existing stack contract. */
    FVM_Result fvm_stack_setfield_bytes(FVM_Stack *stack, int index, const char *name, size_t size)
    {

        /* Initialization / Инициализация локального состояния. */
        FVM_Value object;
        FVM_Result result = fvm_stack_get(stack, index, &object);
        if (result != FVM_OK)
            return result;
        if (object.type != FVM_VALUE_OBJECT || !name)
            return FVM_INVALID_ARGUMENT;
        result = fvm_object_set_bytes(stack->context, object.object, name, size, &stack->values.back());
        if (result == FVM_OK)
            stack->values.pop_back();
        return result;
    }

    /* Compute and return stack getglobal. */
    FVM_Result fvm_stack_getglobal(FVM_Stack *stack, const char *name)
    {
        return name ? fvm_stack_getglobal_bytes(stack, name, std::strlen(name)) : FVM_INVALID_ARGUMENT;
    }
    /* Preserve complete counted input through the existing stack contract. */
    FVM_Result fvm_stack_getglobal_bytes(FVM_Stack *stack, const char *name, size_t size)
    {
        if (!stack || !name)
            return FVM_INVALID_ARGUMENT;

        /* Initialization / Инициализация локального состояния. */
        FVM_Value value;
        FVM_Result result = fvm_get_global_bytes(stack->context, name, size, &value);
        return result == FVM_OK ? fvm_stack_push(stack, &value) : result;
    }

    /* Compute and return stack setglobal. */
    FVM_Result fvm_stack_setglobal(FVM_Stack *stack, const char *name)
    {
        return name ? fvm_stack_setglobal_bytes(stack, name, std::strlen(name)) : FVM_INVALID_ARGUMENT;
    }
    /* Preserve complete counted input through the existing stack contract. */
    FVM_Result fvm_stack_setglobal_bytes(FVM_Stack *stack, const char *name, size_t size)
    {
        if (!stack || !name || stack->values.empty())
            return FVM_INVALID_ARGUMENT;
        FVM_Result result = fvm_set_global_bytes(stack->context, name, size, &stack->values.back());
        if (result == FVM_OK)
            stack->values.pop_back();
        return result;
    }
}
