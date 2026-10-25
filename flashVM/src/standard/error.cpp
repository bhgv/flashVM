/* FlashVM: Built-in ActionScript objects: error.
 * File: src/standard/error.cpp.
 */

#include "error_state.hpp"
#include "error_constants.h"
#include "constructor_identity.hpp"
#include "../core/object_root_scope.hpp"
#include "flashvm/standard.h"
#include <cstring>

namespace
{ /* Define anonymous enum and its representation. */


enum
{
    ERROR_OBJECT = FVM_RESOURCE_ERROR,
    ERROR_CLASS = FVM_RESOURCE_ERROR_CLASS
};


/* Compute and return error string. */
FVM_Result error_string(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *, FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    /* AS2 Error.toString returns the message, without a name prefix. It is
     * intentionally generic: a borrowed method can inspect another receiver. */
    if (c->swf_loaded)
    {
        return fvm_object_get(c, self, "message", out);
    }
    FVM_Value message;
    FVM_Result result = fvm_object_get(c, self, "message", &message);
    return result == FVM_OK ? fvm_value_to_string(c, &message, out) : result;
}


/* Compute and return error constructor. */
FVM_Result error_constructor(
    FVM_Context *c, FVM_Object *, unsigned int count, const FVM_Value *args, FVM_Value *out, void *userdata)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *klass = (FVM_Object *)userdata;
    FVM_Object *object = 0;
    FVM_Result result = FVM_OK;
    if (c->swf_loaded)
    {
        result = fvm_function_allocate_instance(c, klass, ERROR_OBJECT, &object);
        if (result != FVM_OK)
        {
            return result;
        }
    }
    else
    {
        FVM_Value prototype;
        result = fvm_object_get(c, klass, "prototype", &prototype);
        if (result != FVM_OK)
        {
            return result;
        }
        if (prototype.type != FVM_VALUE_OBJECT)
        {
            return FVM_INVALID_ARGUMENT;
        }
        result = fvm_object_create(c, &object);
        if (result != FVM_OK)
        {
            return result;
        }
        object->resource_kind = ERROR_OBJECT;
        object->prototype = prototype.object;
    }


    /* [GC:] A message setter can detach the class or receiver before publication. */
    FVM_ObjectRootScope object_root(c, object);
    /* Preserve the supplied value rather than coercing it at construction.
     * Omitted/undefined message inherits the prototype's default. */
    if (count && args[0].type != FVM_VALUE_UNDEFINED)
    {
        result = fvm_object_set(c, object, "message", &args[0]);
    }
    if (result != FVM_OK)
    {
        return result;
    }
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    out->type = FVM_VALUE_OBJECT;
    out->object = object;
    return FVM_OK;
}


/* Error() is a script-style initializer on the calling receiver, returning undefined. */
FVM_Result
error_call(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args, FVM_Value *out, void *userdata)
{
    if (!c->swf_loaded)
    {
        return error_constructor(c, self, count, args, out, userdata);
    }
    FVM_Result result = FVM_OK;
    if (count && args[0].type != FVM_VALUE_UNDEFINED)
    {
        result = fvm_object_set(c, self ? self : c->global, "message", args);
    }
    if (result == FVM_OK)
    {
        fvm_value_undefined(out);
    }
    return result;
}
} // namespace


