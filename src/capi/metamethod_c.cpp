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
    FVM_Result fvm_meta_finalize(FVM_Context *c, FVM_Object *o)
    {
        if (!c || !o || o->owner != c)
            return FVM_INVALID_ARGUMENT;
        if (o->finalized)
            return FVM_OK;
        o->finalized = true;
        FVM_Object *m = o->metatable;
        if (!m)
            return FVM_OK;
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator it =
            m->metamethods.find("__gc");
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator script =
            m->properties.find("__gc");
        if (it == m->metamethods.end() &&
            (script == m->properties.end() || (script->second.type != FVM_VALUE_STRING &&
                                               (script->second.type != FVM_VALUE_OBJECT ||
                                                !script->second.object->invocable.callback))))
            return FVM_OK;
        FVM_Value args[3], result;
        fvm_value_undefined(&args[0]);
        args[0].type = FVM_VALUE_OBJECT;
        args[0].object = o;
        args[1] = args[0];
        args[1].object = m;
        args[2] = args[0];
        fvm_value_undefined(&result);
        if (it == m->metamethods.end())
            return script->second.type == FVM_VALUE_OBJECT
                       ? fvm_function_invoke(c, script->second.object, o, 3, args, &result)
                       : fvm_avm1_call(c, script->second.string, o, 3, args, &result);
        FVM_NativeEntry entry = it->second;
        try
        {
            FVM_NativeCallGuard native_guard(c);
            return entry.callback(c, o, 3, args, &result, entry.userdata);
        }
        catch (...)
        {
            return FVM_ERROR;
        }
    }
    static FVM_Result iterate(FVM_Context *c, FVM_Object *o, unsigned long index, FVM_Value *key,
                              FVM_Value *value)
    {
        if (!c || !o || o->owner != c || !key || !value)
            return FVM_INVALID_ARGUMENT;
        fvm_value_undefined(key);
        fvm_value_undefined(value);
        FVM_Object *m = o->metatable;
        if (m)
        {
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator it =
                m->metamethods.find("__iterate");
            if (it != m->metamethods.end())
            {
                FVM_Value args[3];
                fvm_value_undefined(&args[0]);
                args[0].type = FVM_VALUE_OBJECT;
                args[0].object = o;
                args[1] = args[0];
                args[1].object = m;
                fvm_value_number(&args[2], (double)index);
                try
                {
                    FVM_NativeCallGuard native_guard(c);
                    return it->second.callback(c, o, 3, args, value, it->second.userdata);
                }
                catch (...)
                {
                    return FVM_ERROR;
                }
            }
        }
        /* Default own iteration keeps numeric keys and skips DONT_ENUM slots. */
        unsigned long position = 0;
        for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = o->indexed.begin();
             i != o->indexed.end(); ++i)
        {
            char name[32];
            std::sprintf(name, "%lu", i->first);
            fvm_detail::map<fvm_detail::string, unsigned int>::iterator flag =
                o->attributes.find(name);
            if (flag != o->attributes.end() && (flag->second & 1))
                continue;
            if (position++ == index)
            {
                fvm_value_number(key, (double)i->first);
                *value = i->second;
                return FVM_OK;
            }
        }
        for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i = o->properties.begin();
             i != o->properties.end(); ++i)
        {
            fvm_detail::map<fvm_detail::string, unsigned int>::iterator flag =
                o->attributes.find(i->first);
            if (flag != o->attributes.end() && (flag->second & 1))
                continue;
            if (position++ == index)
            {
                fvm_value_string(key, i->first.c_str());
                *value = i->second;
                return FVM_OK;
            }
        }
        return FVM_OK;
    }
}

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

extern "C" FVM_Result fvm_meta_register_rooted(FVM_Context *c, FVM_Object *m, const char *name,
                                               FVM_NativeFunction cb, void *ud, FVM_Object *root)
{
    if (!c || (root && root->owner != c))
        return FVM_INVALID_ARGUMENT;
    FVM_Result r = fvm_meta_register(c, m, name, cb, ud);
    if (r == FVM_OK)
        m->metamethods.find(name)->second.root = root;
    return r;
}
