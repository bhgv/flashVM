/* FlashVM: generic SWF6--8 AsBroadcaster subset with rooted snapshot delivery. */
#include "broadcaster.hpp"
#include "broadcaster_constants.h"
#include "base_state.hpp"
#include "../core/keyboard_state.hpp"
#include "../core/property_lookup.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/value_vector_root_scope.hpp"
#include "../core/optional_method.hpp"
#include "../core/execution_work_scope.hpp"
#include "../avm1/coercion.hpp"
#include "flashvm/broadcaster.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include <new>

namespace
{
/* Guard lifecycle separately from the script version policy. */
FVM_Result usable(FVM_Context *c)
{
    if (!fvm_keyboard_context_usable(c))
        return FVM_INVALID_ARGUMENT;
    return c->swf_loaded && c->swf_info.version < FVM_BROADCASTER_MIN_SWF_VERSION
        ? FVM_UNSUPPORTED : FVM_OK;
}
/* [GC:] The accessor root owns the live Array independently of ordinary references. */
FVM_Result listeners(FVM_Context *c, FVM_Object *, unsigned int, const FVM_Value *,
                     FVM_Value *out, void *data)
{
    FVM_Result status = usable(c);
    FVM_Object *array = static_cast<FVM_Object *>(data);
    if (status != FVM_OK)
        return status;
    if (!array || array->owner != c || array->resource_kind != FVM_RESOURCE_ARRAY)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = array;
    return FVM_OK;
}
/* Read one native live list and bound its retained storage length before traversal. */
FVM_Result list(FVM_Context *c, FVM_Object *self, FVM_Object **out, unsigned long &length)
{
    FVM_Result status = usable(c);
    if (status != FVM_OK)
        return status;
    FVM_Value value;
    status = fvm_object_get(c, self, "_listeners", &value);
    if (status != FVM_OK)
        return status;
    if (value.type != FVM_VALUE_OBJECT || !value.object ||
        value.object->resource_kind != FVM_RESOURCE_ARRAY ||
        !value.object->resource_result_valid || value.object->resource_result.type != FVM_VALUE_NUMBER)
        return FVM_INVALID_ARGUMENT;
    double size = value.object->resource_result.number;
    if (size != size || size < 0 || size > FVM_BROADCASTER_MAX_LISTENERS || size != static_cast<unsigned long>(size))
        return FVM_ERROR;
    *out = value.object;
    length = static_cast<unsigned long>(size);
    return FVM_OK;
}
/* Store listener identity once; duplicate adds succeed without moving registration order. */
FVM_Result add(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args,
               FVM_Value *out, void *)
{
    fvm_value_boolean(out, 1);
    FVM_Object *array = 0;
    unsigned long length = 0;
    FVM_Result status = list(c, self, &array, length);
    if (status != FVM_OK)
        return status;
    if (!count || args[0].type != FVM_VALUE_OBJECT || !args[0].object)
        return FVM_OK;
    /* [GC:] The rooted method dispatcher retains incoming arguments; retain list across allocation. */
    FVM_ObjectRootScope root(c, array);
    for (unsigned long i = 0; i < length; ++i)
    {
        fvm_detail::map<unsigned long, FVM_Value>::iterator entry = array->indexed.find(i);
        if (entry != array->indexed.end() && entry->second.type == FVM_VALUE_OBJECT &&
            entry->second.object == args[0].object)
            return FVM_OK;
    }
    if (length >= FVM_BROADCASTER_MAX_LISTENERS)
        return FVM_ERROR;
    /* Prepare the only allocating insertion before changing logical length. */
    fvm_detail::map<unsigned long, FVM_Value> next = array->indexed;
    next[length] = args[0];
    array->indexed.swap(next);
    fvm_value_number(&array->resource_result, static_cast<double>(length + 1));
    return FVM_OK;
}
/* Remove the first matching object while retaining a compact ordinary live list. */
FVM_Result remove(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args,
                  FVM_Value *out, void *)
{
    fvm_value_boolean(out, 0);
    FVM_Object *array = 0;
    unsigned long length = 0;
    FVM_Result status = list(c, self, &array, length);
    if (status != FVM_OK)
        return status;
    if (!count || args[0].type != FVM_VALUE_OBJECT)
        return FVM_OK;
    /* [GC:] Keep list and its remaining listener graphs alive throughout staging. */
    FVM_ObjectRootScope root(c, array);
    for (unsigned long i = 0; i < length; ++i)
    {
        fvm_detail::map<unsigned long, FVM_Value>::iterator entry = array->indexed.find(i);
        if (entry == array->indexed.end() || entry->second.type != FVM_VALUE_OBJECT ||
            entry->second.object != args[0].object)
            continue;
        fvm_detail::map<unsigned long, FVM_Value> next;
        for (fvm_detail::map<unsigned long, FVM_Value>::iterator slot = array->indexed.begin();
             slot != array->indexed.end(); ++slot)
            if (slot->first != i)
                next[slot->first > i && slot->first < length ? slot->first - 1 : slot->first] = slot->second;
        array->indexed.swap(next);
        fvm_value_number(&array->resource_result, static_cast<double>(length - 1));
        fvm_value_boolean(out, 1);
        break;
    }
    return FVM_OK;
}
/* [GC:] Snapshot recipients after event-name coercion; mutations affect later broadcasts only. */
FVM_Result broadcast(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args,
                     FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Result status = usable(c);
    if (status != FVM_OK || !count)
        return status;
    FVM_Value name;
    status = fvm_avm1_string(c, args[0], &name);
    if (status != FVM_OK)
        return status;
    /* Retain a counted spelling, including embedded NUL, before any other script runs. */
    fvm_detail::string key(name.string, fvm_value_string_size(&name));
    FVM_Object *array = 0;
    unsigned long length = 0;
    status = list(c, self, &array, length);
    if (status != FVM_OK)
        return status;
    FVM_ObjectRootScope root(c, array);
    fvm_detail::vector<FVM_Value> recipients;
    /* [GC:] The vector object stays linked while push_back changes its owned element buffer. */
    FVM_ValueVectorRootScope roots(c, recipients);
    FVM_ExecutionWorkScope work(c);
    recipients.reserve(length);
    for (unsigned long i = 0; i < length; ++i)
    {
        status = work.step();
        if (status != FVM_OK)
            return status;
        fvm_detail::map<unsigned long, FVM_Value>::iterator item = array->indexed.find(i);
        if (item != array->indexed.end() && item->second.type == FVM_VALUE_OBJECT && item->second.object)
            recipients.push_back(item->second);
    }
    for (size_t i = 0; i < recipients.size(); ++i)
    {
        status = work.step();
        if (status != FVM_OK)
            return status;
        status = fvm_object_call_optional_bytes(c, recipients[i].object, key.data(), key.size(),
                                                count - 1, count > 1 ? args + 1 : 0);
        if (status != FVM_OK)
            return status;
    }
    return FVM_OK;
}
/* Static entries are shared, but their receiver's live Array supplies all persistent state. */
struct Method
{
    const char *name; /* Static ActionScript spelling. */
    FVM_NativeFunction callback; /* Trusted rooted C++98 bridge. */
};
const Method instance_methods[] = {{"addListener", add}, {"removeListener", remove},
                                    {"broadcastMessage", broadcast}};
/* Recognize our exact descriptors without running getters or allocating wrappers. */
bool intact(FVM_Context *c, FVM_Object *o)
{
    for (size_t i = 0; i < sizeof(instance_methods) / sizeof(instance_methods[0]); ++i)
    {
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator entry =
            fvm_property_find(c, o->methods, fvm_detail::string(instance_methods[i].name));
        if (entry == o->methods.end() || entry->second.callback != instance_methods[i].callback ||
            !entry->second.collector_entry || entry->second.userdata || entry->second.root ||
            fvm_property_find(c, o->properties, fvm_detail::string(instance_methods[i].name)) != o->properties.end() ||
            fvm_property_find(c, o->accessors, fvm_detail::string(instance_methods[i].name)) != o->accessors.end())
            return false;
    }
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator entry =
        fvm_property_find(c, o->accessors, fvm_detail::string("_listeners"));
    return entry != o->accessors.end() && entry->second.getter == listeners &&
        !entry->second.setter && entry->second.root && entry->second.userdata == entry->second.root &&
        fvm_property_find(c, o->properties, fvm_detail::string("_listeners")) == o->properties.end() &&
        fvm_property_find(c, o->methods, fvm_detail::string("_listeners")) == o->methods.end();
}
/* Ordinary slots are rejected rather than silently overwriting application-owned state. */
bool occupied(FVM_Context *c, FVM_Object *o, const char *name)
{
    const fvm_detail::string key(name);
    return fvm_property_find(c, o->properties, key) != o->properties.end() ||
        fvm_property_find(c, o->methods, key) != o->methods.end() ||
        fvm_property_find(c, o->accessors, key) != o->accessors.end();
}
/* The namespace script method delegates to the same transactional public initializer. */
FVM_Result initialize(FVM_Context *c, FVM_Object *, unsigned int count, const FVM_Value *args,
                      FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Result status = usable(c);
    if (status != FVM_OK || !count)
        return status;
    if (args[0].type != FVM_VALUE_OBJECT || !args[0].object)
        return FVM_OK;
    return fvm_broadcaster_initialize(c, args[0].object);
}
} // namespace

