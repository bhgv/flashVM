/* FlashVM: C embedding API: property.
 * File: src/capi/property_c.cpp.
 */

#include "../core/runtime.hpp"
#include "../core/prototype_data.hpp"
#include "../standard/base_state.hpp"
#include "flashvm/property.h"
#include <new>
#include "../core/property_key.hpp"
#include "../core/property_lookup.hpp"


/* Compute and return object property flags. */
extern "C" FVM_Result fvm_object_property_flags(FVM_Context *c, FVM_Object *o, const char *name, unsigned int *out)
{
    return name ? fvm_object_property_flags_bytes(c, o, name, std::strlen(name), out) : FVM_INVALID_ARGUMENT;
}


/* Apply the same attribute/deletion rules to a canonical counted name. */
extern "C" FVM_Result
fvm_object_property_flags_bytes(FVM_Context *c, FVM_Object *o, const char *name, size_t size, unsigned int *out)
{
    if (!c || !o || o->owner != c || !name || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {
        fvm_detail::string key;
        FVM_Result normalized = fvm_property_key(name, size, key);
        if (normalized != FVM_OK)
        {
            return normalized;
        }
        fvm_detail::map<fvm_detail::string, unsigned int>::iterator i = fvm_property_find(c, o->attributes, key);
        *out = i == o->attributes.end() ? 0 : i->second;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}


/* Update object property flags. */
extern "C" FVM_Result
fvm_object_set_property_flags(FVM_Context *c, FVM_Object *o, const char *name, unsigned int set, unsigned int clear)
{
    return name ? fvm_object_set_property_flags_bytes(c, o, name, std::strlen(name), set, clear) : FVM_INVALID_ARGUMENT;
}


/* Apply the same attribute/deletion rules to a canonical counted name. */
extern "C" FVM_Result fvm_object_set_property_flags_bytes(
    FVM_Context *c, FVM_Object *o, const char *name, size_t size, unsigned int set, unsigned int clear)
{
    if (!c || !o || o->owner != c || !name || ((set | clear) & ~FVM_PROPERTY_VALID_FLAGS))
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {
        fvm_detail::string key;
        FVM_Result normalized = fvm_property_key(name, size, key);
        if (normalized != FVM_OK)
        {
            return normalized;
        }
        /* Resolve an own spelling before mutating flags or erasing its tables. */
        if (!fvm_property_select_own_name(c, o, key))
        {
            fvm_detail::map<fvm_detail::string, unsigned int>::iterator attribute =
                fvm_property_find(c, o->attributes, key);
            if (attribute != o->attributes.end())
            {
                key = attribute->first;
            }
        }
        unsigned int old = 0;
        FVM_Result r = fvm_object_property_flags_bytes(c, o, key.data(), key.size(), &old);
        if (r != FVM_OK)
        {
            return r;
        }
        unsigned int flags = (old | set) & ~clear;
        if (flags)
        {
            o->attributes[key] = flags;
        }
        else
        {
            o->attributes.erase(key);
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}


/* Delete object. */
extern "C" FVM_Result fvm_object_delete(FVM_Context *c, FVM_Object *o, const char *name, int *deleted)
{
    return name ? fvm_object_delete_bytes(c, o, name, std::strlen(name), deleted) : FVM_INVALID_ARGUMENT;
}


/* Apply the same attribute/deletion rules to a canonical counted name. */
extern "C" FVM_Result
fvm_object_delete_bytes(FVM_Context *c, FVM_Object *o, const char *name, size_t size, int *deleted)
{
    if (!c || !o || o->owner != c || !name || !deleted)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *deleted = 0;
    try
    {
        fvm_detail::string key;
        FVM_Result normalized = fvm_property_key(name, size, key);
        if (normalized != FVM_OK)
        {
            return normalized;
        }
        /* Resolve an own spelling before mutating flags or erasing its tables. */
        if (!fvm_property_select_own_name(c, o, key))
        {
            fvm_detail::map<fvm_detail::string, unsigned int>::iterator attribute =
                fvm_property_find(c, o->attributes, key);
            if (attribute != o->attributes.end())
            {
                key = attribute->first;
            }
        }
        unsigned int flags = 0;
        FVM_Result r = fvm_object_property_flags_bytes(c, o, key.data(), key.size(), &flags);
        if (r != FVM_OK)
        {
            return r;
        }
        if (flags & FVM_PROPERTY_DONT_DELETE)
        {
            return FVM_OK;
        }
        bool clear_native_length = false;
        if (o->resource_kind == FVM_RESOURCE_ARRAY)
        {
            fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator length =
                fvm_property_find(c, o->accessors, fvm_detail::string("length"));
            clear_native_length =
                length != o->accessors.end() && length->first == key && fvm_base_array_length_accessor(length->second);
        }
        unsigned long index;
        if ((o->resource_kind == FVM_RESOURCE_ARRAY || o->resource_kind == FVM_RESOURCE_OBJECT) &&
            fvm_property_terminated(key) && fvm_as_index(key.c_str(), &index))
        {
            o->indexed.erase(index);
        }
        /* Canonical numeric accessors/methods are own properties too. */
        /* [GC:] Deleting a writable/deletable raw prototype slot removes its inheritance edge. */
        if (c->swf_loaded && fvm_object_prototype_name(c, key) &&
            (o->properties.find(key) != o->properties.end() || fvm_object_intrinsic_prototype(c, o, &key)))
        {
            o->prototype = 0;
        }
        o->properties.erase(key);
        o->methods.erase(key);
        /* [GC:] Deleted methods must not retain captures through their lazy wrapper cache. */
        o->method_values.erase(key);
        if (clear_native_length)
        { /* [GC:] A deleted native length must not retain its obsolete object/metatable graph. */
            fvm_value_number(&o->resource_result, 0);
        }
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
