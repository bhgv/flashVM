/* FlashVM: C embedding API: table.
 * File: src/capi/table_c.cpp.
 */

#include "../core/string_internal.hpp"
#include "table_constants.h"
#include "table_index_binding.hpp"
#include "variable_refs.hpp"
#include "../standard/base_state.hpp"
#include "../standard/super.hpp"
#include "../core/property_watch.hpp"
#include "../core/property_lookup.hpp"
#include "../core/property_write.hpp"
#include "../core/watch_write_scope.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/value_vector_root_scope.hpp"
#include "flashvm/property.h"
#include <cstdio>
#include "../core/runtime.hpp"
#include "flashvm/table.h"
#include "flashvm/debug.h"
#include <new>
#include <cstring>


/* Compute and return valid. */
static int valid(FVM_Context *c, FVM_Object *o)
{
    return c && o && o->owner == c;
}


/* Compute and return store. */
static FVM_Result store(FVM_Context *c, const FVM_Value *v, FVM_Value *out)
{
    if (fvm_validate_value(c, v) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = *v;
    if (v->type == FVM_VALUE_STRING)
    {
        size_t n = fvm_string_size(*v);
        char *p = new (std::nothrow) char[n + 1];
        if (!p)
        {
            return FVM_OUT_OF_MEMORY;
        }
        std::memcpy(p, v->string, n);
        p[n] = 0;
        try
        {
            /* [GC:] Register owned bytes and their counted length together; failed insertion leaves
             * caller cleanup active. */
            fvm_context_adopt_string(c, p, n);
        }
        catch (...)
        {
            delete[] p;
            return FVM_OUT_OF_MEMORY;
        }
        out->string = p;
    }
    return FVM_OK;
}

extern "C"
{


/* [GC:] EN: Bind a new native numeric slot to ref. On failure leave the destination absent.
 * RU: Привязать новый числовой слот к ref. При ошибке слот остаётся отсутствующим. */
static FVM_Result bind_index(FVM_Context *c, FVM_Object *owner, unsigned long key, const FVM_Value *ref)
{
    if (owner->indexed.find(key) != owner->indexed.end())
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_ObjectRootScope keep_ref(c, ref->object);
    FVM_Object *holder = NULL;
    FVM_Result status = fvm_object_create(c, &holder);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_ObjectRootScope keep_holder(c, holder);
    status = fvm_bind_variable_ref(c, holder, FVM_TABLE_INDEX_VALUE, sizeof(FVM_TABLE_INDEX_VALUE) - 1, ref);
    if (status == FVM_OK)
    {
        FVM_Value binding;
        fvm_value_undefined(&binding);
        binding.type = FVM_VALUE_OBJECT;
        binding.object = holder;
        holder->resource_kind = FVM_RESOURCE_TABLE_INDEX_BINDING;
        owner->indexed[key] = binding;
    }
    return status;
}


/* Update index. */
static FVM_Result set_index(FVM_Context *c, FVM_Object *o, unsigned long key, const FVM_Value *v)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value x;
    FVM_Result r;
    FVM_Value watched;
    bool prepared_watch = false;
    if (!valid(c, o) || fvm_validate_value(c, v) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }


    /* [GC:] Length conversion can detach the receiver and watched value before storage. */
    FVM_ObjectRootScope receiver_roots(c, o);
    if (o->resource_kind == FVM_RESOURCE_PLAIN_OBJECT && o != c->global && fvm_is_variable_ref(v))
    {
        return bind_index(c, o, key, v);
    }
    fvm_detail::vector<FVM_Value> retained;


    FVM_ValueVectorRootScope value_roots(c, retained);


    FVM_WatchWriteScope write(c);
    if (o->resource_kind == FVM_RESOURCE_ARRAY || o->resource_kind == FVM_RESOURCE_OBJECT)
    {
        if (key > 0xffffffffUL)
        {
            return FVM_INVALID_ARGUMENT;
        }
        char name[FVM_TABLE_INDEX_KEY_BYTES];
        std::sprintf(name, "%lu", key);
        if (key == 0xffffffffUL)
        {
            return fvm_object_set(c, o, name, v);
        }
        if (c->swf_loaded)
        {
            r = write.defer(fvm_property_watch_prepare(c, o, name, v, &watched), watched);
            if (r != FVM_OK)
            {
                return r;
            }
            prepared_watch = true;
        }
        if (c->swf_loaded && o->resource_kind == FVM_RESOURCE_ARRAY)
        {
            retained.push_back(watched);
        }
        /* Loaded growth can truncate on Int32 overflow and must propagate errors. */
        r = fvm_array_prepare_index(c, o, fvm_detail::string(name));
        if (r != FVM_OK)
        {
            return r;
        }
        unsigned int flags = 0;
        FVM_Result status = fvm_object_property_flags(c, o, name, &flags);
        if (status != FVM_OK)
        {
            return status;
        }
        /* Loaded script setters run even when their stored indexed data is readonly. */
        if (flags & 4)
        {
            fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator script = o->accessors.find(name);
            if (!c->swf_loaded || script == o->accessors.end() || !fvm_base_script_accessor(script->second))
            {
                return write.finish(FVM_OK);
            }
        }
        if (!c->swf_loaded && o->resource_kind == FVM_RESOURCE_ARRAY &&
            (o->resource_result.type != FVM_VALUE_NUMBER || key >= o->resource_result.number))
        {
            status = fvm_object_property_flags(c, o, "length", &flags);
            if (status != FVM_OK)
            {
                return status;
            }
            if (flags & 4)
            {
                return write.finish(FVM_OK);
            }
        }
    }
    if (o->resource_kind == FVM_RESOURCE_ARRAY || o->resource_kind == FVM_RESOURCE_OBJECT)
    {
        char name[FVM_TABLE_INDEX_KEY_BYTES];
        std::sprintf(name, "%lu", key);
        FVM_Object *cursor = o;
        unsigned int depth = 0;
        while (cursor && depth++ < FVM_PROTOTYPE_MAX_DEPTH)
        {
            if (cursor != o && !fvm_property_available(c, cursor, name))
            {
                cursor = cursor->prototype;
                continue;
            }
            /* Only own data blocks loaded virtual dispatch; inherited data is skipped. */
            if (c->swf_loaded ? cursor == o && fvm_property_own_write_data(c, cursor, fvm_detail::string(name))
                              : cursor->indexed.find(key) != cursor->indexed.end() ||
                                    cursor->properties.find(name) != cursor->properties.end())
            {
                break;
            }
            if (cursor->accessors.find(name) != cursor->accessors.end())
            {
                return write.finish(prepared_watch ? fvm_object_set_from(c, o, o, name, &watched, true, true)
                                                   : fvm_object_set_no_index(c, o, name, v),
                                    cursor != o);
            }
            cursor = cursor->prototype;
        }
        if (cursor && depth > FVM_PROTOTYPE_MAX_DEPTH)
        {
            return FVM_ERROR;
        }
    }
    char watched_name[FVM_TABLE_INDEX_KEY_BYTES];
    std::sprintf(watched_name, "%lu", key);
    if (!prepared_watch)
    {
        r = fvm_property_watch_prepare(c, o, watched_name, v, &watched);
        if (r != FVM_OK)
        {
            return r;
        }
    }
    fvm_detail::map<unsigned long, FVM_Value>::const_iterator current = o->indexed.find(key);
    if (current != o->indexed.end())
    {
        FVM_Object *holder = fvm_table_index_binding(current->second);
        if (holder)
        {
            FVM_ObjectRootScope keep_holder(c, holder);
            return write.finish(fvm_object_set(c, holder, FVM_TABLE_INDEX_VALUE, &watched));
        }
    }
    r = store(c, &watched, &x);
    if (r != FVM_OK)
    {
        return r;
    }
    try
    {
        o->indexed[key] = x;
        fvm_property_clear_versions(c, o, watched_name);
        fvm_array_note_index(o, key);
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
    return write.finish(FVM_OK);
}


/* Read index. */
static FVM_Result get_index(FVM_Context *c, FVM_Object *o, unsigned long key, FVM_Value *out)
{
    if (!valid(c, o) || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *receiver = o;
    bool standard_indices = o->resource_kind == FVM_RESOURCE_ARRAY || o->resource_kind == FVM_RESOURCE_OBJECT;
    if (standard_indices)
    {
        char name[FVM_TABLE_INDEX_KEY_BYTES];
        std::sprintf(name, "%lu", key);
        if (key > 0xffffffffUL)
        {
            return FVM_INVALID_ARGUMENT;
        }
        if (key == 0xffffffffUL)
        {
            char name[FVM_TABLE_INDEX_KEY_BYTES];
            std::sprintf(name, "%lu", key);
            return fvm_object_get(c, o, name, out);
        }
        FVM_Object *cursor = o;
        unsigned int depth = 0;
        while (cursor && depth++ < FVM_PROTOTYPE_MAX_DEPTH)
        {
            if (!fvm_property_available(c, cursor, name))
            {
                cursor = cursor->prototype;
                continue;
            }
            fvm_detail::map<unsigned long, FVM_Value>::iterator i = cursor->indexed.find(key);
            if (i != cursor->indexed.end())
            {
                return fvm_table_read_entry(c, receiver, i->second, out);
            }
            if (cursor->properties.find(name) != cursor->properties.end() ||
                cursor->accessors.find(name) != cursor->accessors.end() ||
                cursor->methods.find(name) != cursor->methods.end())
            {
                return fvm_object_get_no_index(c, receiver, name, out);
            }
            cursor = cursor->prototype;
        }
        if (cursor)
        {
            return FVM_ERROR;
        }
    }
    for (unsigned int depth = 0; o && depth < FVM_PROTOTYPE_MAX_DEPTH; ++depth)
    {
        if (standard_indices)
        {
            char name[FVM_TABLE_INDEX_KEY_BYTES];
            std::sprintf(name, "%lu", key);
            if (!fvm_property_available(c, o, name))
            {
                o = o->metatable;
                continue;
            }
        }
        fvm_detail::map<unsigned long, FVM_Value>::const_iterator it = o->indexed.find(key);
        if (it != o->indexed.end())
        {
            return fvm_table_read_entry(c, receiver, it->second, out);
        }
        o = o->metatable;
    }
    if (standard_indices)
    {
        char name[FVM_TABLE_INDEX_KEY_BYTES];
        std::sprintf(name, "%lu", key);
        return fvm_object_get_no_index(c, receiver, name, out);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* Update table index. */
FVM_Result fvm_table_set_index(FVM_Context *c, FVM_Object *o, unsigned long key, const FVM_Value *v)
{
    try
    {
        return set_index(c, o, key, v);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}


/* Read table index. */
FVM_Result fvm_table_get_index(FVM_Context *c, FVM_Object *o, unsigned long key, FVM_Value *out)
{
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_undefined(out);
    try
    {
        return get_index(c, o, key, out);
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


/* Update table metatable. */
FVM_Result fvm_table_set_metatable(FVM_Context *c, FVM_Object *o, FVM_Object *meta)
{
    if (!valid(c, o) || (meta && !valid(c, meta)))
    {
        return FVM_INVALID_ARGUMENT;
    }

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *cursor = meta;
    unsigned int depth = 0;
    while (cursor && depth++ < FVM_PROTOTYPE_MAX_DEPTH)
    {
        if (cursor == o)
        {
            return FVM_INVALID_ARGUMENT;
        }
        cursor = cursor->metatable;
    }
    if (cursor)
    {
        return FVM_INVALID_ARGUMENT;
    }
    o->metatable = meta;
    return FVM_OK;
}


/* Read table metatable. */
FVM_Result fvm_table_get_metatable(FVM_Context *c, FVM_Object *o, FVM_Object **out)
{
    if (!valid(c, o) || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = o->metatable;
    return FVM_OK;
}


#include "table_inspect.inc"
}
