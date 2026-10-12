#include "../core/property_watch.hpp"
#include "../standard/base_state.hpp"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include <cstring>
#include <new>
bool fvm_property_watchable(FVM_Object *object, const char *name)
{
    if (object->resource_kind == 40)
    {
        const char *fields[] = {"_x",     "_y",       "_xscale", "_yscale", "_rotation",
                                "_alpha", "_visible", "_name",   "enabled", "useHandCursor"};
        for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
            if (!std::strcmp(name, fields[i]))
                return false;
    }
    unsigned long index = 0;
    const bool indexed = fvm_as_index(name, &index);
    unsigned int depth = 0;
    for (FVM_Object *o = object; o && depth++ < 1024; o = o->prototype)
    {
        if (indexed && o->indexed.find(index) != o->indexed.end())
            return true;
        if (o->accessors.find(name) != o->accessors.end())
            return false;
        if (o->properties.find(name) != o->properties.end())
            return true;
    }
    return depth < 1024;
}
FVM_Result fvm_property_watch_prepare(FVM_Context *c, FVM_Object *object, const char *name,
                                      const FVM_Value *requested, FVM_Value *out)
{
    if (!c || !object || object->owner != c || !name || !out ||
        fvm_validate_value(c, requested) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    *out = *requested;
    try
    {
        fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator entry =
            object->watchers.find(name);
        if (entry == object->watchers.end() || !fvm_property_watchable(object, name))
            return FVM_OK;
        FVM_Object *descriptor = entry->second;
        for (size_t i = 0; i < c->watch_running.size(); ++i)
            if (c->watch_running[i] == descriptor)
                return FVM_OK;
        fvm_detail::string key(name);
        fvm_detail::vector<FVM_Value> roots;
        FVM_Value root;
        fvm_value_undefined(&root);
        root.type = FVM_VALUE_OBJECT;
        root.object = descriptor;
        roots.push_back(root);
        roots.push_back(*requested);
        FVM_GCRootFrame frame;
        frame.previous = c->gc_frames;
        frame.stack = &roots;
        frame.locals = 0;
        frame.receiver = object;
        c->watch_running.push_back(descriptor);
        c->gc_frames = &frame;
        struct Pop
        {
            FVM_Context *c;
            FVM_GCRootFrame *previous;
            ~Pop()
            {
                c->gc_frames = previous;
                c->watch_running.pop_back();
            }
        } pop;
        pop.c = c;
        pop.previous = frame.previous;
        FVM_Value args[4], handler, result;
        fvm_value_string(args, key.c_str());
        FVM_Result r = fvm_object_get(c, object, key.c_str(), args + 1);
        if (r != FVM_OK)
            return r;
        roots.push_back(args[1]);
        args[2] = *requested;
        r = fvm_object_get(c, descriptor, "userData", args + 3);
        if (r != FVM_OK)
            return r;
        roots.push_back(args[3]);
        r = fvm_object_get(c, descriptor, "handler", &handler);
        if (r != FVM_OK)
            return r;
        if (handler.type != FVM_VALUE_OBJECT || !handler.object->invocable.callback)
            return FVM_INVALID_ARGUMENT;
        r = fvm_function_invoke(c, handler.object, object, 4, args, &result);
        if (r != FVM_OK)
            return r;
        if (result.type == FVM_VALUE_STRING)
        {
            r = fvm_value_to_string(c, &result, out);
            if (r == FVM_OK)
                out->metatable = result.metatable;
            return r;
        }
        *out = result;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
