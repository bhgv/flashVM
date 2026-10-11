#include "error_state.hpp"
#include "flashvm/standard.h"
#include <cstring>

namespace
{
enum
{
    ERROR_OBJECT = 22,
    ERROR_CLASS = 23
};

FVM_Result error_string(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *,
                        FVM_Value *out, void *)
{
    /* AS2 Error.toString returns the message, without a name prefix. It is
     * intentionally generic: a borrowed method can inspect another receiver. */
    FVM_Value message;
    FVM_Result result = fvm_object_get(c, self, "message", &message);
    return result == FVM_OK ? fvm_value_to_string(c, &message, out) : result;
}

FVM_Result error_constructor(FVM_Context *c, FVM_Object *, unsigned int count,
                             const FVM_Value *args, FVM_Value *out, void *userdata)
{
    FVM_Object *klass = (FVM_Object *)userdata;
    FVM_Value prototype;
    FVM_Result result = fvm_object_get(c, klass, "prototype", &prototype);
    if (result != FVM_OK)
        return result;
    if (prototype.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *object = 0;
    result = fvm_object_create(c, &object);
    if (result != FVM_OK)
        return result;
    object->resource_kind = ERROR_OBJECT;
    object->prototype = prototype.object;
    /* Preserve the supplied value rather than coercing it at construction.
     * Omitted/undefined message inherits the prototype's default. */
    if (count && args[0].type != FVM_VALUE_UNDEFINED)
        result = fvm_object_set(c, object, "message", &args[0]);
    if (result != FVM_OK)
        return result;
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = object;
    return FVM_OK;
}
} // namespace

FVM_Result fvm_error_stage(FVM_Context *c, fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
                           fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{
    FVM_Object *klass = 0, *prototype = 0;
    FVM_Result result = fvm_object_create(c, &klass);
    if (result != FVM_OK)
        return result;
    result = fvm_object_create(c, &prototype);
    if (result != FVM_OK)
        return result;
    klass->resource_kind = ERROR_CLASS;
    prototype->resource_kind = ERROR_OBJECT;
    FVM_Value value;
    fvm_value_string(&value, "Error");
    prototype->properties["name"] = value;
    prototype->properties["message"] = value;
    FVM_NativeEntry method;
    method.callback = error_string;
    prototype->methods["toString"] = method;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = klass;
    prototype->properties["constructor"] = value;
    globals["Error"] = value;
    value.object = prototype;
    klass->properties["prototype"] = value;
    FVM_NativeEntry constructor;
    constructor.callback = error_constructor;
    constructor.userdata = klass;
    constructor.root = klass;
    constructors["Error"] = constructor;
    return FVM_OK;
}

FVM_Result fvm_error_validate(FVM_Context *c)
{
    FVM_Value klass, prototype;
    if (fvm_get_global(c, "Error", &klass) != FVM_OK || klass.type != FVM_VALUE_OBJECT ||
        klass.object->resource_kind != ERROR_CLASS)
        return FVM_INVALID_ARGUMENT;
    if (fvm_object_get(c, klass.object, "prototype", &prototype) != FVM_OK ||
        prototype.type != FVM_VALUE_OBJECT || prototype.object->resource_kind != ERROR_OBJECT)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
        c->constructors.find("Error");
    if (ctor == c->constructors.end() || ctor->second.callback != error_constructor ||
        ctor->second.root != klass.object || ctor->second.userdata != klass.object)
        return FVM_INVALID_ARGUMENT;
    FVM_NativeEntry method;
    return fvm_lookup_native_method(prototype.object, "toString", &method) == FVM_OK &&
                   method.callback == error_string
               ? FVM_OK
               : FVM_INVALID_ARGUMENT;
}

void fvm_error_attach(FVM_Context *c, FVM_Object *error)
{
    /* Resource materialization must not run user getters while assembling the
     * pending exception. Read registry metadata directly and provide a method
     * even when the optional standard library is absent or has been modified. */
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
        c->constructors.find("Error");
    if (ctor != c->constructors.end() && ctor->second.callback == error_constructor &&
        ctor->second.root)
    {
        FVM_Object *klass = ctor->second.root;
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator proto =
            klass->properties.find("prototype");
        if (proto != klass->properties.end() && proto->second.type == FVM_VALUE_OBJECT)
            error->prototype = proto->second.object;
    }
    error->resource_kind = ERROR_OBJECT;
    FVM_NativeEntry method;
    method.callback = error_string;
    error->methods["toString"] = method;
}

unsigned long fvm_error_callback_id(FVM_NativeFunction callback)
{
    return callback == error_string ? 301UL : 0UL;
}
FVM_NativeFunction fvm_error_callback_from_id(unsigned long id)
{
    return id == 301 ? error_string : 0;
}
