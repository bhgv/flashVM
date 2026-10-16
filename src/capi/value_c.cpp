/* FlashVM: C embedding API: value.
 * File: src/capi/value_c.cpp.
 */

#include "../standard/function_state.hpp"
#include "flashvm/function.h"
#include "../standard/base_state.hpp"
#include "../core/property_watch.hpp"
#include "../core/property_resolve.hpp"
#include "../core/property_resolve_constants.h"
#include "../core/member_call_constants.h"
#include "../core/function_call_constants.h"
#include "../core/watch_write_scope.hpp"
#include "flashvm/table.h"
#include "flashvm/property.h"
#include "../core/native_call_guard.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/value_vector_root_scope.hpp"
#include "../core/runtime.hpp"
#include "../core/prototype_data.hpp"
#include "../core/exception_internal.hpp"
#include "flashvm/stack.h"
#include <cstring>
#include "../core/string_internal.hpp"
#include "../core/property_key.hpp"
#include "../core/property_lookup.hpp"
#include "../core/property_write.hpp"
#include <new>
#include "../core/call_scope.hpp"
#include "../core/setter_scope.hpp"
#include "../standard/super.hpp"
#include "../avm1/scope_keywords.hpp"
#include "../standard/movieclip_state.hpp"
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
            /* [GC:] Register owned bytes and their counted length together; failed insertion leaves
             * caller cleanup active. */
            fvm_context_adopt_string(context, p, n);
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
FVM_Result fvm_lookup_native_method(FVM_Object *object, const fvm_detail::string &name, FVM_NativeEntry *out)
{
    if (!object || !out)
        return FVM_INVALID_ARGUMENT;
    try
    {
        const fvm_detail::string &key = name;
        for (unsigned int depth = 0; object && depth < FVM_PROTOTYPE_MAX_DEPTH; ++depth)
        {
            if (!fvm_property_available(object->owner, object, key))
            {
                object = object->prototype;
                continue;
            }
            /* Function-valued data/accessor dispatch is still unsupported. A data
             * assignment must nevertheless shadow a previously registered method. */
            if (fvm_property_find(object->owner, object->properties, key) != object->properties.end() ||
                fvm_property_find(object->owner, object->accessors, key) != object->accessors.end())
                return FVM_UNSUPPORTED;
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method =
                fvm_property_find(object->owner, object->methods, key);
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

/* Read a selected prototype chain with a separate accessor receiver. */
FVM_Result fvm_object_get_from(FVM_Context *c, FVM_Object *o, FVM_Object *receiver,
                               const fvm_detail::string &name, FVM_Value *out, bool allow_meta, bool allow_resolve)
{

    /* Initialization / Инициализация локального состояния. */
    unsigned int depth = 0;
    if (!c || !o || o->owner != c || !receiver || receiver->owner != c || !out)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    unsigned long index = 0;
    const bool indexed_name = fvm_property_terminated(name) && fvm_as_index(name.c_str(), &index);
    FVM_Object *start = o;
    try
    {
        while (o && depth++ < FVM_PROTOTYPE_MAX_DEPTH)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* Version-hidden own slots do not stop visible prototype lookup. */
            if (!fvm_property_available(c, o, name))
            {
                o = o->prototype;
                continue;
            }
            /* Selected ancestor reads, including super, share ordinary indexed precedence. */
            if (indexed_name && (o->resource_kind == FVM_RESOURCE_ARRAY || o->resource_kind == FVM_RESOURCE_OBJECT))
            {
                fvm_detail::map<unsigned long, FVM_Value>::iterator slot = o->indexed.find(index);
                if (slot != o->indexed.end())
                {
                    *out = slot->second;
                    return FVM_OK;
                }
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator it = fvm_property_find(c, o->properties, name);
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
                fvm_property_find(c, o->accessors, name);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (accessor != o->accessors.end() &&
                !fvm_movieclip_hidden_path_accessor(c, accessor->second))
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
                    /* [GC:] Script getters may detach their descriptor/prototype and collect.
                     * Retain the exact entry snapshot and both lookup/receiver identities. */
                    FVM_ObjectRootScope receiver_roots(c, receiver);
                    FVM_ObjectRootScope owner_roots(c, o);
                    FVM_ObjectRootScope capture_roots(c, entry.root);
                    FVM_NativeCallGuard guard(c, !fvm_base_script_accessor(entry));
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
                        receiver,
                        fvm_native_result(c, entry.getter(c, receiver, 0, 0, out, entry.userdata),
                                          out));
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
                fvm_property_find(c, o->methods, name);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (method != o->methods.end())
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return fvm_function_wrap_entry(c, o, method->first, method->second, out);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            /* Intrinsic display paths follow ordinary own members, before prototypes. */
            int intrinsic = 0;
            FVM_Result path_status = fvm_avm1_movieclip_path_property(c, o, name, out, &intrinsic);
            if (path_status != FVM_OK || intrinsic)
                return path_status;
            o = o->prototype;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        if (o)
            return FVM_ERROR;
        if (allow_resolve && c->swf_loaded)
        {
            bool handled = false;
            FVM_Result status = fvm_property_resolve(c, start, receiver, name, out, &handled);
            if (status != FVM_OK || handled)
                return status;
        }
        if (!allow_meta)
            return FVM_OK;
        o = receiver;
        for (depth = 0; o && depth < FVM_PROTOTYPE_MAX_DEPTH; ++depth)
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
                fvm_value_string_bytes(&args[2], name.data(), name.size());
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
                    /* [GC:] Snapshot an index result before the metatable/receiver roots unwind. */
                    status = fvm_native_result(c, status, &result);
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
                fvm_value_string_bytes(args + 2, name.data(), name.size());
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
/* Read an ordinary property chain using its original receiver and metadata fallbacks. */
FVM_Result fvm_object_get_no_index(FVM_Context *c, FVM_Object *o, const fvm_detail::string &name, FVM_Value *out)
{
    return fvm_object_get_from(c, o, o, name, out, true);
}

/* Write an ordinary property using accessors, attributes and metadata hooks. */
FVM_Result fvm_object_set_from(FVM_Context *c, FVM_Object *o, FVM_Object *start, const fvm_detail::string &requested_name,
                               const FVM_Value *v, bool allow_meta, bool prepared_watch, bool super_write)
{

    /* Initialization / Инициализация локального состояния. */
    fvm_detail::string name(requested_name);
    FVM_Value copy;
    unsigned int depth = 0;
    if (!c || !o || o->owner != c || fvm_validate_value(c, v) != FVM_OK)
        return FVM_INVALID_ARGUMENT;

    /* Loaded Object.set ignores an empty name before descriptor, watcher or metadata dispatch. */
    if (c->swf_loaded && name.empty())
        return FVM_OK;

    /* [GC:] Watchers may detach the selected prototype lookup owner; retain it through re-resolution. */
    FVM_ObjectRootScope lookup_roots(c, start);
    /* [GC:] Deferred exception roots must unwind before the lookup-owner frame. */
    FVM_WatchWriteScope write(c);
    FVM_Value watched = *v;
    FVM_Result r = FVM_OK;
    if (c->swf_loaded && !prepared_watch)
    {
        r = write.defer(fvm_property_watch_prepare(c, o, name, v, &watched), watched);
        if (r != FVM_OK)
            return r;
    }

    /* Loaded super forwards watcher and membership to the actual receiver,
     * but its local storage is inert even when the receiver owns a virtual. */
    const bool loaded_super = c->swf_loaded && super_write;
    if (loaded_super)
    {
        int own = 0;
        r = fvm_object_has_own_bytes(c, o, name.data(), name.size(), &own);
        if (r != FVM_OK)
            return r;
        if (own)
            return write.finish(FVM_OK);
    }

    /* Watchers observe requested spelling and may replace the own property.
     * Select the current stored spelling only after their callback has returned. */
    if (!fvm_property_select_own_name(c, o, name) &&
        name != fvm_detail::string("__proto__") &&
        fvm_object_prototype_name(c, name) && fvm_object_intrinsic_prototype(c, o, &name))
        name = "__proto__";

    /* Canonical table writes already prepared growth before entering this fallback. */
    if (c->swf_loaded && !loaded_super && start == o && !prepared_watch)
    {
        r = fvm_array_prepare_index(c, o, name);
        if (r != FVM_OK)
            return r;
    }

    /* Initialization / Инициализация локального состояния. */
    unsigned int flag_depth = 0;
    FVM_Object *flag_cursor = loaded_super ? 0 : start;
    for (; flag_cursor && flag_depth++ < FVM_PROTOTYPE_MAX_DEPTH; flag_cursor = flag_cursor->prototype)
    {
        unsigned int flags = 0;
        FVM_Result flag_result = fvm_object_property_flags_bytes(c, flag_cursor, name.data(), name.size(), &flags);
        if (flag_result != FVM_OK)
            return flag_result;
        if (flags & FVM_PROPERTY_READ_ONLY)
        {
            if (c->swf_loaded && flag_cursor != o)
                continue;
            fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator script = fvm_property_find(c, o->accessors, name);
            bool native_length = c->swf_loaded && o->resource_kind == FVM_RESOURCE_ARRAY && name == "length" &&
                script != o->accessors.end() && fvm_base_array_length_accessor(script->second);
            if (!native_length && (!c->swf_loaded || script == o->accessors.end() ||
                                   !fvm_base_script_accessor(script->second)))
                return write.finish(FVM_OK);
        }
    }
    if (flag_cursor)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    if (start != o && !loaded_super)
    {
        unsigned int own_flags = 0;
        FVM_Result flag_result = fvm_object_property_flags_bytes(c, o, name.data(), name.size(), &own_flags);
        if (flag_result != FVM_OK)
            return flag_result;
        if (own_flags & FVM_PROPERTY_READ_ONLY)
        {
            fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator script = fvm_property_find(c, o->accessors, name);
            bool native_length = c->swf_loaded && o->resource_kind == FVM_RESOURCE_ARRAY && name == "length" &&
                script != o->accessors.end() && fvm_base_array_length_accessor(script->second);
            if (!native_length && (!c->swf_loaded || script == o->accessors.end() ||
                                   !fvm_base_script_accessor(script->second)))
                return write.finish(FVM_OK);
        }
    }
    if (!c->swf_loaded && !prepared_watch)
    {
        r = fvm_property_watch_prepare(c, o, name, v, &watched);
        if (r != FVM_OK)
            return r;
    }
    r = copy_value(c, &watched, &copy);
    if (r != FVM_OK)
        return r;

    /* Initialization / Инициализация локального состояния. */
    /* Loaded ordinary writes search through inherited data for virtuals.
     * Own data prevents that search; setters retain the original receiver.
     * Loaded super searches through inherited data without writing local storage. */
    FVM_Object *cursor = start;
    unsigned int accessor_depth = 0;
    while (cursor && accessor_depth++ < FVM_PROTOTYPE_MAX_DEPTH)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* Own setters still run on hidden slots; hidden inherited virtuals are skipped. */
        if (cursor != o && !fvm_property_available(c, cursor, name))
        {
            cursor = cursor->prototype;
            continue;
        }
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (c->swf_loaded && (start == o || loaded_super) ? cursor == o && fvm_property_own_write_data(c, cursor, name) :
            fvm_property_find(c, cursor->properties, name) != cursor->properties.end())
            break;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator found =
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_property_find(c, cursor->accessors, name);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (found != cursor->accessors.end() &&
            !fvm_movieclip_hidden_path_accessor(c, found->second))
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_AccessorEntry entry = found->second;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (!entry.setter)
                /* [GC:] Getter-only inherited virtual dispatch consumes the deferred watch error. */
                return write.finish(FVM_OK, cursor != o);
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
                /* [GC:] Retain lookup/capture ownership even if script detaches the descriptor. */
                FVM_SetterScope setter_scope(c, cursor, o, entry.root, found->first);
                /* [GC:] The setter scope retains owner/receiver/descriptor; script invocation
                 * roots its copied argument before any collection. Preserve outer C barriers. */
                FVM_NativeCallGuard guard(c, !fvm_base_script_accessor(entry));
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return write.finish(fvm_exception_complete(
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    c, o, entry.setter(c, o, 1, &copy, &result, entry.userdata)), cursor != o);
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
    if (accessor_depth > FVM_PROTOTYPE_MAX_DEPTH)
        return FVM_ERROR;
    /* A loaded super facade never falls through to data, metatable or __proto__ writes. */
    if (loaded_super)
        return write.finish(FVM_OK);
    /* An existing own property is always written directly. */
    if (allow_meta && o->properties.find(name) == o->properties.end())
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        while (o->metatable && depth++ < FVM_PROTOTYPE_MAX_DEPTH)
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
                fvm_value_string_bytes(&args[2], name.data(), name.size());
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
                    return write.finish(fn->second.callback(c, o, 4, args, &result, fn->second.userdata));
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
                fvm_value_string_bytes(args + 2, name.data(), name.size());
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                args[3] = copy;
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return write.finish(fvm_function_invoke(c, m->second.object, o, 4, args, &result));
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
        if (depth >= FVM_PROTOTYPE_MAX_DEPTH)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_ERROR;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Loaded raw __proto__ data also controls inheritance when no user setter intercepts it. */
    if (c->swf_loaded && name == fvm_detail::string("__proto__"))
        return write.finish(fvm_object_store_raw_prototype(c, o, copy));
    try
    {
        o->properties[name] = copy;
        fvm_property_clear_versions(c, o, name);
        /* [GC:] Published own data replaces the hidden method and lazy wrapper roots. */
        o->methods.erase(name);
        o->method_values.erase(name);
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
    return write.finish(FVM_OK);
}
/* Write an ordinary property through the receiver's complete property chain. */
FVM_Result fvm_object_set_no_index(FVM_Context *c, FVM_Object *o, const fvm_detail::string &name,
                                   const FVM_Value *value)
{
    return fvm_object_set_from(c, o, o, name, value, true);
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
        o->interfaces_are_prototypes = false; /* Fresh objects have no installed snapshot list. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->metatable = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->swf_initializer_next = 0;
        o->swf_initializer_running = false;
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
        o->resource_kind = FVM_RESOURCE_PLAIN_OBJECT;
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
        while (cursor && prototype_depth++ < FVM_PROTOTYPE_MAX_DEPTH)
        {
            if (cursor == o)
                return FVM_INVALID_ARGUMENT;
            cursor = cursor->prototype;
        }
        if (cursor)
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Keep an existing loaded raw data slot consistent with explicit host relinking. */
        if (c->swf_loaded)
        {
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator raw = o->properties.find("__proto__");
            if (raw != o->properties.end())
            {
                fvm_value_undefined(&raw->second);
                if (proto) { raw->second.type = FVM_VALUE_OBJECT; raw->second.object = proto; }
            }
        }
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
    /* Preserve the terminated-name API through the counted implementation. */
    FVM_Result fvm_object_has_own(FVM_Context *c, FVM_Object *o, const char *name, int *out)
    {
        return name ? fvm_object_has_own_bytes(c, o, name, std::strlen(name), out)
                    : FVM_INVALID_ARGUMENT;
    }
    /* Resolve a validated counted name through the complete property bridge. */
    FVM_Result fvm_object_has_own_bytes(FVM_Context *c, FVM_Object *o, const char *name, size_t size, int *out)
    {
        if (!c || !o || o->owner != c || !name || !out)
            return FVM_INVALID_ARGUMENT;
        try
        {
            fvm_detail::string key;
            FVM_Result normalized = fvm_property_key(name, size, key);
            if (normalized != FVM_OK)
                return normalized;
            if (c->swf_loaded && o->resource_kind == FVM_RESOURCE_SUPER)
                return fvm_super_has_own(c, o, key, out);
            unsigned long index;
            if ((o->resource_kind == FVM_RESOURCE_ARRAY || o->resource_kind == FVM_RESOURCE_OBJECT) &&
                fvm_property_terminated(key) && fvm_as_index(key.c_str(), &index))
            {
                if (o->indexed.find(index) != o->indexed.end())
                {
                    *out = 1;
                    return FVM_OK;
                }
            }

            /* The host accessor implements an intrinsic edge, not an own slot on
             * Object.prototype. Raw user data and custom accessors remain own. */
            if (fvm_object_prototype_name(c, key) &&
                fvm_object_intrinsic_prototype(c, o, &key))
            {
                *out = o->prototype != 0;
                return FVM_OK;
            }
            fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::const_iterator accessor =
                fvm_property_find(c, o->accessors, key);
            bool visible_accessor = accessor != o->accessors.end() &&
                !fvm_movieclip_hidden_path_accessor(c, accessor->second);
            *out = fvm_property_find(c, o->properties, key) != o->properties.end() || visible_accessor ||
                   fvm_property_find(c, o->methods, key) != o->methods.end();
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
    /* Preserve the terminated-name API through the counted implementation. */
    FVM_Result fvm_object_get(FVM_Context *c, FVM_Object *o, const char *name, FVM_Value *out)
    {
        return name ? fvm_object_get_bytes(c, o, name, std::strlen(name), out)
                    : FVM_INVALID_ARGUMENT;
    }
    /* Resolve a validated counted name through the complete property bridge. */
    FVM_Result fvm_object_get_bytes(FVM_Context *c, FVM_Object *o, const char *name, size_t size, FVM_Value *out)
    {
        if (!c || !o || o->owner != c || !name || !out)
            return FVM_INVALID_ARGUMENT;
        fvm_value_undefined(out);
        try
        {
            /* [GC:] Delegated super/resolver callbacks retain the original API input as well. */
            FVM_ObjectRootScope target_roots(c, o);
            fvm_detail::string key;
            FVM_Result normalized = fvm_property_key(name, size, key);
            if (normalized != FVM_OK)
                return normalized;
            if (o->resource_kind == FVM_RESOURCE_SUPER)
                return fvm_super_get(c, o, key, out);
            unsigned long index;
            if ((o->resource_kind == FVM_RESOURCE_ARRAY || o->resource_kind == FVM_RESOURCE_OBJECT) &&
                fvm_property_terminated(key) && fvm_as_index(key.c_str(), &index))
                return fvm_table_get_index(c, o, index, out);
            if (key != fvm_detail::string("__proto__") &&
                fvm_object_prototype_name(c, key) &&
                fvm_object_intrinsic_prototype(c, o, &key))
                key = "__proto__";
            return fvm_object_get_no_index(c, o, key, out);
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
    /* Preserve the terminated-name API through the counted implementation. */
    FVM_Result fvm_object_set(FVM_Context *c, FVM_Object *o, const char *name, const FVM_Value *v)
    {
        return name ? fvm_object_set_bytes(c, o, name, std::strlen(name), v)
                    : FVM_INVALID_ARGUMENT;
    }
    /* Resolve a validated counted name through the complete property bridge. */
    FVM_Result fvm_object_set_bytes(FVM_Context *c, FVM_Object *o, const char *name, size_t size, const FVM_Value *v)
    {
        if (!c || !o || o->owner != c || !name || fvm_validate_value(c, v) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
        try
        {
            fvm_detail::string key;
            FVM_Result normalized = fvm_property_key(name, size, key);
            if (normalized != FVM_OK)
                return normalized;
            if (o->resource_kind == FVM_RESOURCE_SUPER)
                return fvm_super_set(c, o, key, v);
            unsigned long index;
            if ((o->resource_kind == FVM_RESOURCE_ARRAY || o->resource_kind == FVM_RESOURCE_OBJECT) &&
                fvm_property_terminated(key) && fvm_as_index(key.c_str(), &index))
                return fvm_table_set_index(c, o, index, v);
            return fvm_object_set_no_index(c, o, key, v);
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
    /* Read a terminated global name through the counted lookup bridge. */
    FVM_Result fvm_get_global(FVM_Context *c, const char *name, FVM_Value *out)
    {
        return name ? fvm_get_global_bytes(c, name, std::strlen(name), out)
                    : FVM_INVALID_ARGUMENT;
    }
    /* Read a full global name and preserve own undefined shadowing of native entries. */
    FVM_Result fvm_get_global_bytes(FVM_Context *c, const char *name, size_t size, FVM_Value *out)
    {
        if (!c || !name || !out)
            return FVM_INVALID_ARGUMENT;
        fvm_value_undefined(out);
        try
        {
            fvm_detail::string key;
            FVM_Result result = fvm_property_key(name, size, key);
            if (result != FVM_OK)
                return result;
            /* Determine binding presence before getters can delete or replace their slots.
               An inherited undefined binding also suppresses the registered native fallback. */
            int present = 0;
            unsigned int depth = 0;
            for (FVM_Object *cursor = c->global; cursor; cursor = cursor->prototype)
            {
                if (++depth > FVM_PROTOTYPE_MAX_DEPTH)
                    return FVM_ERROR;
                result = fvm_object_has_own_bytes(c, cursor, key.data(), key.size(), &present);
                if (result != FVM_OK)
                    return result;
                if (present)
                    break;
            }
            result = fvm_object_get_bytes(c, c->global, key.data(), key.size(), out);
            if (result != FVM_OK || out->type != FVM_VALUE_UNDEFINED || present)
                return result;
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator fn =
                fvm_property_find(c, c->functions, key);
            return fn == c->functions.end() ? FVM_OK
                                            : fvm_function_wrap_entry(c, 0, fn->first, fn->second, out);
        }
        catch (const std::bad_alloc &)
        {
            fvm_value_undefined(out);
            return FVM_OUT_OF_MEMORY;
        }
    }
    /* Assign a terminated global name through the counted property bridge. */
    FVM_Result fvm_set_global(FVM_Context *c, const char *name, const FVM_Value *value)
    {
        return name ? fvm_set_global_bytes(c, name, std::strlen(name), value)
                    : FVM_INVALID_ARGUMENT;
    }
    /* Assign the complete global key without shortening its byte count. */
    FVM_Result fvm_set_global_bytes(FVM_Context *c, const char *name, size_t size,
                                   const FVM_Value *value)
    {
        return c ? fvm_object_set_bytes(c, c->global, name, size, value) : FVM_INVALID_ARGUMENT;
    }
    /* Register an unrooted terminated function with canonical name identity. */
    FVM_Result fvm_register_function(FVM_Context *c, const char *name, FVM_NativeFunction callback,
                                     void *userdata)
    {
        return name ? fvm_register_function_rooted_bytes(c, name, std::strlen(name), callback,
                                                         userdata, 0) : FVM_INVALID_ARGUMENT;
    }
    /* [GC:] Register a terminated function and its traced capture in one operation. */
    FVM_Result fvm_register_function_rooted(FVM_Context *c, const char *name,
                                            FVM_NativeFunction callback, void *userdata,
                                            FVM_Object *root)
    {
        return name ? fvm_register_function_rooted_bytes(c, name, std::strlen(name), callback,
                                                         userdata, root) : FVM_INVALID_ARGUMENT;
    }
    /* [GC:] Prepare a canonical key and capture before replacing the native registration. */
    FVM_Result fvm_register_function_rooted_bytes(FVM_Context *c, const char *name, size_t size,
                                                  FVM_NativeFunction callback, void *userdata,
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
            FVM_NativeEntry entry;
            entry.callback = callback;
            entry.userdata = userdata;
            entry.root = root;
            /* A legacy alternate spelling updates the existing registration. Copy its
               identity before publication so allocation failure preserves the old entry. */
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator previous =
                fvm_property_find(c, c->functions, key);
            if (previous != c->functions.end())
                key = previous->first;
            c->functions[key] = entry;
            /* [GC:] A stale cached wrapper must not keep the replaced capture alive.
               Saved Function values retain their own original entry through normal roots. */
            fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator cached =
                c->function_objects.find(key);
            if (cached != c->function_objects.end() &&
                (cached->second->invocable.callback != entry.callback ||
                 cached->second->invocable.userdata != entry.userdata ||
                 cached->second->invocable.root != entry.root ||
                 cached->second->invocable.collector_entry != entry.collector_entry))
                c->function_objects.erase(key);
            return FVM_OK;
        }
        catch (const std::bad_alloc &)
        {
            return FVM_OUT_OF_MEMORY;
        }
    }
    /* Call object. */
    FVM_Result fvm_object_call(FVM_Context *c, FVM_Object *self, const char *name,
                               unsigned int argc, const FVM_Value *argv, FVM_Value *out)
    {
        return name ? fvm_object_call_bytes(c, self, name, std::strlen(name), argc, argv, out)
                    : FVM_INVALID_ARGUMENT;
    }
    /* Preserve a canonical counted name through the native dispatch bridge. */
    FVM_Result fvm_object_call_bytes(FVM_Context *c, FVM_Object *self, const char *name, size_t size,
                               unsigned int argc, const FVM_Value *argv, FVM_Value *out)
    {
        unsigned int i;
        if (!c || !self || self->owner != c || !name || !out || (argc && !argv))
            return FVM_INVALID_ARGUMENT;
        for (i = 0; i < argc; ++i)
            if (fvm_validate_value(c, &argv[i]) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            fvm_detail::string key;
            FVM_Result normalized = fvm_property_key(name, size, key);
            if (normalized != FVM_OK)
            {
                fvm_value_undefined(out);
                return normalized;
            }
            unsigned long index = 0;
            const bool indexed_name = fvm_property_terminated(key) && fvm_as_index(key.c_str(), &index);
            /* [GC:] Snapshot caller values before clearing an aliased output or running a getter. */
            FVM_ObjectRootScope call_receiver_roots(c, self);
            fvm_detail::vector<FVM_Value> call_args;
            FVM_ValueVectorRootScope call_argument_roots(c, call_args);
            call_args.reserve(argc);
            for (i = 0; i < argc; ++i)
                call_args.push_back(argv[i]);
            argv = argc ? call_args.begin() : 0;
            fvm_value_undefined(out);

            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            if (self->resource_kind == FVM_RESOURCE_SUPER)
                return fvm_super_method(c, self, key, argc, argv, out);
            FVM_Object *cursor = self;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            unsigned int levels = 0;
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            while (cursor && levels++ < FVM_PROTOTYPE_MAX_DEPTH)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (!fvm_property_available(c, cursor, key))
                {
                    cursor = cursor->prototype;
                    continue;
                }
                if (fvm_property_find(c, cursor->properties, key) != cursor->properties.end() ||
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    fvm_property_find(c, cursor->accessors, key) != cursor->accessors.end() ||
                    /* Canonical numeric members use indexed storage on standard objects. */
                    (indexed_name && (cursor->resource_kind == FVM_RESOURCE_ARRAY ||
                                      cursor->resource_kind == FVM_RESOURCE_OBJECT) &&
                     cursor->indexed.find(index) != cursor->indexed.end()))
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                {
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    /* [GC:] A getter may detach the defining prototype before the method collects. */
                    FVM_ObjectRootScope receiver_roots(c, self);
                    FVM_ObjectRootScope owner_roots(c, cursor);
                    FVM_Value member;
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    FVM_Result r = fvm_object_get_bytes(c, self, key.data(), key.size(), &member);
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    if (r != FVM_OK)
                        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                        return r;
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    if (member.type != FVM_VALUE_OBJECT)
                        return FVM_UNSUPPORTED;
                    /* [GC:] Retain the actual owner separately from the lookup depth captured
                     * before getter execution; own loaded methods have a minimum depth of one. */
                    FVM_CallScope scope(c, 0, member.object, cursor,
                                        c->swf_loaded ? (levels > FVM_MEMBER_CALL_MIN_SUPER_DEPTH
                                            ? levels - 1 : FVM_MEMBER_CALL_MIN_SUPER_DEPTH)
                                                      : FVM_CALL_SCOPE_INFER_SUPER_DEPTH);
                    return fvm_function_invoke(c, member.object, self, argc, argv, out);
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                }
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (fvm_property_find(c, cursor->methods, key) != cursor->methods.end())
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    break;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                cursor = cursor->prototype;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_NativeEntry entry;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Result lookup = fvm_lookup_native_method(self, key, &entry);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (lookup != FVM_OK && lookup != FVM_NOT_FOUND)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return lookup;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (lookup == FVM_NOT_FOUND)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                if (c->swf_loaded)
                {
                    FVM_Value function;
                    bool handled = false;
                    FVM_Result status = fvm_property_resolve(c, self, self, key, &function, &handled);
                    if (status != FVM_OK)
                        return status;
                    if (handled)
                    {
                        if (function.type != FVM_VALUE_OBJECT)
                            return FVM_UNSUPPORTED;
                        /* [GC:] A resolver result has no defining owner but still executes as
                         * a method with explicit depth one, even after prototype detachment. */
                        FVM_CallScope scope(c, 0, function.object, 0,
                                            FVM_PROPERTY_RESOLVED_METHOD_SUPER_DEPTH);
                        return fvm_function_invoke(c, function.object, self, argc, argv, out);
                    }
                }
                fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator it =
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    fvm_property_find(c, c->functions, key);
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (it == c->functions.end())
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return FVM_UNSUPPORTED;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                entry = it->second;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            /* [GC:] A trusted callback may remove its registration before collecting its capture. */
            FVM_ObjectRootScope entry_roots(c, entry.root);
            fvm_resource_fault_begin(c);
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_value_undefined(out);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (entry.collector_entry)
            {
                /* [GC:] Receiver and snapshot arguments remain linked through this trusted callback. */
                return fvm_exception_complete(c, self,
                    fvm_native_result(c, entry.callback(c, self, argc, argv, out, entry.userdata), out));
            }
            /* [GC:] Ordinary native entries retain their existing collector barrier. */
            FVM_NativeCallGuard native_guard(c);
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return fvm_exception_complete(
                c, self,
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_native_result(c, entry.callback(c, self, argc, argv, out, entry.userdata),
                                  out));
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
        return name ? fvm_object_define_method_rooted_bytes(c, object, name, std::strlen(name), callback, userdata, root)
                    : FVM_INVALID_ARGUMENT;
    }
    /* Preserve a canonical counted name through the native dispatch bridge. */
    FVM_Result fvm_object_define_method_rooted_bytes(
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Context *c, FVM_Object *object, const char *name, size_t size,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeFunction callback, void *userdata,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *root)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !object || object->owner != c || !name || !size || !callback ||
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
            fvm_detail::string key;
            FVM_Result normalized = fvm_property_key(name, size, key);
            if (normalized != FVM_OK)
                return normalized;
            /* Loaded definitions share the existing own spelling and conflict policy. */
            fvm_property_select_own_name(c, object, key);

            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */

            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (object->properties.find(key) != object->properties.end() ||
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                object->accessors.find(key) != object->accessors.end())
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return FVM_INVALID_ARGUMENT;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            object->methods[key] = entry;
            /* [GC:] Release a stale lazy wrapper only after replacement succeeds.
             * Exported/pinned Functions retain their own callback capture. */
            fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator cached =
                object->method_values.find(key);
            if (cached != object->method_values.end() &&
                (cached->second->invocable.callback != entry.callback ||
                 cached->second->invocable.userdata != entry.userdata ||
                 cached->second->invocable.root != entry.root))
                object->method_values.erase(key);
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
        return name ? fvm_object_define_accessor_rooted_bytes(c, object, name, std::strlen(name), getter, setter, userdata, root)
                    : FVM_INVALID_ARGUMENT;
    }
    /* Preserve a canonical counted name through the native dispatch bridge. */
    FVM_Result fvm_object_define_accessor_rooted_bytes(
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Context *c, FVM_Object *object,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        const char *name, size_t size, FVM_NativeFunction getter,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeFunction setter, void *userdata,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *root)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !object || object->owner != c || !name || !size || (!getter && !setter) ||
            /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
            (root && root->owner != c) || c->destroying || c->gc_releasing)
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
            fvm_detail::string key;
            FVM_Result normalized = fvm_property_key(name, size, key);
            if (normalized != FVM_OK)
                return normalized;
            /* Loaded definitions share the existing own spelling and conflict policy. */
            fvm_property_select_own_name(c, object, key);

            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            if (object->properties.find(key) != object->properties.end())
                return FVM_INVALID_ARGUMENT;
            object->accessors[key] = entry;
            /* [GC:] A published accessor shadows and replaces any native method root. */
            object->methods.erase(key);
            object->method_values.erase(key);
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
    FVM_Result fvm_register_constructor_rooted(FVM_Context *c, const char *name,
                                      FVM_NativeFunction callback, void *userdata, FVM_Object *root)
    {
        return name ? fvm_register_constructor_rooted_bytes(c, name, std::strlen(name), callback,
                                                            userdata, root) : FVM_INVALID_ARGUMENT;
    }
    /* [GC:] Register a counted constructor and its capture under one canonical key. */
    FVM_Result fvm_register_constructor_rooted_bytes(
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Context *c, const char *name, size_t size,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeFunction callback, void *userdata,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *root)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!c || !name || !size || !callback ||
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
            fvm_detail::string key;
            FVM_Result normalized = fvm_property_key(name, size, key);
            if (normalized != FVM_OK)
                return normalized;
            if (key == fvm_detail::string("Object"))
                return FVM_INVALID_ARGUMENT;

            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            /* Preserve an existing legacy registration's stored identity. All allocating
               key work precedes publication so failure leaves its capture unchanged. */
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator previous =
                fvm_property_find(c, c->constructors, key);
            if (previous != c->constructors.end())
                key = previous->first;
            c->constructors[key] = entry;
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
                  const FVM_Value *argv, FVM_Object **out)
    {
        if (!name)
        {
            if (out) *out = 0;
            return FVM_INVALID_ARGUMENT;
        }
        return fvm_construct_bytes(c, name, std::strlen(name), argc, argv, out);
    }
    /* [GC:] Construct through a full native name or a full-key Function global. */
    FVM_Result fvm_construct_bytes(FVM_Context *c, const char *name, size_t size, unsigned int argc,
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
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Constructor-name getters may collect or mutate caller storage before
               Function construction begins; retain a stable argument snapshot first. */
            fvm_detail::vector<FVM_Value> call_args;
            FVM_ValueVectorRootScope call_argument_roots(c, call_args);
            call_args.reserve(argc);
            for (unsigned int i = 0; i < argc; ++i)
                call_args.push_back(argv[i]);
            argv = argc ? call_args.begin() : 0;
            fvm_detail::string key;
            FVM_Result normalized = fvm_property_key(name, size, key);
            if (normalized != FVM_OK)
                return normalized;
            if (key == fvm_detail::string("Object") && !c->standard_library)
                return argc ? FVM_UNSUPPORTED : fvm_object_create(c, out);

            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator found =
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_property_find(c, c->constructors, key);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (found == c->constructors.end())
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Value function;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Result r = fvm_get_global_bytes(c, key.data(), key.size(), &function);
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
    /* Call a full global name; only terminated keys reach the legacy stack registry. */
    FVM_Result fvm_call_bytes(FVM_Context *c, const char *name, size_t size, unsigned int argc,
                              const FVM_Value *args, FVM_Value *out)
    {
        if (!c || !name || !out || (argc && !args))
            return FVM_INVALID_ARGUMENT;
        for (unsigned int i = 0; i < argc; ++i)
            if (fvm_validate_value(c, args + i) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
        try
        {
            /* [GC:] Snapshot inputs before an aliased output or a lookup callback can mutate
               caller storage; retain the snapshot through native/script/resolver dispatch. */
            fvm_detail::vector<FVM_Value> call_args;
            FVM_ValueVectorRootScope call_argument_roots(c, call_args);
            call_args.reserve(argc);
            for (unsigned int i = 0; i < argc; ++i)
                call_args.push_back(args[i]);
            args = argc ? call_args.begin() : 0;
            fvm_value_undefined(out);
            fvm_detail::string key;
            FVM_Result result = fvm_property_key(name, size, key);
            if (result != FVM_OK)
                return result;
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator registered =
                fvm_property_find(c, c->functions, key);
            if (registered != c->functions.end())
            {
                /* Copy the entry before a callback can replace its registration. */
                FVM_NativeEntry entry = registered->second;
                /* [GC:] Preserve the native barrier and result lifetime of the legacy call. */
                FVM_NativeCallGuard guard(c);
                fvm_resource_fault_begin(c);
                return fvm_exception_complete(c, c->global,
                    fvm_native_result(c, entry.callback(c, c->global, argc, args, out,
                                                        entry.userdata), out));
            }
            FVM_Value callable;
            result = fvm_get_global_bytes(c, key.data(), key.size(), &callable);
            if (result != FVM_OK)
                return result;
            if (callable.type == FVM_VALUE_OBJECT && callable.object->invocable.callback)
            {
                if (c->swf_loaded)
                {
                    /* [GC:] Named global function values retain identity independently of
                     * a binding getter/resolver that may delete the original property. */
                    FVM_CallScope call(c, 0, callable.object, 0, FVM_FUNCTION_CALL_SUPER_DEPTH);
                    return fvm_function_invoke(c, callable.object, c->global, argc, args, out);
                }
                return fvm_function_invoke(c, callable.object, c->global, argc, args, out);
            }
            return fvm_stack_call_bytes(c, key.data(), key.size(), argc, args, out);
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
    /* Call a terminated global name through the canonical counted bridge. */
    FVM_Result fvm_call(FVM_Context *c, const char *name, unsigned int argc,
                         const FVM_Value *args, FVM_Value *out)
    {
        return name ? fvm_call_bytes(c, name, std::strlen(name), argc, args, out)
                    : FVM_INVALID_ARGUMENT;
    }
}
