#include "../core/native_call_guard.hpp"
#include "../core/runtime.hpp"
#include "../core/exception_internal.hpp"
#include "flashvm/stack.h"
#include <cstring>
#include <new>
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
static FVM_Result copy_value(FVM_Context *context, const FVM_Value *src, FVM_Value *dst)
{
    FVM_Result r = fvm_validate_value(context, src);
    if (r != FVM_OK)
        return r;
    *dst = *src;
    if (src->type == FVM_VALUE_STRING)
    {
        size_t n = std::strlen(src->string);
        char *p = new (std::nothrow) char[n + 1];
        if (!p)
            return FVM_OUT_OF_MEMORY;
        std::memcpy(p, src->string, n + 1);
        try
        {
            context->strings.push_back(p);
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
FVM_Result fvm_lookup_native_method(FVM_Object *object, const char *name, FVM_NativeEntry *out)
{
    if (!object || !name || !out)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::string key(name);
        for (unsigned int depth = 0; object && depth < 1024; ++depth)
        {
            /* Function-valued data/accessor dispatch is still unsupported. A data
             * assignment must nevertheless shadow a previously registered method. */
            if (object->properties.find(key) != object->properties.end() ||
                object->accessors.find(key) != object->accessors.end())
                return FVM_UNSUPPORTED;
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method =
                object->methods.find(key);
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
extern "C"
{
    void fvm_value_undefined(FVM_Value *v)
    {
        if (v)
        {
            std::memset(v, 0, sizeof(*v));
            v->type = FVM_VALUE_UNDEFINED;
        }
    }
    void fvm_value_number(FVM_Value *v, double n)
    {
        if (v)
        {
            fvm_value_undefined(v);
            v->type = FVM_VALUE_NUMBER;
            v->number = n;
        }
    }
    void fvm_value_boolean(FVM_Value *v, int b)
    {
        if (v)
        {
            fvm_value_undefined(v);
            v->type = FVM_VALUE_BOOLEAN;
            v->boolean = !!b;
        }
    }
    FVM_Result fvm_value_string(FVM_Value *v, const char *s)
    {
        if (!v || !s)
            return FVM_INVALID_ARGUMENT;
        fvm_value_undefined(v);
        v->type = FVM_VALUE_STRING;
        v->string = s;
        return FVM_OK;
    }
    FVM_Result fvm_object_create(FVM_Context *c, FVM_Object **out)
    {
        FVM_Object *o;
        if (!c || !out || c->destroying || c->gc_releasing)
            return FVM_INVALID_ARGUMENT;
        *out = 0;
        o = new (std::nothrow) FVM_Object;
        if (!o)
            return FVM_OUT_OF_MEMORY;
        o->owner = c;
        o->prototype = 0;
        o->metatable = 0;
        o->finalized = false;
        o->gc_marked = false;
        o->gc_pins = 0;
        o->callable.callback = 0;
        o->callable.userdata = 0;
        o->resource_result_valid = false;
        fvm_value_undefined(&o->resource_result);
        o->resource_origin = 0;
        o->resource_kind = 0;
        o->resource = 0;
        o->release_resource = 0;
        try
        {
            c->objects.push_back(o);
        }
        catch (...)
        {
            delete o;
            return FVM_OUT_OF_MEMORY;
        }
        *out = o;
        if (c->gc_allocations != (unsigned long)-1)
            ++c->gc_allocations;
        return FVM_OK;
    }
    FVM_Result fvm_object_set_prototype(FVM_Context *c, FVM_Object *o, FVM_Object *proto)
    {
        FVM_Object *cursor;
        if (!c || !o || o->owner != c || (proto && proto->owner != c))
            return FVM_INVALID_ARGUMENT;
        cursor = proto;
        while (cursor)
        {
            if (cursor == o)
                return FVM_INVALID_ARGUMENT;
            cursor = cursor->prototype;
        }
        o->prototype = proto;
        return FVM_OK;
    }
    FVM_Result fvm_object_get_prototype(FVM_Context *c, FVM_Object *o, FVM_Object **out)
    {
        if (!c || !o || o->owner != c || !out)
            return FVM_INVALID_ARGUMENT;
        *out = o->prototype;
        return FVM_OK;
    }
    FVM_Result fvm_object_has_own(FVM_Context *c, FVM_Object *o, const char *name, int *out)
    {
        if (!c || !o || o->owner != c || !name || !out)
            return FVM_INVALID_ARGUMENT;
        try
        {
            fvm_detail::string key(name);
            *out = o->properties.find(key) != o->properties.end() ||
                   o->accessors.find(key) != o->accessors.end() ||
                   o->methods.find(key) != o->methods.end();
            return FVM_OK;
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
    }
    /* AVM1 prototype lookup takes precedence over Lua-style __index fallback.
     * Only table-valued metamethods are supported at this stage. */
    FVM_Result fvm_object_get(FVM_Context *c, FVM_Object *o, const char *name, FVM_Value *out)
    {
        FVM_Object *receiver = o;
        unsigned int depth = 0;
        if (!c || !o || o->owner != c || !name || !out)
            return FVM_INVALID_ARGUMENT;
        while (o && depth++ < 1024)
        {
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator it = o->properties.find(name);
            if (it != o->properties.end())
            {
                *out = it->second;
                return FVM_OK;
            }
            fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor =
                o->accessors.find(name);
            if (accessor != o->accessors.end())
            {
                FVM_AccessorEntry entry = accessor->second;
                fvm_value_undefined(out);
                if (!entry.getter)
                    return FVM_OK;
                try
                {
                    FVM_NativeCallGuard guard(c);
                    fvm_resource_fault_begin(c);
                    return fvm_exception_complete(
                        c, receiver, entry.getter(c, receiver, 0, 0, out, entry.userdata));
                }
                catch (...)
                {
                    return FVM_ERROR;
                }
            }
            o = o->prototype;
        }
        if (o)
            return FVM_ERROR;
        o = receiver;
        for (depth = 0; o && depth < 1024; ++depth)
        {
            if (!o->metatable)
                break;
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator m =
                o->metatable->properties.find("__index");
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator fn =
                o->metatable->metamethods.find("__index");
            if (fn != o->metatable->metamethods.end())
            {
                FVM_Value args[3];
                FVM_Value result;
                fvm_value_undefined(&args[0]);
                args[0].type = FVM_VALUE_OBJECT;
                args[0].object = receiver;
                args[1] = args[0];
                args[1].object = o->metatable;
                fvm_value_string(&args[2], name);
                fvm_value_undefined(&result);
                try
                {
                    FVM_NativeCallGuard native_guard(c);
                    return fn->second.callback(c, receiver, 3, args, &result,
                                               fn->second.userdata) == FVM_OK
                               ? (*out = result, FVM_OK)
                               : FVM_ERROR;
                }
                catch (...)
                {
                    return FVM_ERROR;
                }
            }
            if (m == o->metatable->properties.end())
                break;
            if (m->second.type != FVM_VALUE_OBJECT || !m->second.object)
                break;
            o = m->second.object;
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator it = o->properties.find(name);
            if (it != o->properties.end())
            {
                *out = it->second;
                return FVM_OK;
            }
        }
        if (depth == 1024)
            return FVM_ERROR;
        fvm_value_undefined(out);
        return FVM_OK;
    }
    FVM_Result fvm_object_set(FVM_Context *c, FVM_Object *o, const char *name, const FVM_Value *v)
    {
        FVM_Value copy;
        unsigned int depth = 0;
        if (!c || !o || o->owner != c || !name)
            return FVM_INVALID_ARGUMENT;
        FVM_Result r = copy_value(c, v, &copy);
        if (r != FVM_OK)
            return r;
        /* Own data shadows inherited accessors. An own accessor always wins
         * over prototype data, and setters bind the original receiver. */
        FVM_Object *cursor = o;
        unsigned int accessor_depth = 0;
        while (cursor && accessor_depth++ < 1024)
        {
            if (cursor->properties.find(name) != cursor->properties.end())
                break;
            fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator found =
                cursor->accessors.find(name);
            if (found != cursor->accessors.end())
            {
                FVM_AccessorEntry entry = found->second;
                if (!entry.setter)
                    return FVM_OK;
                FVM_Value result;
                fvm_value_undefined(&result);
                try
                {
                    FVM_NativeCallGuard guard(c);
                    return entry.setter(c, o, 1, &copy, &result, entry.userdata);
                }
                catch (...)
                {
                    return FVM_ERROR;
                }
            }
            cursor = cursor->prototype;
        }
        if (accessor_depth > 1024)
            return FVM_ERROR;
        /* An existing own property is always written directly. */
        if (o->properties.find(name) == o->properties.end())
        {
            while (o->metatable && depth++ < 1024)
            {
                fvm_detail::map<fvm_detail::string, FVM_Value>::iterator m =
                    o->metatable->properties.find("__newindex");
                fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator fn =
                    o->metatable->metamethods.find("__newindex");
                if (fn != o->metatable->metamethods.end())
                {
                    FVM_Value args[4], result;
                    fvm_value_undefined(&args[0]);
                    args[0].type = FVM_VALUE_OBJECT;
                    args[0].object = o;
                    args[1] = args[0];
                    args[1].object = o->metatable;
                    fvm_value_string(&args[2], name);
                    args[3] = copy;
                    fvm_value_undefined(&result);
                    try
                    {
                        FVM_NativeCallGuard native_guard(c);
                        return fn->second.callback(c, o, 4, args, &result, fn->second.userdata);
                    }
                    catch (...)
                    {
                        return FVM_ERROR;
                    }
                }
                if (m == o->metatable->properties.end() || m->second.type != FVM_VALUE_OBJECT ||
                    !m->second.object)
                    break;
                o = m->second.object;
                if (o->properties.find(name) != o->properties.end())
                    break;
            }
            if (depth >= 1024)
                return FVM_ERROR;
        }
        try
        {
            o->properties[name] = copy;
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }
    FVM_Result fvm_get_global(FVM_Context *c, const char *name, FVM_Value *out)
    {
        return c ? fvm_object_get(c, c->global, name, out) : FVM_INVALID_ARGUMENT;
    }
    FVM_Result fvm_set_global(FVM_Context *c, const char *name, const FVM_Value *v)
    {
        return c ? fvm_object_set(c, c->global, name, v) : FVM_INVALID_ARGUMENT;
    }
    FVM_Result fvm_register_function(FVM_Context *c, const char *name, FVM_NativeFunction cb,
                                     void *user)
    {
        FVM_NativeEntry entry;
        if (!c || !name || !*name || !cb)
            return FVM_INVALID_ARGUMENT;
        entry.callback = cb;
        entry.userdata = user;
        try
        {
            c->functions[name] = entry;
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }
    FVM_Result fvm_register_function_rooted(FVM_Context *c, const char *name, FVM_NativeFunction cb,
                                            void *user, FVM_Object *root)
    {
        if (!c || (root && root->owner != c))
            return FVM_INVALID_ARGUMENT;
        FVM_Result r = fvm_register_function(c, name, cb, user);
        if (r == FVM_OK)
            c->functions.find(name)->second.root = root;
        return r;
    }
    FVM_Result fvm_object_call(FVM_Context *c, FVM_Object *self, const char *name,
                               unsigned int argc, const FVM_Value *argv, FVM_Value *out)
    {
        unsigned int i;
        if (!c || !self || self->owner != c || !name || !out || (argc && !argv))
            return FVM_INVALID_ARGUMENT;
        for (i = 0; i < argc; ++i)
            if (fvm_validate_value(c, &argv[i]) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
        FVM_NativeEntry entry;
        FVM_Result lookup = fvm_lookup_native_method(self, name, &entry);
        if (lookup != FVM_OK && lookup != FVM_NOT_FOUND)
            return lookup;
        if (lookup == FVM_NOT_FOUND)
        {
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator it =
                c->functions.find(name);
            if (it == c->functions.end())
                return FVM_UNSUPPORTED;
            entry = it->second;
        }
        fvm_resource_fault_begin(c);
        fvm_value_undefined(out);
        try
        {
            if (entry.collector_entry)
            {
                fvm_detail::vector<FVM_Value> args;
                for (i = 0; i < argc; ++i)
                    args.push_back(argv[i]);
                FVM_GCRootFrame frame;
                frame.previous = c->gc_frames;
                frame.stack = &args;
                frame.locals = 0;
                frame.receiver = self;
                c->gc_frames = &frame;
                struct Pop
                {
                    FVM_Context *c;
                    FVM_GCRootFrame *f;
                    ~Pop()
                    {
                        c->gc_frames = f->previous;
                    }
                } pop;
                pop.c = c;
                pop.f = &frame;
                return fvm_exception_complete(
                    c, self, entry.callback(c, self, argc, argv, out, entry.userdata));
            }
            FVM_NativeCallGuard native_guard(c);
            return fvm_exception_complete(c, self,
                                          entry.callback(c, self, argc, argv, out, entry.userdata));
        }
        catch (...)
        {
            return FVM_ERROR;
        }
    }
    FVM_Result fvm_object_define_method_rooted(FVM_Context *c, FVM_Object *object, const char *name,
                                               FVM_NativeFunction callback, void *userdata,
                                               FVM_Object *root)
    {
        if (!c || !object || object->owner != c || !name || !*name || !callback ||
            (root && root->owner != c) || c->destroying || c->gc_releasing)
            return FVM_INVALID_ARGUMENT;
        FVM_NativeEntry entry;
        entry.callback = callback;
        entry.userdata = userdata;
        entry.root = root;
        try
        {
            fvm_detail::string key(name);
            if (object->properties.find(key) != object->properties.end() ||
                object->accessors.find(key) != object->accessors.end())
                return FVM_INVALID_ARGUMENT;
            object->methods[key] = entry;
            return FVM_OK;
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
    }
    FVM_Result fvm_object_define_accessor_rooted(FVM_Context *c, FVM_Object *object,
                                                 const char *name, FVM_NativeFunction getter,
                                                 FVM_NativeFunction setter, void *userdata,
                                                 FVM_Object *root)
    {
        if (!c || !object || object->owner != c || !name || !*name || (!getter && !setter) ||
            (root && root->owner != c) || c->destroying || c->gc_releasing ||
            object->properties.find(name) != object->properties.end())
            return FVM_INVALID_ARGUMENT;
        FVM_AccessorEntry entry;
        entry.getter = getter;
        entry.setter = setter;
        entry.userdata = userdata;
        entry.root = root;
        try
        {
            object->accessors[name] = entry;
            return FVM_OK;
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
    }
    FVM_Result fvm_register_constructor_rooted(FVM_Context *c, const char *name,
                                               FVM_NativeFunction callback, void *userdata,
                                               FVM_Object *root)
    {
        if (!c || !name || !*name || !callback || std::strcmp(name, "Object") == 0 ||
            (root && root->owner != c) || c->destroying || c->gc_releasing)
            return FVM_INVALID_ARGUMENT;
        FVM_NativeEntry entry;
        entry.callback = callback;
        entry.userdata = userdata;
        entry.root = root;
        try
        {
            c->constructors[name] = entry;
            return FVM_OK;
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
    }
    FVM_Result fvm_construct(FVM_Context *c, const char *name, unsigned int argc,
                             const FVM_Value *argv, FVM_Object **out)
    {
        if (!out)
            return FVM_INVALID_ARGUMENT;
        *out = 0;
        if (!c || !name || (argc && !argv) || c->destroying || c->gc_releasing)
            return FVM_INVALID_ARGUMENT;
        for (unsigned int i = 0; i < argc; ++i)
            if (fvm_validate_value(c, &argv[i]) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
        if (std::strcmp(name, "Object") == 0)
            return argc ? FVM_UNSUPPORTED : fvm_object_create(c, out);
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator found =
            c->constructors.find(name);
        if (found == c->constructors.end())
            return FVM_UNSUPPORTED;
        FVM_NativeEntry entry = found->second;
        FVM_Value result;
        fvm_value_undefined(&result);
        try
        {
            FVM_NativeCallGuard guard(c);
            FVM_Result status = entry.callback(c, c->global, argc, argv, &result, entry.userdata);
            if (status != FVM_OK)
                return status;
            FVM_ValueType object_type = FVM_VALUE_OBJECT;
            if (std::memcmp(&result.type, &object_type, sizeof(object_type)) != 0 ||
                !result.object || result.object->owner != c)
                return FVM_ERROR;
            *out = result.object;
            return FVM_OK;
        }
        catch (...)
        {
            return FVM_ERROR;
        }
    }
    FVM_Result fvm_call(FVM_Context *c, const char *name, unsigned int argc, const FVM_Value *argv,
                        FVM_Value *out)
    {
        unsigned int i;
        if (!c || !name || !out || (argc && !argv))
            return FVM_INVALID_ARGUMENT;
        for (i = 0; i < argc; ++i)
            if (fvm_validate_value(c, &argv[i]) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator it = c->functions.find(name);
        if (it == c->functions.end())
            return fvm_stack_call(c, name, argc, argv, out);
        fvm_value_undefined(out);
        try
        {
            FVM_NativeCallGuard native_guard(c);
            fvm_resource_fault_begin(c);
            return fvm_exception_complete(
                c, c->global,
                it->second.callback(c, c->global, argc, argv, out, it->second.userdata));
        }
        catch (...)
        {
            return FVM_ERROR;
        }
    }
}