/* Build constructor and prototype for error. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
fvm_error_stage(FVM_Context *c,
                fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors,
                fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &functions)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *klass = 0, *prototype = 0;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result result = fvm_object_create(c, &klass);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (result != FVM_OK)
    { /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return result;
    }
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    result = fvm_object_create(c, &prototype);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (result != FVM_OK)
    { /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return result;
    }

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    klass->resource_kind = ERROR_CLASS;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    prototype->resource_kind = ERROR_OBJECT;

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value value;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_string(&value, "Error");

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    prototype->properties["name"] = value;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    prototype->properties["message"] = value;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeEntry method;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    method.callback = error_string;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    prototype->methods["toString"] = method;
    if (c->swf_loaded)
    {
        prototype->attributes["toString"] = FVM_STANDARD_SCRIPT_METHOD_FLAGS;
    }
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_undefined(&value);

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    value.type = FVM_VALUE_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    value.object = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    prototype->properties["constructor"] = value;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    globals["Error"] = value;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    value.object = prototype;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    klass->properties["prototype"] = value;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeEntry constructor;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    constructor.callback = error_constructor;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    constructor.userdata = klass;
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    constructor.root = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    constructors["Error"] = constructor;
    constructor.callback = error_call;
    functions["Error"] = constructor;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}


/* Stage the ordinary script-defined toString flags without replacing user state. */
void fvm_error_prepare_loaded(FVM_Context *c, FVM_MethodAttributeActivation &out)
{
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor = c->constructors.find("Error");
    if (ctor == c->constructors.end() || ctor->second.callback != error_constructor || !ctor->second.root)
    {
        return;
    }
    FVM_Object *klass = ctor->second.root;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator stored = klass->properties.find("prototype");
    if (stored == klass->properties.end() || stored->second.type != FVM_VALUE_OBJECT)
    {
        return;
    }
    FVM_Object *proto = stored->second.object;
    if (proto->resource_kind != ERROR_OBJECT)
    {
        return;
    }
    out.attributes = proto->attributes;
    fvm_stage_default_method_flags(proto, "toString", error_string, FVM_STANDARD_SCRIPT_METHOD_FLAGS, out.attributes);
    out.prototype = proto;
}


/* Validate error. */


/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result fvm_error_validate(FVM_Context *c)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{ /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value klass, prototype;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (fvm_get_global(c, "Error", &klass) != FVM_OK || klass.type != FVM_VALUE_OBJECT ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        klass.object->resource_kind != ERROR_CLASS)
    { /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    }
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (fvm_object_get(c, klass.object, "prototype", &prototype) != FVM_OK ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        prototype.type != FVM_VALUE_OBJECT || prototype.object->resource_kind != ERROR_OBJECT)
    { /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    }
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->constructors.find("Error");
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (ctor == c->constructors.end() || ctor->second.callback != error_constructor ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        ctor->second.root != klass.object || ctor->second.userdata != klass.object)
    { /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    }
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeEntry method;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return fvm_lookup_native_method(prototype.object, "toString", &method) == FVM_OK &&
                   /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                   method.callback == error_string
               /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
               ? FVM_OK
               /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
               : FVM_INVALID_ARGUMENT;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}


/* Implement error attach for this subsystem. */


/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
void fvm_error_attach(FVM_Context *c, FVM_Object *error)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* Resource materialization must not run user getters while assembling the
     * pending exception. Read registry metadata directly and provide a method
     * even when the optional standard library is absent or has been modified. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        c->constructors.find("Error");
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (ctor != c->constructors.end() && ctor->second.callback == error_constructor &&
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        ctor->second.root)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    { /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *klass = ctor->second.root;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator proto =
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            klass->properties.find("prototype");
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (proto != klass->properties.end() && proto->second.type == FVM_VALUE_OBJECT)
        { /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            error->prototype = proto->second.object;
        }
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    error->resource_kind = ERROR_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeEntry method;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    method.callback = error_string;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    error->methods["toString"] = method;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}


/* Map a native error callback to its stable serialization identifier. */
unsigned long fvm_error_callback_id(FVM_NativeFunction callback)
{
    return callback == error_string        ? FVM_ERROR_CALLBACK_STRING
           : callback == error_constructor ? FVM_ERROR_CALLBACK_CONSTRUCTOR
           : callback == error_call        ? FVM_ERROR_CALLBACK_CALL
                                           : 0UL;
}


/* Restore a native error callback from its stable serialization identifier. */
FVM_NativeFunction fvm_error_callback_from_id(unsigned long id)
{
    return id == FVM_ERROR_CALLBACK_STRING        ? error_string
           : id == FVM_ERROR_CALLBACK_CONSTRUCTOR ? error_constructor
           : id == FVM_ERROR_CALLBACK_CALL        ? error_call
                                                  : 0;
}
