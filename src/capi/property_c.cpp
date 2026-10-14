/* FlashVM: C embedding API: property.
 * File: src/capi/property_c.cpp.
 */

#include "../core/runtime.hpp"
#include "../standard/base_state.hpp"
#include "flashvm/property.h"
#include <new>
#include "../core/property_key.hpp"
/* Compute and return object property flags. */
extern "C" FVM_Result fvm_object_property_flags(FVM_Context *c, FVM_Object *o, const char *name,
                                                unsigned int *out)
{
    return name ? fvm_object_property_flags_bytes(c, o, name, std::strlen(name), out)
                : FVM_INVALID_ARGUMENT;
}
/* Apply the same attribute/deletion rules to a canonical counted name. */
extern "C" FVM_Result fvm_object_property_flags_bytes(FVM_Context *c, FVM_Object *o, const char *name, size_t size,
                                                unsigned int *out)
{
    if (!c || !o || o->owner != c || !name || !out)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::string key;
        FVM_Result normalized = fvm_property_key(name, size, key);
        if (normalized != FVM_OK)
            return normalized;
        fvm_detail::map<fvm_detail::string, unsigned int>::iterator i = o->attributes.find(key);
        *out = i == o->attributes.end() ? 0 : i->second;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Update object property flags. */
extern "C" FVM_Result fvm_object_set_property_flags(FVM_Context *c, FVM_Object *o, const char *name,
                                                    unsigned int set, unsigned int clear)
{
    return name ? fvm_object_set_property_flags_bytes(c, o, name, std::strlen(name), set, clear)
                : FVM_INVALID_ARGUMENT;
}
/* Apply the same attribute/deletion rules to a canonical counted name. */
extern "C" FVM_Result fvm_object_set_property_flags_bytes(FVM_Context *c, FVM_Object *o, const char *name, size_t size,
                                                    unsigned int set, unsigned int clear)
{
    if (!c || !o || o->owner != c || !name || (set | clear) > 7)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::string key;
        FVM_Result normalized = fvm_property_key(name, size, key);
        if (normalized != FVM_OK)
            return normalized;
        unsigned int old = 0;
        FVM_Result r = fvm_object_property_flags_bytes(c, o, key.data(), key.size(), &old);
        if (r != FVM_OK)
            return r;
        unsigned int flags = (old | set) & ~clear;
        if (flags)
            o->attributes[key] = flags;
        else
            o->attributes.erase(key);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
/* Delete object. */
extern "C" FVM_Result fvm_object_delete(FVM_Context *c, FVM_Object *o, const char *name,
                                        int *deleted)
{
    return name ? fvm_object_delete_bytes(c, o, name, std::strlen(name), deleted)
                : FVM_INVALID_ARGUMENT;
}
/* Apply the same attribute/deletion rules to a canonical counted name. */
extern "C" FVM_Result fvm_object_delete_bytes(FVM_Context *c, FVM_Object *o, const char *name, size_t size,
                                        int *deleted)
{
    if (!c || !o || o->owner != c || !name || !deleted)
        return FVM_INVALID_ARGUMENT;
    *deleted = 0;
    try
    {
        fvm_detail::string key;
        FVM_Result normalized = fvm_property_key(name, size, key);
        if (normalized != FVM_OK)
            return normalized;
        unsigned int flags = 0;
        FVM_Result r = fvm_object_property_flags_bytes(c, o, key.data(), key.size(), &flags);
        if (r != FVM_OK)
            return r;
        if (flags & 2)
            return FVM_OK;
        unsigned long index;
        if ((o->resource_kind == 28 || o->resource_kind == 30) &&
                fvm_property_terminated(key) && fvm_as_index(key.c_str(), &index))
            o->indexed.erase(index);
        /* Canonical numeric accessors/methods are own properties too. */
        /* [GC:] Deleting a writable/deletable raw prototype slot removes its inheritance edge. */
        if (c->swf_loaded && key == fvm_detail::string("__proto__") &&
            o->properties.find(key) != o->properties.end())
            o->prototype = 0;
        o->properties.erase(key);
        o->methods.erase(key);
        o->accessors.erase(key);
        o->attributes.erase(key);
        *deleted = 1;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
