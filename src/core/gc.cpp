/* FlashVM: Interpreter state, object lifetimes and shared runtime helpers: gc.
 * File: src/core/gc.cpp.
 */

/* [GC:] Подключаем объявления, используемые сборщиком и защитой корней. */
#include "../core/runtime.hpp"
/* [GC:] Optional string plans share the object root and resurrection traversals. */
#include "string_gc.hpp"
/* [GC:] Reachable native TextFields contribute their implicit text buffer root. */
#include "../standard/text_state.hpp"
/* [GC:] Подключаем объявления, используемые сборщиком и защитой корней. */
#include "flashvm/gc.h"
/* [GC:] Подключаем объявления, используемые сборщиком и защитой корней. */
#include "flashvm/metamethod.h"
/* [GC:] Подключаем объявления, используемые сборщиком и защитой корней. */
#include <climits>

/* Iterative marking avoids recursion overflow on deep/cyclic object graphs. */
/* Mark object. */
/* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
static void mark_object(FVM_Object *o, fvm_detail::vector<FVM_Object *> &pending)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!o || o->gc_marked)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Отмечаем объект, чтобы не обходить его повторно при циклических ссылках. */
    o->gc_marked = true;
    /* [GC:] Работаем с очередью объектов для итеративного обхода графа. */
    pending.push_back(o);
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Mark value. */
/* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
static void mark_value(const FVM_Value &value, fvm_detail::vector<FVM_Object *> &pending,
                       FVM_StringGCPlan &strings)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (value.type == FVM_VALUE_OBJECT)
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(value.object, pending);
    /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
    mark_object(value.metatable, pending);
    /* [GC:] Primitive strings retain their containing context buffer, including interior views. */
    if (value.type == FVM_VALUE_STRING)
        strings.mark(value.string);
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Mark graph. */
/* [GC:] Обходим корни и все достижимые ссылки, включая циклические графы. */
static void mark_graph(FVM_Context *c, FVM_StringGCPlan &strings, size_t newborn_start = (size_t)-1,
                       bool retain_previous = false)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Работаем с очередью объектов для итеративного обхода графа. */
    fvm_detail::vector<FVM_Object *> pending;
    /* [GC:] Rescan earlier survivors after native release without unmarking unreleased resources.
     */
    if (retain_previous)
        /* [GC:] Existing survivors may have gained late edges even when their marks are unchanged.
         */
        for (size_t i = 0; i < c->objects.size(); ++i)
            /* [GC:] Preserve the earlier live set until a later collection can release detached
             * nodes. */
            if (c->objects[i]->gc_marked)
                /* [GC:] Force traversal of changed properties rather than skipping an
                 * already-marked root. */
                pending.push_back(c->objects[i]);
    /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
    mark_object(c->global, pending);
    /* [GC:] Retain the active function while nested calls prepare their caller property. */
    mark_object(c->script_callee, pending);
    /* [GC:] Native Array results retain the original prototype after public prototype replacement. */
    mark_object(c->array_builtin_prototype, pending);
    /* [GC:] A registered trace sink keeps its capture alive between ActionTrace calls. */
    mark_object(c->avm1_trace.root, pending);
    /* [GC:] Host URL registration retains its capture between requests. */
    mark_object(c->avm1_url.root, pending);
    /* [GC:] Current display targets remain live between target changes and nested calls. */
    mark_object(c->avm1_base_clip, pending);
    mark_object(c->avm1_target_clip, pending);
    /* [GC:] Keep invocation metadata reachable during callback-triggered collection. */
    mark_object(c->calling_constructor, pending);
    /* [GC:] Retain the function used to resolve the current method owner. */
    mark_object(c->calling_function, pending);
    /* [GC:] Retain a prototype that callbacks may detach during execution. */
    mark_object(c->calling_owner, pending);
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (size_t j = 0; j < c->joining_arrays.size(); ++j)
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(c->joining_arrays[j], pending);
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (size_t j = 0; j < c->lexical_scopes.size(); ++j)
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(c->lexical_scopes[j], pending);
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (size_t j = 0; j < c->watch_running.size(); ++j)
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(c->watch_running[j], pending);
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator f =
             /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
         c->function_objects.begin();
         /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
         f != c->function_objects.end(); ++f)
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(f->second, pending);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (c->exception_pending)
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_value(c->exception_value, pending, strings);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_StackEntry>::iterator stack_fn;
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (stack_fn = c->stack_functions.begin(); stack_fn != c->stack_functions.end(); ++stack_fn)
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(stack_fn->second.root, pending);

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    size_t i;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator fn;
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (fn = c->functions.begin(); fn != c->functions.end(); ++fn)
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(fn->second.root, pending);
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (fn = c->constructors.begin(); fn != c->constructors.end(); ++fn)
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(fn->second.root, pending);
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (i = 0; i < c->gc_stacks.size(); ++i)
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (size_t j = 0; j < c->gc_stacks[i]->size(); ++j)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Регистрируем временный стек значений среди корней контекста. */
            const FVM_Value &v = (*c->gc_stacks[i])[j];
            /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
            mark_value(v, pending, strings);
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (i = 0; i < c->objects.size(); ++i)
        /* [GC:] Проверяем или инициализируем количество явных корней объекта. */
        if (i >= newborn_start || c->objects[i]->gc_pins)
            /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
            mark_object(c->objects[i], pending);
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (FVM_GCRootFrame *frame = c->gc_frames; frame; frame = frame->previous)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(frame->receiver, pending);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (frame->stack)
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (size_t j = 0; j < frame->stack->size(); ++j)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                const FVM_Value &v = (*frame->stack)[j];
                /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
                mark_value(v, pending, strings);
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (frame->locals)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator it;
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (it = frame->locals->begin(); it != frame->locals->end(); ++it)
                /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
                mark_value(it->second, pending, strings);
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    while (!pending.empty())
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Работаем с очередью объектов для итеративного обхода графа. */
        FVM_Object *o = pending.back();
        /* [GC:] Работаем с очередью объектов для итеративного обхода графа. */
        pending.pop_back();
        /* [GC:] Mark native TextField storage that is not represented by ordinary properties. */
        strings.mark(fvm_textfield_string_root(o));
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (o->resource_result_valid)
            /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
            mark_value(o->resource_result, pending, strings);
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(o->callable.root, pending);
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(o->invocable.root, pending);
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(o->constructable.root, pending);
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator f =
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
             o->method_values.begin();
             /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
             f != o->method_values.end(); ++f)
            /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
            mark_object(f->second, pending);
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator watcher =
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
             o->watchers.begin();
             /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
             watcher != o->watchers.end(); ++watcher)
            /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
            mark_object(watcher->second, pending);
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor =
                 /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
             o->accessors.begin();
             /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
             accessor != o->accessors.end(); ++accessor)
            /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
            mark_object(accessor->second.root, pending);
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(o->prototype, pending);
        /* [GC:] Interface inheritance adds constructor edges to the reachable graph. */
        for (size_t j = 0; j < o->implemented_interfaces.size(); ++j)
            /* [GC:] Retain each implemented interface and its transitive declarations. */
            mark_object(o->implemented_interfaces[j], pending);
        /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
        mark_object(o->metatable, pending);
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (fn = o->methods.begin(); fn != o->methods.end(); ++fn)
            /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
            mark_object(fn->second.root, pending);
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (fn = o->metamethods.begin(); fn != o->metamethods.end(); ++fn)
            /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
            mark_object(fn->second.root, pending);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator p;
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (p = o->properties.begin(); p != o->properties.end(); ++p)
            /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
            mark_value(p->second, pending, strings);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<unsigned long, FVM_Value>::iterator q;
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (q = o->indexed.begin(); q != o->indexed.end(); ++q)
            /* [GC:] Добавляем достижимый объект или значение в очередь маркировки. */
            mark_value(q->second, pending, strings);
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C"
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* Set collector scheduling mode, or request a full collection. */
    /* [GC:] Работаем с выбранным режимом планирования сборки. */
    FVM_Result fvm_gc_mode(FVM_Context *c, int mode)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || mode < -100 || mode > FVM_GC_FULL)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (mode > 100 && mode != FVM_GC_SUSPENDED && mode != FVM_GC_FULL)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (mode == FVM_GC_FULL)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return fvm_gc_collect(c, 0);
        /* [GC:] Работаем с выбранным режимом планирования сборки. */
        c->gc_mode = mode;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Read the collector scheduling mode without running collection. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_Result fvm_gc_state(FVM_Context *c, int *mode)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !mode)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Работаем с выбранным режимом планирования сборки. */
        *mode = c->gc_mode;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Add one explicit host root to an object in this context. */
    /* [GC:] Работаем со счётчиком явных корней хоста для управляемого объекта. */
    FVM_Result fvm_gc_pin(FVM_Context *c, FVM_Object *o)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
        if (!c || !o || c->gc_releasing || o->owner != c || o->gc_pins == ULONG_MAX)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Работаем со счётчиком явных корней хоста для управляемого объекта. */
        ++o->gc_pins;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Read the number of explicit host roots retaining this object. */
    /* [GC:] Работаем со счётчиком явных корней хоста для управляемого объекта. */
    FVM_Result fvm_gc_pin_count(FVM_Context *c, FVM_Object *o, unsigned long *count)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !o || !count || o->owner != c)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Проверяем или инициализируем количество явных корней объекта. */
        *count = o->gc_pins;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Remove one explicit host root; reject an unbalanced release. */
    /* [GC:] Снимаем одну явную удерживающую ссылку хоста. */
    FVM_Result fvm_gc_unpin(FVM_Context *c, FVM_Object *o)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
        if (!c || !o || c->gc_releasing || o->owner != c || !o->gc_pins)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Снимаем одну явную удерживающую ссылку хоста. */
        --o->gc_pins;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Read the current number of managed objects in the context. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_Result fvm_gc_object_count(FVM_Context *c, unsigned long *n)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !n)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        *n = (unsigned long)c->objects.size();
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Mark roots, finalize unreachable objects, mark resurrected graphs and sweep. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_Result fvm_gc_collect(FVM_Context *c, unsigned long *reclaimed)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
        if (!c || c->gc_collecting || c->gc_native_depth)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (reclaimed)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            *reclaimed = 0;
        /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
        c->gc_collecting = true;

        /* Initialization / Инициализация локального состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        size_t i;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        const size_t original_count = c->objects.size();
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Result finalizer_result = FVM_OK;
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Prepare optional string tracing before finalizers or heap mutation. */
            FVM_StringGCPlan strings(c);
            /* [GC:] Initial object traversal begins with no marked context strings. */
            strings.reset();
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (i = 0; i < c->objects.size(); ++i)
                /* [GC:] Сбрасываем метку перед очередным проходом достижимости. */
                c->objects[i]->gc_marked = false;
            /* [GC:] Обходим корни и все достижимые ссылки, включая циклические графы. */
            mark_graph(c, strings);
            /* [GC:] Finalizers may resurrect objects by placing them into the global graph.
               Mark again after all finalizers, before deleting anything. */
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (i = 0; i < original_count; ++i)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Object *o = c->objects[i];
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (!o->gc_marked)
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                {
                    /* [GC:] Вызываем финализатор; он может сделать объект снова достижимым. */
                    FVM_Result r = fvm_meta_finalize(c, o);
                    /* [GC:] Do not sweep a finalizer lost to allocation failure before body entry.
                     */
                    if (r == FVM_OUT_OF_MEMORY && !o->finalized)
                        /* [GC:] Abort through the existing collection rollback/state-reset handler.
                         */
                        throw std::bad_alloc();
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    if (finalizer_result == FVM_OK && r != FVM_OK)
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        finalizer_result = r;
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                }
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (i = 0; i < c->objects.size(); ++i)
                /* [GC:] Сбрасываем метку перед очередным проходом достижимости. */
                c->objects[i]->gc_marked = false;
            /* Snapshot bounds finalization; newborn graphs survive this pass. */
            /* [GC:] Обходим корни и все достижимые ссылки, включая циклические графы. */
            /* [GC:] Discard obsolete string roots before tracing finalizer resurrection. */
            strings.reset();
            /* [GC:] Trace old and newborn managed objects with their string values. */
            mark_graph(c, strings, original_count);
            /* Resource callbacks see a coherent heap: do not delete objects until
               every unreachable resource has been released. */
            /* [GC:] A late graph pass is necessary only if native release callbacks can publish
             * roots. */
            bool released_resources = false;
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (i = 0; i < c->objects.size(); ++i)
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (!c->objects[i]->gc_marked)
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                {
                    /* [GC:] Освобождаем ресурс объекта до удаления его записи из кучи. */
                    /* [GC:] Record callback execution before release clears its function pointer.
                     */
                    if (c->objects[i]->release_resource)
                        /* [GC:] Native cleanup may publish object/string roots after resurrection
                         * remark. */
                        released_resources = true;
                    /* [GC:] Release native payloads while every managed graph node is still
                     * allocated. */
                    FVM_Result r = fvm_release_object_resource(c, c->objects[i]);
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    if (finalizer_result == FVM_OK && r != FVM_OK)
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        finalizer_result = r;
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            /* [GC:] Preserve late object and string roots, including edges added to old survivors.
             */
            if (released_resources)
                /* [GC:] Keep released object shells reachable while native resources stay
                 * irreversibly closed. */
                mark_graph(c, strings, original_count, true);
            /* [GC:] Count object reclamation separately from string-buffer reclamation. */
            unsigned long freed = 0;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            size_t kept = 0;
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (i = 0; i < c->objects.size(); ++i)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Object *o = c->objects[i];
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (o->gc_marked)
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                {
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    c->objects[kept++] = o;
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    continue;
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                }
                /* [GC:] Удаляем запись или область корней после завершения работы с ней. */
                delete o;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                ++freed;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            c->objects.resize(kept);
            /* [GC:] Native release callbacks and object destructors finish before string release.
             */
            strings.sweep();
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (reclaimed)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                *reclaimed = freed;
            /* [GC:] Учитываем выделения для планирования следующего прохода сборки. */
            c->gc_allocations = 0;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        catch (...)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
            c->gc_collecting = false;
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OUT_OF_MEMORY;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
        c->gc_collecting = false;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return finalizer_result;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Read the allocation count used by automatic collection scheduling. */
    /* [GC:] Работаем с очередью объектов для итеративного обхода графа. */
    FVM_Result fvm_gc_pending_allocations(FVM_Context *c, unsigned long *count)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !count)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Учитываем выделения для планирования следующего прохода сборки. */
        *count = c->gc_allocations;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Compute and return gc threshold. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_Result fvm_gc_threshold(FVM_Context *c, unsigned long *threshold)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !threshold)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Работаем с выбранным режимом планирования сборки. */
        if (c->gc_mode == FVM_GC_SUSPENDED)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            *threshold = ULONG_MAX;
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OK;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Работаем с выбранным режимом планирования сборки. */
        *threshold = (unsigned long)(1000 - c->gc_mode * 9);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Snapshot collector mode, object counts, roots and allocation accounting. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_Result fvm_gc_stats(FVM_Context *c, FVM_GCStats *out)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !out || out->struct_size != sizeof(FVM_GCStats))
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        out->live_objects = (unsigned long)c->objects.size();
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        out->pinned_objects = 0;
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (size_t i = 0; i < c->objects.size(); ++i)
            /* [GC:] Проверяем или инициализируем количество явных корней объекта. */
            if (c->objects[i]->gc_pins)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                ++out->pinned_objects;

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Учитываем выделения для планирования следующего прохода сборки. */
        out->pending_allocations = c->gc_allocations;
        /* [GC:] Работаем с выбранным режимом планирования сборки. */
        out->mode = c->gc_mode;
        /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
        out->collecting = c->gc_collecting ? 1 : 0;
        /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
        out->native_depth = c->gc_native_depth;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return fvm_gc_threshold(c, &out->threshold);
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Run a collection at a safe point when the scheduling threshold is reached. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_Result fvm_gc_step(FVM_Context *c, unsigned long *reclaimed)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (reclaimed)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            *reclaimed = 0;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (c->gc_native_depth)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OK;
        /* [GC:] Работаем с выбранным режимом планирования сборки. */
        if (c->gc_mode == FVM_GC_SUSPENDED)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OK;
        /* The allocation threshold is monotonic with the tuning setting. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        unsigned long threshold = 0;
        /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
        fvm_gc_threshold(c, &threshold);
        /* [GC:] Учитываем выделения для планирования следующего прохода сборки. */
        if (c->gc_allocations < threshold)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OK;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return fvm_gc_collect(c, reclaimed);
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}
