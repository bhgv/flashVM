/* FlashVM: Built-in ActionScript objects: function.
 * File: src/standard/function.cpp.
 */

#include "../core/string_internal.hpp"
#include "function_state.hpp"
#include "constructor_identity.hpp"
#include "function_apply_constants.h"
#include "../avm1/coercion.hpp"
#include "../avm1/handlers/class_prototype.hpp"
#include "../avm1/scope_keywords.hpp"
#include "../core/numeric_constants.h"
#include "../core/property_key.hpp"
#include "../avm1/function_record.hpp"
#include "../avm1/block.hpp"
#include "../avm1/encoding_constants.h"
#include "../core/call_scope.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/value_vector_root_scope.hpp"
#include "../core/execution_work_scope.hpp"
#include "super.hpp"
#include "base_state.hpp"
#include "flashvm/function.h"
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/table.h"
#include "flashvm/avm1.h"
#include "../core/native_call_guard.hpp"
#include "../core/exception_internal.hpp"
#include <cstring>
#include <new>
#include <cmath>
#include <limits>
#include <cstdio>

namespace
{
/* Define anonymous enum and its representation. */
enum
{
    SCRIPT_FUNCTION = FVM_RESOURCE_SCRIPT_FUNCTION,
    FUNCTION_CLASS = FVM_RESOURCE_FUNCTION_CLASS,
    NATIVE_FUNCTION = FVM_RESOURCE_NATIVE_FUNCTION
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
    if (c->swf_loaded)
    {
        /* Loaded call/apply select the active global boxing constructor. A missing
         * or unusable constructor leaves their implicit receiver at the global. */
        FVM_Value boxed;
        FVM_Result result = fvm_exec_to_object(c, value, &boxed);
        if (result == FVM_OK)
            *out = boxed.type == FVM_VALUE_OBJECT ? boxed.object : c->global;
        return result;
    }
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

/* Loaded borrowed adapters return undefined for non-callable targets only after
 * receiver boxing and argument preparation have completed successfully. */
FVM_Result invoke_adapter(FVM_Context *c, FVM_Object *fn, FVM_Object *receiver,
                           unsigned int n, const FVM_Value *args, FVM_Value *out)
{
    /* A superclass facade is callable only through direct super dispatch. */
    if (c->swf_loaded && fn &&
        (!fn->invocable.callback || fn->resource_kind == FVM_RESOURCE_SUPER))
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return fvm_function_invoke(c, fn, receiver, n, args, out);
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
    return invoke_adapter(c, fn, receiver, n ? n - 1 : 0, n ? a + 1 : 0, out);
}

/* Find own length with loaded casing, charging each legacy fallback candidate. */
template <class Entries>
FVM_Result apply_length_slot(FVM_Context *c, Entries &entries, FVM_ExecutionWorkScope &work,
                              typename Entries::iterator &found)
{
    found = entries.find(FVM_FUNCTION_APPLY_LENGTH_NAME);
    if (found != entries.end() || c->swf_info.version >= FVM_SWF_VERSION_7)
        return FVM_OK;
    for (found = entries.begin(); found != entries.end(); ++found)
    {
        FVM_Result status = work.step();
        if (status != FVM_OK)
            return status;
        if (fvm_avm1_keyword_name(found->first, FVM_FUNCTION_APPLY_LENGTH_NAME,
                                  FVM_FUNCTION_APPLY_LENGTH_UNITS, false))
            return FVM_OK;
    }
    return FVM_OK;
}

/* Capture raw length once under the caller's budget. Positive Int32 lengths can be
 * gathered incrementally; negative lengths retain an explicit runtime rejection. */
FVM_Result apply_loaded_count(FVM_Context *c, FVM_Object *source, double &count,
                               FVM_ExecutionWorkScope &work)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator property;
    FVM_Result status = apply_length_slot(c, source->properties, work, property);
    if (status != FVM_OK)
        return status;
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor = source->accessors.end();
    if (property == source->properties.end())
    {
        status = apply_length_slot(c, source->accessors, work, accessor);
        if (status != FVM_OK)
            return status;
    }
    if (property != source->properties.end())
        value = property->second;
    else if (accessor != source->accessors.end())
    {
        if (!fvm_base_accessor_data(accessor->second, value) &&
            fvm_base_array_length_accessor(accessor->second) && source->resource_result_valid)
            value = source->resource_result;
    }
    else
    {
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method;
        status = apply_length_slot(c, source->methods, work, method);
        if (status != FVM_OK)
            return status;
        if (method != source->methods.end())
        {
            status = fvm_function_wrap_entry(c, source, method->first, method->second, &value);
            if (status != FVM_OK)
                return status;
        }
        else if (source->resource_kind == FVM_RESOURCE_ARRAY && source->resource_result_valid)
            value = source->resource_result;
    }
    /* [GC:] Length conversion may remove its own source slot and invoke collecting accessors. */
    FVM_ObjectRootScope length_roots(c, value.type == FVM_VALUE_OBJECT ? value.object : 0);
    double number = 0;
    status = fvm_avm1_number(c, &value, &number);
    if (status != FVM_OK)
        return status;
    count = 0;
    if (number != number || std::fabs(number) == std::numeric_limits<double>::infinity())
        return FVM_OK;
    number = std::fmod(number < 0 ? std::ceil(number) : std::floor(number), FVM_UINT32_MODULUS);
    if (number < 0)
        number += FVM_UINT32_MODULUS;
    if (number >= FVM_UINT32_SIGN_BIT)
        return FVM_UNSUPPORTED;
    count = number;
    return FVM_OK;
}

/* Compute and return apply. */
FVM_Result apply(FVM_Context *c, FVM_Object *fn, unsigned int n, const FVM_Value *a, FVM_Value *out,
                 void *userdata)
{
    /* Loaded boxing, conversion and element callbacks inherit one execution quota. */
    FVM_ExecutionWorkScope work(c, c->swf_loaded);
    FVM_Object *receiver = n && a[0].type == FVM_VALUE_OBJECT ? a[0].object : c->global;
    /* apply and call use the same primitive receiver boxing rules. */
    if (n && a[0].type != FVM_VALUE_OBJECT && a[0].type != FVM_VALUE_UNDEFINED &&
        a[0].type != FVM_VALUE_NULL)
    {
        FVM_Result r = box_receiver(c, a, (FVM_Object *)userdata, &receiver);
        if (r != FVM_OK)
            return r;
    }
    /* [GC:] Boxing may create a receiver absent from the original call arguments. */
    FVM_ObjectRootScope receiver_roots(c, receiver);
    if (n > 1 && a[1].type != FVM_VALUE_NULL && a[1].type != FVM_VALUE_UNDEFINED)
    {
        if (a[1].type != FVM_VALUE_OBJECT)
            return c->swf_loaded ? invoke_adapter(c, fn, receiver, 0, 0, out)
                                  : FVM_INVALID_ARGUMENT;
        double count = 0;
        FVM_Result r = FVM_OK;
        if (c->swf_loaded)
            r = apply_loaded_count(c, a[1].object, count, work);
        else
        {
            FVM_Value length;
            r = fvm_object_get(c, a[1].object, FVM_FUNCTION_APPLY_LENGTH_NAME, &length);
            if (r == FVM_OK)
                r = fvm_value_to_number(c, &length, &count);
            if (r == FVM_OK && (!(count >= 0 && count <= FVM_FUNCTION_APPLY_HOST_LIMIT) ||
                                 count != std::floor(count)))
                r = FVM_INVALID_ARGUMENT;
        }
        if (r != FVM_OK)
            return r;
        fvm_detail::vector<FVM_Value> args(c->swf_loaded ? 0 : (size_t)count);
        for (size_t i = 0; i < args.size(); ++i)
            fvm_value_undefined(&args[i]);
        /* [GC:] Later script getters may collect values returned by earlier getters. */
        FVM_ValueVectorRootScope argument_roots(c, args);
        for (unsigned int i = 0; i < (unsigned int)count; ++i)
        {
            if (c->swf_loaded)
            {
                r = work.step();
                if (r != FVM_OK)
                    return r;
                FVM_Value absent;
                fvm_value_undefined(&absent);
                /* [GC:] Publish an initialized slot before its getter can collect. */
                args.push_back(absent);
                /* Loaded apply uses ordinary named lookup even on native non-Array objects. */
                char name[FVM_FUNCTION_APPLY_INDEX_KEY_BYTES];
                std::sprintf(name, "%u", i);
                r = fvm_object_get(c, a[1].object, name, &args[i]);
            }
            else
                r = fvm_table_get_index(c, a[1].object, i, &args[i]);
            if (r != FVM_OK)
                return r;
        }
        return invoke_adapter(c, fn, receiver, (unsigned int)count, args.begin(), out);
    }
    return invoke_adapter(c, fn, receiver, 0, 0, out);
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

/* Read the parameter bound to this arguments index through its retained descriptor. */
FVM_Result argument_get(FVM_Context *c, FVM_Object *, unsigned int, const FVM_Value *,
                        FVM_Value *out, void *ud)
{
    FVM_Value scope, name;
    FVM_Result r = fvm_object_get(c, (FVM_Object *)ud, "scope", &scope);
    if (r == FVM_OK)
        r = fvm_object_get(c, (FVM_Object *)ud, "name", &name);
    if (r != FVM_OK)
        return r;
    if (scope.type != FVM_VALUE_OBJECT || name.type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    return fvm_object_get_bytes(c, scope.object, name.string, fvm_string_size(name), out);
}
/* Update the named parameter bound to this arguments entry. */
FVM_Result argument_set(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                        FVM_Value *out, void *ud)
{
    if (!n)
        return FVM_INVALID_ARGUMENT;
    FVM_Value scope, name;
    FVM_Result r = fvm_object_get(c, (FVM_Object *)ud, "scope", &scope);
    if (r == FVM_OK)
        r = fvm_object_get(c, (FVM_Object *)ud, "name", &name);
    if (r != FVM_OK)
        return r;
    if (scope.type != FVM_VALUE_OBJECT || name.type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    r = fvm_object_set_bytes(c, scope.object, name.string, fvm_string_size(name), a);
    fvm_value_undefined(out);
    return r;
}
/* Decode a hexadecimal bytecode digit, or return an invalid-digit sentinel. */
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
    FVM_Value code, params, constants, closure, base_clip;
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
    r = fvm_object_get(c, data, "base_clip", &base_clip);
    if (r != FVM_OK) return r;
    if (base_clip.type != FVM_VALUE_UNDEFINED && base_clip.type != FVM_VALUE_NULL &&
        (base_clip.type != FVM_VALUE_OBJECT || !base_clip.object ||
         base_clip.object->owner != c || base_clip.object->resource_kind != FVM_RESOURCE_MOVIECLIP))
        return FVM_INVALID_ARGUMENT;
    if (code.type != FVM_VALUE_STRING || params.type != FVM_VALUE_OBJECT ||
        constants.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    size_t bytes = fvm_string_size(code);
    if (bytes % FVM_AVM1_HEX_DIGITS_PER_BYTE || bytes > FVM_AVM1_MAX_HEX_BODY_BYTES)
        return FVM_INVALID_ARGUMENT;
    FVM_ScriptFunction body;
    for (size_t i = 0; i < bytes; i += FVM_AVM1_HEX_DIGITS_PER_BYTE)
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
    if (length.type != FVM_VALUE_NUMBER || !(length.number >= 0 && length.number <= FVM_AVM1_MAX_CALL_ARGUMENTS) ||
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

        fvm_detail::string key;
        r = fvm_property_key(v.string, fvm_string_size(v), key);
        if (r != FVM_OK)
            return r;
        body.parameters.push_back(key);
    }
    r = fvm_object_get(c, constants.object, "length", &length);
    if (r != FVM_OK)
        return r;
    if (length.type != FVM_VALUE_NUMBER || !(length.number >= 0 && length.number <= FVM_AVM1_MAX_CALL_ARGUMENTS) ||
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
        body.constants.push_back(fvm_detail::string(v.string, fvm_string_size(v)));
    }
    FVM_Value kind;
    r = fvm_object_get(c, data, "function2", &kind);
    if (r != FVM_OK)
        return r;
    if (kind.type == FVM_VALUE_BOOLEAN && kind.boolean)
    {
        FVM_Value count, flags, registers, register_length;
        r = fvm_object_get(c, data, "register_count", &count);
        if (r == FVM_OK)
            r = fvm_object_get(c, data, "flags", &flags);
        if (r == FVM_OK)
            r = fvm_object_get(c, data, "parameter_registers", &registers);
        if (r != FVM_OK)
            return r;
        if (count.type != FVM_VALUE_NUMBER || count.number < 0 || count.number > FVM_AVM1_MAX_REGISTER_INDEX ||
            count.number != std::floor(count.number) || flags.type != FVM_VALUE_NUMBER ||
            flags.number < 0 || flags.number > FVM_AVM1_F2_VALID_FLAGS || flags.number != std::floor(flags.number) ||
            registers.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        r = fvm_object_get(c, registers.object, "length", &register_length);
        if (r != FVM_OK)
            return r;
        if (register_length.type != FVM_VALUE_NUMBER ||
            register_length.number != body.parameters.size())
            return FVM_INVALID_ARGUMENT;
        body.function2 = true;
        body.register_count = (unsigned int)count.number;
        body.flags = (unsigned int)flags.number;
        unsigned int preload = 0;
        const unsigned int bits[] = {FVM_AVM1_F2_PRELOAD_THIS, FVM_AVM1_F2_PRELOAD_ARGUMENTS, FVM_AVM1_F2_PRELOAD_SUPER, FVM_AVM1_F2_PRELOAD_ROOT, FVM_AVM1_F2_PRELOAD_PARENT, FVM_AVM1_F2_PRELOAD_GLOBAL};
        for (unsigned int i = 0; i < FVM_AVM1_PRELOAD_COUNT; ++i)
            if (body.flags & bits[i])
                ++preload;
        if (!fvm_function2_flags_valid(body.flags) || (!c->swf_loaded && preload > body.register_count))
            return FVM_INVALID_ARGUMENT;
        for (size_t i = 0; i < body.parameters.size(); ++i)
        {
            FVM_Value reg;
            r = fvm_table_get_index(c, registers.object, (unsigned long)i, &reg);
            if (r != FVM_OK)
                return r;
            if (reg.type != FVM_VALUE_NUMBER || reg.number < 0 ||
                reg.number > (c->swf_loaded ? FVM_AVM1_MAX_REGISTER_INDEX : body.register_count) || reg.number != std::floor(reg.number))
                return FVM_INVALID_ARGUMENT;
            body.parameter_registers.push_back((unsigned int)reg.number);
        }
    }
    FVM_Value name;
    r = fvm_object_get(c, data, "name", &name);
    if (r != FVM_OK)
        return r;
    if (name.type == FVM_VALUE_STRING)
    {
        r = fvm_property_key(name.string, fvm_string_size(name), body.name);
        if (r != FVM_OK)
            return r;
    }
    return fvm_avm1_invoke_script(c, body, closure.type == FVM_VALUE_OBJECT ? closure.object : 0,
                                  receiver, n, a, out, fn, base_clip.type == FVM_VALUE_OBJECT ? base_clip.object : 0);
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
    FVM_Context *c, FVM_Object *owner, const fvm_detail::string &name,
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
    /* [GC:] New reads must reflect the current callback's collection policy; saved
       Function objects retain the policy copied at their own creation. */
    if (!fn || fn->invocable.callback != entry.callback ||
        fn->invocable.userdata != entry.userdata || fn->invocable.root != entry.root ||
        fn->invocable.collector_entry != entry.collector_entry)
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
    /* Internal function metadata must not execute public Array.prototype accessors. */
    r = fvm_array_create_like(c, 0, (unsigned long)body.parameters.size(), &params, true);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_array_create_like(c, 0, (unsigned long)body.constants.size(), &constants, true);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */

    params->prototype = 0;
    constants->prototype = 0;
    for (size_t i = 0; i < body.parameters.size(); ++i)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value v;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::string key;
        FVM_Result normalized = fvm_property_key(body.parameters[i].data(), body.parameters[i].size(), key);
        if (normalized != FVM_OK)
            return normalized;
        fvm_value_string_bytes(&v, key.data(), key.size());
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
    /* [GC:] A loaded function retains the timeline on which its body was defined. */
    v.type = c->swf_loaded && c->avm1_base_clip ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    v.object = c->swf_loaded ? c->avm1_base_clip : 0;
    data->properties["base_clip"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_string_bytes(&v, body.name.data(), body.name.size());
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_object_set(c, data, "name", &v);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */

    if (body.function2)
    {
        if (body.parameter_registers.size() != body.parameters.size())
            return FVM_INVALID_ARGUMENT;
        FVM_Object *registers = 0;
        r = fvm_array_create_like(c, 0, (unsigned long)body.parameter_registers.size(), &registers, true);
        if (r != FVM_OK)
            return r;
        registers->prototype = 0;
        for (size_t i = 0; i < body.parameter_registers.size(); ++i)
        {
            fvm_value_number(&v, body.parameter_registers[i]);
            r = fvm_table_set_index(c, registers, (unsigned long)i, &v);
            if (r != FVM_OK)
                return r;
        }
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_OBJECT;
        v.object = registers;
        data->properties["parameter_registers"] = v;
        fvm_value_number(&v, body.register_count);
        data->properties["register_count"] = v;
        fvm_value_number(&v, body.flags);
        data->properties["flags"] = v;
        fvm_value_boolean(&v, 1);
        data->properties["function2"] = v;
    }
    fn->resource_kind = SCRIPT_FUNCTION;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fn->resource_result_valid = true;
    fvm_value_undefined(&v);
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
    /* [GC:] Loaded constructor prototypes inherit the original builtin Object
     * prototype even when script has replaced Object.prototype. Host layout stays unchanged. */
    if (c->swf_loaded)
        instance_proto->prototype = c->object_builtin_prototype;
    v.type = FVM_VALUE_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.object = instance_proto;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fn->properties["prototype"] = v;
    /* Loaded function prototype is writable, nonenumerable and nondeletable. */
    if (c->swf_loaded)
        fn->attributes["prototype"] = 3;
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
/* Create a legacy terminated-parameter function through the counted factory. */
extern "C" FVM_Result fvm_function_create_avm1(FVM_Context *c, const void *actions, size_t size,
                                               unsigned int count, const char *const *parameters,
                                               FVM_Object *environment, FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || (!actions && size) || (count && !parameters) || count > FVM_AVM1_MAX_CALL_ARGUMENTS || size > FVM_AVM1_MAX_BODY_BYTES ||
        (environment && environment->owner != c))
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::vector<FVM_Value> names(count);
        for (unsigned int i = 0; i < count; ++i)
            if (fvm_value_string(&names[i], parameters[i]) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
        return fvm_function_create_avm1_bytes(c, actions, size, count, names.begin(), environment, out);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
/* Copy complete UTF-16 parameter identities into a traced script-function snapshot. */
extern "C" FVM_Result fvm_function_create_avm1_bytes(FVM_Context *c, const void *actions, size_t size,
                                                     unsigned int count, const FVM_Value *parameters,
                                                     FVM_Object *environment, FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || (!actions && size) || (count && !parameters) ||
        (environment && environment->owner != c) || size > FVM_AVM1_MAX_BODY_BYTES || count > FVM_AVM1_MAX_CALL_ARGUMENTS)
        return FVM_INVALID_ARGUMENT;
    FVM_AVM1Info info;
    info.struct_size = sizeof(info);
    FVM_Result result = (c->swf_loaded ? fvm_avm1_inspect_loaded : fvm_avm1_inspect)(actions, size, &info);
    if (result != FVM_OK)
        return result;
    try
    {
        FVM_ScriptFunction body;
        if (size)
            body.body.assign((const unsigned char *)actions, (const unsigned char *)actions + size);
        for (unsigned int i = 0; i < count; ++i)
        {
            if (fvm_validate_value(c, parameters+i) != FVM_OK || parameters[i].type != FVM_VALUE_STRING)
                return FVM_INVALID_ARGUMENT;
            fvm_detail::string key;
            result = fvm_property_key(parameters[i].string, fvm_string_size(parameters[i]), key);
            if (result != FVM_OK)
                return result;
            body.parameters.push_back(key);
        }
        return fvm_function_make_script(c, body, environment, out);
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
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!c || !fn || fn->owner != c || (self && self->owner != c) || (n && !args) || c->destroying)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    {
        fvm_value_undefined(out);
        return FVM_INVALID_ARGUMENT;
    }
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (unsigned int i = 0; i < n; ++i)
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (fvm_validate_value(c, args + i) != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        {
            fvm_value_undefined(out);
            return FVM_INVALID_ARGUMENT;
        }
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!fn->invocable.callback)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    {
        fvm_value_undefined(out);
        return FVM_UNSUPPORTED;
    }
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
        /* Pass the snapshot, then clear a possibly aliased result only after copying.
           The frame below retains the same values when callbacks mutate caller storage. */
        args = n ? roots.begin() + (roots.size() - n) : 0;
        fvm_value_undefined(out);
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
                frame.receiver,
                /* [GC:] Snapshot/validate the result while function/capture root frames are active.
                 */
                fvm_native_result(
                    /* [GC:] Validate and retain the callback result before root-frame teardown. */
                    c, entry.callback(c, frame.receiver, n, args, out, entry.userdata), out));
        /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
        FVM_NativeCallGuard guard(c);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return fvm_exception_complete(
            /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
            c, frame.receiver,
            /* [GC:] Copy foreign string views before the native barrier and capture roots unwind.
             */
            fvm_native_result(c, entry.callback(c, frame.receiver, n, args, out, entry.userdata),
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
        /* [GC:] Direct C callers may supply an unpinned Function and arguments. Keep
           them live and stable before prototype access can mutate caller storage or collect. */
        FVM_ObjectRootScope function_root(c, fn);
        fvm_detail::vector<FVM_Value> arguments;
        FVM_ValueVectorRootScope argument_roots(c, arguments);
        arguments.reserve(n);
        for (unsigned int i = 0; i < n; ++i)
            arguments.push_back(a[i]);
        a = n ? arguments.begin() : 0;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (fn->constructable.callback)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value v;
            /* A factory that leaves its output untouched has not constructed an Object. */
            fvm_value_undefined(&v);
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
            /* Never publish null or foreign-context factory results to the caller. */
            if (v.type != FVM_VALUE_OBJECT || !v.object || v.object->owner != c)
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
        /* [GC:] An unloaded host prototype getter can collect before invocation
         * publishes this new instance as its receiver. Retain it across that gap. */
        FVM_ObjectRootScope instance_root(c, instance);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        instance->resource_kind = FVM_RESOURCE_OBJECT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value proto;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        if (c->swf_loaded)
        {
            /* AVM1 construction uses own stored data, not an ordinary property get. */
            fvm_value_undefined(&proto);
            r = fvm_avm1_stored_prototype(c, fn, proto);
        }
        else
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
        /* Define the loaded receiver's own identities before user constructor code runs. */
        r = fvm_function_define_constructor(c, instance, fn);
        if (r != FVM_OK)
            return r;
        FVM_Value result;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        /* [GC:] Retain constructor identity and restore the caller mode on every exit. */
        FVM_CallScope constructor_scope(c, fn, fn, instance->prototype);
        r = fvm_function_invoke(c, fn, instance, n, a, &result);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        /* Loaded AVM1 script constructors ignore every returned value. The unloaded
           embedding namespace retains its existing object-return substitution. */
        *out = !c->swf_loaded && result.type == FVM_VALUE_OBJECT ? result.object : instance;
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
    return cb == fvm_super_callback() ? 808
           : cb == script             ? 800
           : cb == call               ? 801
           : cb == apply              ? 802
           : cb == function_text      ? 803
           : cb == empty              ? 804
           : cb == constructor        ? 805
           : cb == argument_get       ? 806
           : cb == argument_set       ? 807
                                      : 0;
}

/* Restore a native function callback from its stable serialization identifier. */
FVM_NativeFunction fvm_function_callback_from_id(unsigned long id)
{
    return id == 808   ? fvm_super_callback()
           : id == 800 ? script
           : id == 801 ? call
           : id == 802 ? apply
           : id == 803 ? function_text
           : id == 804 ? empty
           : id == 805 ? constructor
           : id == 806 ? argument_get
           : id == 807 ? argument_set
                       : 0;
}

/* Compute and return function collector callback. */
bool fvm_function_collector_callback(FVM_NativeFunction cb)
{
    return cb == script || cb == call || cb == apply || cb == fvm_super_callback();
}

/* [GC:] Link an arguments index to an activation using a traced accessor descriptor. */
FVM_Result fvm_function_map_argument(FVM_Context *c, FVM_Object *arguments, unsigned long index,
                                     FVM_Object *activation, const fvm_detail::string &name)
{
    FVM_Object *descriptor = 0;
    FVM_Result r = fvm_object_create(c, &descriptor);
    if (r != FVM_OK)
        return r;
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = activation;
    r = fvm_object_set(c, descriptor, "scope", &value);
    if (r != FVM_OK)
        return r;
    fvm_value_string_bytes(&value, name.data(), name.size());
    r = fvm_object_set(c, descriptor, "name", &value);
    if (r != FVM_OK)
        return r;
    char key[32];
    std::sprintf(key, "%lu", index);
    arguments->indexed.erase(index);
    return fvm_object_define_accessor_rooted(c, arguments, key, argument_get, argument_set,
                                             descriptor, descriptor);
}
