#include "../core/runtime.hpp"
#include "../standard/base_state.hpp"
#include "flashvm/property.h"
#include <new>
extern "C" FVM_Result fvm_object_property_flags(FVM_Context *c, FVM_Object *o, const char *name,
                                                unsigned int *out)
{
    if (!c || !o || o->owner != c || !name || !out)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::map<fvm_detail::string, unsigned int>::iterator i = o->attributes.find(name);
        *out = i == o->attributes.end() ? 0 : i->second;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" FVM_Result fvm_object_set_property_flags(FVM_Context *c, FVM_Object *o, const char *name,
                                                    unsigned int set, unsigned int clear)
{
    if (!c || !o || o->owner != c || !name || (set | clear) > 7)
        return FVM_INVALID_ARGUMENT;
    try
    {
        unsigned int old = 0;
        FVM_Result r = fvm_object_property_flags(c, o, name, &old);
        if (r != FVM_OK)
            return r;
        unsigned int flags = (old | set) & ~clear;
        if (flags)
            o->attributes[name] = flags;
        else
            o->attributes.erase(name);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" FVM_Result fvm_object_delete(FVM_Context *c, FVM_Object *o, const char *name,
                                        int *deleted)
{
    if (!c || !o || o->owner != c || !name || !deleted)
        return FVM_INVALID_ARGUMENT;
    *deleted = 0;
    try
    {
        unsigned int flags = 0;
        FVM_Result r = fvm_object_property_flags(c, o, name, &flags);
        if (r != FVM_OK)
            return r;
        if (flags & 2)
            return FVM_OK;
        unsigned long index;
        if ((o->resource_kind == 28 || o->resource_kind == 30) && fvm_as_index(name, &index))
            o->indexed.erase(index);
        else
        {
            o->properties.erase(name);
            o->methods.erase(name);
            o->accessors.erase(name);
        }
        o->attributes.erase(name);
        *deleted = 1;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