/* [GC:] Stage every allocating change on a rooted temporary object, then swap without callbacks. */
extern "C" FVM_Result fvm_broadcaster_initialize(FVM_Context *c, FVM_Object *object)
{
    FVM_Result status = usable(c);
    if (status != FVM_OK)
        return status;
    if (!object || object->owner != c)
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Global Object/Array validation can invoke a host-installed getter. */
        FVM_ObjectRootScope receiver_root(c, object);
        status = fvm_base_validate(c);
        if (status != FVM_OK)
            return status;
        bool conflict = occupied(c, object, "_listeners");
        for (size_t i = 0; i < sizeof(instance_methods) / sizeof(instance_methods[0]); ++i)
            conflict = conflict || occupied(c, object, instance_methods[i].name);
        if (conflict && !intact(c, object))
            return FVM_INVALID_ARGUMENT;
        FVM_Object *array = 0, *staged = 0;
        status = fvm_array_create_like(c, 0, 0, &array, true);
        if (status != FVM_OK)
            return status;
        FVM_ObjectRootScope array_root(c, array);
        status = fvm_object_create(c, &staged);
        if (status != FVM_OK)
            return status;
        FVM_ObjectRootScope staging_root(c, staged);
        staged->properties = object->properties;
        staged->methods = object->methods;
        staged->accessors = object->accessors;
        staged->attributes = object->attributes;
        staged->method_values = object->method_values;
        FVM_AccessorEntry entry;
        entry.getter = listeners;
        entry.root = array;
        entry.userdata = array;
        staged->accessors["_listeners"] = entry;
        staged->attributes["_listeners"] = FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE | FVM_PROPERTY_READ_ONLY;
        for (size_t i = 0; i < sizeof(instance_methods) / sizeof(instance_methods[0]); ++i)
        {
            FVM_NativeEntry method;
            method.callback = instance_methods[i].callback;
            /* [GC:] The rooted dispatcher retains receiver/arguments through recursive script/GC. */
            method.collector_entry = true;
            staged->methods[instance_methods[i].name] = method;
            staged->attributes[instance_methods[i].name] = FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE;
            staged->method_values.erase(fvm_detail::string(instance_methods[i].name));
        }
        object->properties.swap(staged->properties);
        object->methods.swap(staged->methods);
        object->accessors.swap(staged->accessors);
        object->attributes.swap(staged->attributes);
        object->method_values.swap(staged->method_values);
        return FVM_OK;
    }
    catch (const std::bad_alloc &) { return FVM_OUT_OF_MEMORY; }
    catch (...) { return FVM_ERROR; }
}
/* Publish an opt-in namespace transactionally; repeated intact installation is idempotent. */
extern "C" FVM_Result fvm_standard_broadcaster_install(FVM_Context *c)
{
    FVM_Result status = usable(c);
    if (status != FVM_OK)
        return status;
    try
    {
        status = fvm_base_validate(c);
        if (status != FVM_OK)
            return status;
        if (occupied(c, c->global, "AsBroadcaster") ||
            fvm_property_find(c, c->functions, fvm_detail::string("AsBroadcaster")) != c->functions.end() ||
            fvm_property_find(c, c->constructors, fvm_detail::string("AsBroadcaster")) != c->constructors.end() ||
            fvm_property_find(c, c->stack_functions, fvm_detail::string("AsBroadcaster")) != c->stack_functions.end() ||
            fvm_property_find(c, c->script_functions, fvm_detail::string("AsBroadcaster")) != c->script_functions.end())
        {
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator found =
                fvm_property_find(c, c->global->properties, fvm_detail::string("AsBroadcaster"));
            if (found == c->global->properties.end() || found->second.type != FVM_VALUE_OBJECT ||
                !found->second.object || found->second.object->owner != c ||
                fvm_property_find(c, c->global->methods, fvm_detail::string("AsBroadcaster")) != c->global->methods.end() ||
                fvm_property_find(c, c->global->accessors, fvm_detail::string("AsBroadcaster")) != c->global->accessors.end() ||
                fvm_property_find(c, c->functions, fvm_detail::string("AsBroadcaster")) != c->functions.end() ||
                fvm_property_find(c, c->constructors, fvm_detail::string("AsBroadcaster")) != c->constructors.end() ||
                fvm_property_find(c, c->stack_functions, fvm_detail::string("AsBroadcaster")) != c->stack_functions.end() ||
                fvm_property_find(c, c->script_functions, fvm_detail::string("AsBroadcaster")) != c->script_functions.end())
                return FVM_INVALID_ARGUMENT;
            FVM_Object *existing = found->second.object;
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method =
                fvm_property_find(c, existing->methods, fvm_detail::string("initialize"));
            return method != existing->methods.end() && method->second.callback == initialize &&
                method->second.collector_entry && !method->second.root && !method->second.userdata &&
                !occupied(c, existing, "prototype") &&
                fvm_property_find(c, existing->properties, fvm_detail::string("initialize")) == existing->properties.end() &&
                fvm_property_find(c, existing->accessors, fvm_detail::string("initialize")) == existing->accessors.end()
                ? FVM_OK : FVM_INVALID_ARGUMENT;
        }
        FVM_Object *object = 0;
        status = fvm_object_create(c, &object);
        if (status != FVM_OK)
            return status;
        FVM_ObjectRootScope root(c, object);
        FVM_NativeEntry method;
        method.callback = initialize;
        /* [GC:] Script initialization arguments stay rooted during allocation and publication. */
        method.collector_entry = true;
        object->methods["initialize"] = method;
        object->attributes["initialize"] = FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE;
        fvm_detail::map<fvm_detail::string, FVM_Value> globals = c->global->properties;
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = object;
        globals["AsBroadcaster"] = value;
        c->global->properties.swap(globals);
        return FVM_OK;
    }
    catch (const std::bad_alloc &) { return FVM_OUT_OF_MEMORY; }
    catch (...) { return FVM_ERROR; }
}

/* Map code identities without retaining process addresses in graph snapshots. */
unsigned long fvm_broadcaster_callback_id(FVM_NativeFunction callback)
{
    if (callback == initialize) return FVM_BROADCASTER_CALLBACK_INITIALIZE;
    if (callback == listeners) return FVM_BROADCASTER_CALLBACK_LISTENERS;
    if (callback == add) return FVM_BROADCASTER_CALLBACK_ADD;
    if (callback == remove) return FVM_BROADCASTER_CALLBACK_REMOVE;
    if (callback == broadcast) return FVM_BROADCASTER_CALLBACK_BROADCAST;
    return 0;
}
/* Restore only known callbacks; the graph decoder separately restores traced capture roots. */
FVM_NativeFunction fvm_broadcaster_callback_from_id(unsigned long id)
{
    switch (id)
    {
    case FVM_BROADCASTER_CALLBACK_INITIALIZE: return initialize;
    case FVM_BROADCASTER_CALLBACK_LISTENERS: return listeners;
    case FVM_BROADCASTER_CALLBACK_ADD: return add;
    case FVM_BROADCASTER_CALLBACK_REMOVE: return remove;
    case FVM_BROADCASTER_CALLBACK_BROADCAST: return broadcast;
    default: return 0;
    }
}
