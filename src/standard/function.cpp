#include "../core/string_internal.hpp"
#include "function_state.hpp"
#include "base_state.hpp"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/table.h"
#include "flashvm/avm1.h"
#include "../core/native_call_guard.hpp"
#include "../core/exception_internal.hpp"
#include <cstring>
#include <new>
#include <cmath>
namespace
{
enum
{
    SCRIPT_FUNCTION = 32,
    FUNCTION_CLASS = 33,
    NATIVE_FUNCTION = 36
};
FVM_Object *function_prototype(FVM_Context *c)
{
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
        c->constructors.find("Function");
    if (ctor == c->constructors.end() || !ctor->second.root)
        return 0;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator p =
        ctor->second.root->properties.find("prototype");
    return p != ctor->second.root->properties.end() && p->second.type == FVM_VALUE_OBJECT
               ? p->second.object
               : 0;
}
FVM_Result create(FVM_Context *c, FVM_Object **out)
{
    FVM_Result r = fvm_object_create(c, out);
    if (r != FVM_OK)
        return r;
    (*out)->resource_kind = NATIVE_FUNCTION;
    (*out)->prototype = function_prototype(c);
    return FVM_OK;
}
FVM_Result empty(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *out,
                 void *)
{
    fvm_value_undefined(out);
    return FVM_OK;
}
FVM_Result function_text(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *)
{
    return fvm_value_string(out, "[type Function]");
}
FVM_Result box_receiver(FVM_Context *c, const FVM_Value *value, FVM_Object *bundle,
                        FVM_Object **out)
{
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
    return fvm_function_invoke(c, fn, receiver, n ? n - 1 : 0, n ? a + 1 : 0, out);
}
FVM_Result apply(FVM_Context *c, FVM_Object *fn, unsigned int n, const FVM_Value *a, FVM_Value *out,
                 void *userdata)
{
    FVM_Object *receiver = n && a[0].type == FVM_VALUE_OBJECT ? a[0].object : c->global;
    /* apply and call use the same primitive receiver boxing rules. */
    if (n && a[0].type != FVM_VALUE_OBJECT && a[0].type != FVM_VALUE_UNDEFINED &&
        a[0].type != FVM_VALUE_NULL)
    {
        FVM_Result r = box_receiver(c, a, (FVM_Object *)userdata, &receiver);
        if (r != FVM_OK)
            return r;
    }
    if (n > 1 && a[1].type != FVM_VALUE_NULL && a[1].type != FVM_VALUE_UNDEFINED)
    {
        if (a[1].type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        FVM_Value length;
        FVM_Result r = fvm_object_get(c, a[1].object, "length", &length);
        if (r != FVM_OK)
            return r;
        double count = 0;
        r = fvm_value_to_number(c, &length, &count);
        if (r != FVM_OK)
            return r;
        if (!(count >= 0 && count <= 65536) || count != std::floor(count))
            return FVM_INVALID_ARGUMENT;
        fvm_detail::vector<FVM_Value> args((size_t)count);
        for (unsigned int i = 0; i < (unsigned int)count; ++i)
        {
            r = fvm_table_get_index(c, a[1].object, i, &args[i]);
            if (r != FVM_OK)
                return r;
        }
        return fvm_function_invoke(c, fn, receiver, (unsigned int)count, args.begin(), out);
    }
    return fvm_function_invoke(c, fn, receiver, 0, 0, out);
}
FVM_Result constructor(FVM_Context *c, FVM_Object *, unsigned int, const FVM_Value *,
                       FVM_Value *out, void *ud)
{
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
    fn->prototype = proto.object;
    fn->invocable.callback = empty;
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = fn;
    return FVM_OK;
}
int hex(unsigned char c)
{
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
}
FVM_Result script(FVM_Context *c, FVM_Object *receiver, unsigned int n, const FVM_Value *a,
                  FVM_Value *out, void *ud)
{
    FVM_Object *fn = (FVM_Object *)ud;
    if (!fn || fn->resource_kind != SCRIPT_FUNCTION || !fn->resource_result_valid ||
        fn->resource_result.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = fn->resource_result.object;
    FVM_Value code, params, constants, closure;
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
    if (code.type != FVM_VALUE_STRING || params.type != FVM_VALUE_OBJECT ||
        constants.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    size_t bytes = fvm_string_size(code);
    if (bytes % 2 || bytes > 131072)
        return FVM_INVALID_ARGUMENT;
    FVM_ScriptFunction body;
    for (size_t i = 0; i < bytes; i += 2)
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
    if (length.type != FVM_VALUE_NUMBER || !(length.number >= 0 && length.number <= 65535) ||
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
        body.parameters.push_back(fvm_detail::string(v.string));
    }
    r = fvm_object_get(c, constants.object, "length", &length);
    if (r != FVM_OK)
        return r;
    if (length.type != FVM_VALUE_NUMBER || !(length.number >= 0 && length.number <= 65535) ||
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
        body.constants.push_back(fvm_detail::string(v.string));
    }
    FVM_Value name;
    r = fvm_object_get(c, data, "name", &name);
    if (r != FVM_OK)
        return r;
    if (name.type == FVM_VALUE_STRING)
        body.name = name.string;
    return fvm_avm1_invoke_script(c, body, closure.type == FVM_VALUE_OBJECT ? closure.object : 0,
                                  receiver, n, a, out, fn);
}
} // namespace
FVM_Result fvm_function_stage(FVM_Context *c,
                              fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
                              fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{
    FVM_Object *klass = 0, *proto = 0;
    FVM_Result r = fvm_object_create(c, &klass);
    if (r != FVM_OK)
        return r;
    r = fvm_object_create(c, &proto);
    if (r != FVM_OK)
        return r;
    klass->resource_kind = FUNCTION_CLASS;
    proto->resource_kind = NATIVE_FUNCTION;
    proto->invocable.callback = empty;
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = klass;
    globals["Function"] = v;
    proto->properties["constructor"] = v;
    v.object = proto;
    klass->properties["prototype"] = v;
    FVM_NativeEntry e;
    e.callback = call;
    e.collector_entry = true;
    proto->methods["call"] = e;
    e.callback = apply;
    proto->methods["apply"] = e;
    e.callback = function_text;
    e.collector_entry = false;
    proto->methods["toString"] = e;
    e.callback = constructor;
    e.root = klass;
    e.userdata = klass;
    constructors["Function"] = e;
    return FVM_OK;
}
FVM_Result fvm_function_validate(FVM_Context *c)
{
    FVM_Value v;
    return fvm_get_global(c, "Function", &v) == FVM_OK && v.type == FVM_VALUE_OBJECT &&
                   v.object->resource_kind == FUNCTION_CLASS
               ? FVM_OK
               : FVM_INVALID_ARGUMENT;
}
FVM_Result fvm_function_wrap_entry(FVM_Context *c, FVM_Object *owner, const char *name,
                                   const FVM_NativeEntry &entry, FVM_Value *out)
{
    fvm_detail::map<fvm_detail::string, FVM_Object *> &cache =
        owner ? owner->method_values : c->function_objects;
    fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator old = cache.find(name);
    FVM_Object *fn = old == cache.end() ? 0 : old->second;
    if (!fn || fn->invocable.callback != entry.callback ||
        fn->invocable.userdata != entry.userdata || fn->invocable.root != entry.root)
    {
        FVM_Result r = create(c, &fn);
        if (r != FVM_OK)
            return r;
        fn->invocable = entry;
        cache[name] = fn;
    }
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = fn;
    return FVM_OK;
}
FVM_Result fvm_function_make_script(FVM_Context *c, const FVM_ScriptFunction &body,
                                    FVM_Object *closure, FVM_Object **out)
{
    FVM_Object *fn = 0, *data = 0, *params = 0, *constants = 0;
    FVM_Result r = create(c, &fn);
    if (r != FVM_OK)
        return r;
    r = fvm_object_create(c, &data);
    if (r != FVM_OK)
        return r;
    r = fvm_array_create_like(c, 0, (unsigned long)body.parameters.size(), &params);
    if (r != FVM_OK)
        return r;
    r = fvm_array_create_like(c, 0, (unsigned long)body.constants.size(), &constants);
    if (r != FVM_OK)
        return r;
    for (size_t i = 0; i < body.parameters.size(); ++i)
    {
        FVM_Value v;
        fvm_value_string(&v, body.parameters[i].c_str());
        r = fvm_table_set_index(c, params, (unsigned long)i, &v);
        if (r != FVM_OK)
            return r;
    }
    for (size_t i = 0; i < body.constants.size(); ++i)
    {
        FVM_Value v;
        fvm_value_string(&v, body.constants[i].c_str());
        r = fvm_table_set_index(c, constants, (unsigned long)i, &v);
        if (r != FVM_OK)
            return r;
    }
    fvm_detail::vector<char> code;
    const char *digits = "0123456789abcdef";
    for (size_t i = 0; i < body.body.size(); ++i)
    {
        code.push_back(digits[body.body[i] >> 4]);
        code.push_back(digits[body.body[i] & 15]);
    }
    code.push_back(0);
    FVM_Value v;
    fvm_value_string(&v, code.begin());
    r = fvm_object_set(c, data, "code", &v);
    if (r != FVM_OK)
        return r;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = params;
    data->properties["parameters"] = v;
    v.object = constants;
    data->properties["constants"] = v;
    v.type = closure ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    v.object = closure;
    data->properties["closure"] = v;
    fvm_value_string(&v, body.name.c_str());
    r = fvm_object_set(c, data, "name", &v);
    if (r != FVM_OK)
        return r;
    fn->resource_kind = SCRIPT_FUNCTION;
    fn->resource_result_valid = true;
    v.type = FVM_VALUE_OBJECT;
    v.object = data;
    fn->resource_result = v;
    fn->invocable.callback = script;
    fn->invocable.root = fn;
    fn->invocable.userdata = fn;
    fn->invocable.collector_entry = true;
    fvm_value_number(&v, (double)body.parameters.size());
    fn->properties["length"] = v;
    fn->attributes["length"] = 7;
    FVM_Object *instance_proto = 0;
    r = fvm_object_create(c, &instance_proto);
    if (r != FVM_OK)
        return r;
    v.type = FVM_VALUE_OBJECT;
    v.object = instance_proto;
    fn->properties["prototype"] = v;
    v.object = fn;
    instance_proto->properties["constructor"] = v;
    instance_proto->attributes["constructor"] = 1;
    *out = fn;
    return FVM_OK;
}
extern "C" FVM_Result fvm_function_create_native(FVM_Context *c, FVM_NativeFunction cb, void *ud,
                                                 FVM_Object *capture, FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || !cb || (capture && capture->owner != c))
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_Result r = create(c, out);
        if (r != FVM_OK)
            return r;
        (*out)->invocable.callback = cb;
        (*out)->invocable.userdata = ud;
        (*out)->invocable.root = capture;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        *out = 0;
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" FVM_Result fvm_function_create_avm1(FVM_Context *c, const void *actions, size_t size,
                                               unsigned int count, const char *const *parameters,
                                               FVM_Object *env, FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || (!actions && size) || (count && !parameters) || (env && env->owner != c) ||
        size > 65536 || count > 65535)
        return FVM_INVALID_ARGUMENT;
    FVM_AVM1Info info;
    info.struct_size = sizeof(info);
    FVM_Result r = fvm_avm1_inspect(actions, size, &info);
    if (r != FVM_OK)
        return r;
    try
    {
        FVM_ScriptFunction body;
        body.body.assign((const unsigned char *)actions, (const unsigned char *)actions + size);
        for (unsigned int i = 0; i < count; ++i)
        {
            if (!parameters[i])
                return FVM_INVALID_ARGUMENT;
            body.parameters.push_back(fvm_detail::string(parameters[i]));
        }
        return fvm_function_make_script(c, body, env, out);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" FVM_Result fvm_function_invoke(FVM_Context *c, FVM_Object *fn, FVM_Object *self,
                                          unsigned int n, const FVM_Value *args, FVM_Value *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    if (!c || !fn || fn->owner != c || (self && self->owner != c) || (n && !args) || c->destroying)
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < n; ++i)
        if (fvm_validate_value(c, args + i) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
    if (!fn->invocable.callback)
        return FVM_UNSUPPORTED;
    try
    {
        fvm_detail::vector<FVM_Value> roots;
        FVM_Value function;
        fvm_value_undefined(&function);
        function.type = FVM_VALUE_OBJECT;
        function.object = fn;
        roots.push_back(function);
        for (unsigned int i = 0; i < n; ++i)
            roots.push_back(args[i]);
        FVM_GCRootFrame frame;
        frame.previous = c->gc_frames;
        frame.stack = &roots;
        frame.locals = 0;
        frame.receiver = self ? self : c->global;
        c->gc_frames = &frame;
        struct Pop
        {
            FVM_Context *c;
            FVM_GCRootFrame *old;
            ~Pop()
            {
                c->gc_frames = old;
            }
        } pop;
        pop.c = c;
        pop.old = frame.previous;
        FVM_NativeEntry entry = fn->invocable;
        fvm_resource_fault_begin(c);
        if (entry.collector_entry)
            return fvm_exception_complete(
                c, frame.receiver, entry.callback(c, frame.receiver, n, args, out, entry.userdata));
        FVM_NativeCallGuard guard(c);
        return fvm_exception_complete(
            c, frame.receiver, entry.callback(c, frame.receiver, n, args, out, entry.userdata));
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
extern "C" FVM_Result fvm_function_construct(FVM_Context *c, FVM_Object *fn, unsigned int n,
                                             const FVM_Value *a, FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || !fn || fn->owner != c || (n && !a))
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < n; ++i)
        if (fvm_validate_value(c, a + i) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
    try
    {
        if (fn->constructable.callback)
        {
            FVM_Value v;
            FVM_NativeCallGuard guard(c);
            FVM_NativeEntry e = fn->constructable;
            FVM_Result r = fvm_exception_complete(c, c->global,
                                                  e.callback(c, c->global, n, a, &v, e.userdata));
            if (r != FVM_OK)
                return r;
            if (v.type != FVM_VALUE_OBJECT)
                return FVM_ERROR;
            *out = v.object;
            return FVM_OK;
        }
        if (!fn->invocable.callback || fn->resource_kind != SCRIPT_FUNCTION)
            return FVM_UNSUPPORTED;
        FVM_Object *instance = 0;
        FVM_Result r = fvm_object_create(c, &instance);
        if (r != FVM_OK)
            return r;
        instance->resource_kind = 30;
        FVM_Value proto;
        r = fvm_object_get(c, fn, "prototype", &proto);
        if (r != FVM_OK)
            return r;
        if (proto.type == FVM_VALUE_OBJECT)
            instance->prototype = proto.object;
        FVM_Value result;
        r = fvm_function_invoke(c, fn, instance, n, a, &result);
        if (r != FVM_OK)
            return r;
        *out = result.type == FVM_VALUE_OBJECT ? result.object : instance;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
unsigned long fvm_function_callback_id(FVM_NativeFunction cb)
{
    return cb == script          ? 800
           : cb == call          ? 801
           : cb == apply         ? 802
           : cb == function_text ? 803
           : cb == empty         ? 804
           : cb == constructor   ? 805
                                 : 0;
}
FVM_NativeFunction fvm_function_callback_from_id(unsigned long id)
{
    return id == 800   ? script
           : id == 801 ? call
           : id == 802 ? apply
           : id == 803 ? function_text
           : id == 804 ? empty
           : id == 805 ? constructor
                       : 0;
}
bool fvm_function_collector_callback(FVM_NativeFunction cb)
{
    return cb == script || cb == call || cb == apply;
}
