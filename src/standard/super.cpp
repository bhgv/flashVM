/* FlashVM: retained superclass facades with receiver-preserving property/method dispatch. */
#include "super.hpp"
#include "../core/property_key.hpp"
#include "inheritance.hpp"
#include "base_state.hpp"
#include "../core/call_scope.hpp"
#include "flashvm/function.h"
#include <new>

namespace
{
/* Read a private metadata edge without invoking user-controlled accessors. */
FVM_Result edge(FVM_Object *data, const char *key, FVM_Object **out)
{
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator it = data->properties.find(key);
    if (it == data->properties.end())
        return FVM_INVALID_ARGUMENT;
    if (it->second.type == FVM_VALUE_UNDEFINED)
    {
        *out = 0;
        return FVM_OK;
    }
    if (it->second.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    *out = it->second.object;
    return FVM_OK;
}
/* Resolve the live owner at the captured prototype depth and its parent chain. */
FVM_Result unpack(FVM_Context *c, FVM_Object *facade, FVM_Object **receiver, FVM_Object **owner,
                  FVM_Object **start)
{
    if (!facade || facade->owner != c || facade->resource_kind != 37 ||
        !facade->resource_result_valid || facade->resource_result.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = facade->resource_result.object;
    FVM_Result result = edge(data, "receiver", receiver);
    if (result != FVM_OK || !*receiver)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator depth = data->properties.find("depth");
    if (depth == data->properties.end() || depth->second.type != FVM_VALUE_NUMBER ||
        !(depth->second.number >= 0 && depth->second.number <= 1024) ||
        depth->second.number != (unsigned int)depth->second.number)
        return FVM_INVALID_ARGUMENT;
    *owner = *receiver;
    for (unsigned int i = 0; i < (unsigned int)depth->second.number && *owner; ++i)
        *owner = (*owner)->prototype;
    *start = *owner ? (*owner)->prototype : 0;
    return FVM_OK;
}
/* Invoke the live parent's constructor against the existing receiver. */
FVM_Result invoke(FVM_Context *c, FVM_Object *, unsigned int count, const FVM_Value *args,
                  FVM_Value *out, void *userdata)
{
    FVM_Object *receiver = 0, *owner = 0, *start = 0;
    FVM_Result result = unpack(c, (FVM_Object *)userdata, &receiver, &owner, &start);
    if (result != FVM_OK)
        return result;
    fvm_value_undefined(out);
    if (!owner)
        return FVM_OK;
    FVM_Value constructor;
    result = fvm_object_get_from(c, owner, owner, "__constructor__", &constructor, false);
    if (result != FVM_OK)
        return result;
    if (constructor.type != FVM_VALUE_OBJECT || !fvm_is_class_constructor(constructor.object))
        return FVM_OK;
    /* [GC:] The selected parent prototype and constructor remain rooted across the call. */
    FVM_CallScope scope(c, constructor.object, constructor.object, start);
    return fvm_function_invoke(c, constructor.object, receiver, count, args, out);
}
/* Recognize the function in a private script accessor descriptor. */
bool accessor_contains(FVM_Object *descriptor, FVM_Object *function)
{
    if (!descriptor)
        return false;
    const char *names[] = {"get", "set"};
    for (size_t i = 0; i < 2; ++i)
    {
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator it =
            descriptor->properties.find(names[i]);
        if (it != descriptor->properties.end() && it->second.type == FVM_VALUE_OBJECT &&
            it->second.object == function)
            return true;
    }
    return false;
}
/* Locate the defining property owner for direct calls lacking an explicit call-site scope. */
FVM_Result method_owner(FVM_Object *receiver, FVM_Object *function, FVM_Object **out)
{
    for (unsigned int depth = 0; receiver && depth < 1024; ++depth)
    {
        for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator p =
                 receiver->properties.begin();
             p != receiver->properties.end(); ++p)
            if (p->second.type == FVM_VALUE_OBJECT && p->second.object == function)
            {
                *out = receiver;
                return FVM_OK;
            }
        for (fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator p =
                 receiver->accessors.begin();
             p != receiver->accessors.end(); ++p)
            /* [GC:] Inspect the traced accessor capture to identify its owning prototype. */
            if (accessor_contains(p->second.root, function))
            {
                *out = receiver;
                return FVM_OK;
            }
        receiver = receiver->prototype;
    }
    if (receiver)
        return FVM_ERROR;
    *out = 0;
    return FVM_OK;
}
/* Store an optional object edge in unpublished, private superclass metadata. */
void store(FVM_Object *data, const char *name, FVM_Object *object)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    if (object)
    {
        value.type = FVM_VALUE_OBJECT;
        value.object = object;
    }
    /* [GC:] The facade state traces these receiver/prototype/constructor references. */
    data->properties[name] = value;
}
} // namespace

/* Return the stable native callback implementing callable superclass facades. */
FVM_NativeFunction fvm_super_callback()
{
    return invoke;
}

/* Build an activation's superclass reference using its actual constructor or method owner. */
FVM_Result fvm_super_create(FVM_Context *c, FVM_Object *callee, FVM_Object *receiver,
                            FVM_Value *out)
{
    fvm_value_undefined(out);
    if (!callee || !receiver)
        return FVM_OK;
    try
    {
        FVM_Object *owner = 0;
        if (c->calling_constructor == callee)
            owner = c->calling_owner ? c->calling_owner : receiver->prototype;
        else if (c->calling_function == callee)
            owner = c->calling_owner;
        else
        {
            FVM_Result result = method_owner(receiver, callee, &owner);
            if (result != FVM_OK)
                return result;
        }
        if (!owner)
            return FVM_OK;
        unsigned int depth = 0;
        FVM_Object *cursor = receiver;
        while (cursor && cursor != owner && depth < 1024)
        {
            cursor = cursor->prototype;
            ++depth;
        }
        if (cursor != owner)
            return FVM_OK;
        FVM_Object *data = 0, *facade = 0;
        FVM_Result result = fvm_object_create(c, &data);
        if (result != FVM_OK)
            return result;
        /* Construction does not invoke user callbacks until every metadata edge is published. */
        data->prototype = 0;
        store(data, "receiver", receiver);
        FVM_Value level;
        fvm_value_number(&level, depth);
        data->properties["depth"] = level;
        result = fvm_object_create(c, &facade);
        if (result != FVM_OK)
            return result;
        facade->prototype = 0;
        facade->resource_kind = 37;
        facade->resource_result_valid = true;
        facade->resource_result.type = FVM_VALUE_OBJECT;
        /* [GC:] Retain all metadata through the facade's managed state edge. */
        facade->resource_result.object = data;
        facade->invocable.callback = invoke;
        facade->invocable.userdata = facade;
        /* [GC:] Retain the facade as the native callback's userdata capture. */
        facade->invocable.root =
            facade; /* [GC:] The callback capture is the same retained facade. */
        facade->invocable.collector_entry = true;
        out->type = FVM_VALUE_OBJECT;
        out->object = facade;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Read only the selected ancestor chain, preserving the original accessor receiver. */
FVM_Result fvm_super_get(FVM_Context *c, FVM_Object *facade, const fvm_detail::string &name, FVM_Value *out)
{
    FVM_Object *receiver = 0, *owner = 0, *start = 0;
    FVM_Result result = unpack(c, facade, &receiver, &owner, &start);
    if (result != FVM_OK)
        return result;
    if (!start)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    /* Loaded super reads the selected ancestor's intrinsic proto slot. User
     * data/accessors still dispatch normally with the original receiver. This
     * does not implement the still-open ordinary raw __proto__ property model. */
    if (c->swf_loaded && name == fvm_detail::string("__proto__"))
    {
        FVM_Object *cursor = start;
        for (unsigned int depth = 0; cursor && depth < 1024; ++depth)
        {
            if (cursor->properties.find(name) != cursor->properties.end() ||
                cursor->methods.find(name) != cursor->methods.end())
                break;
            fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator entry =
                cursor->accessors.find(name);
            if (entry != cursor->accessors.end())
            {
                if (fvm_base_prototype_accessor(entry->second))
                {
                    fvm_value_undefined(out);
                    if (start->prototype)
                    {
                        out->type = FVM_VALUE_OBJECT;
                        out->object = start->prototype;
                    }
                    return FVM_OK;
                }
                break;
            }
            cursor = cursor->prototype;
        }
    }
    return fvm_object_get_from(c, start, receiver, name, out, false);
}

/* Invoke a named parent member and record the prototype that actually supplied it. */
FVM_Result fvm_super_method(FVM_Context *c, FVM_Object *facade, const fvm_detail::string &name,
                            unsigned int count, const FVM_Value *args, FVM_Value *out)
{
    FVM_Object *receiver = 0, *current = 0, *start = 0;
    FVM_Result result = unpack(c, facade, &receiver, &current, &start);
    if (result != FVM_OK)
        return result;
    if (!start)
        return FVM_UNSUPPORTED;
    FVM_Object *owner = start;
    for (unsigned int depth = 0; owner && depth < 1024; ++depth)
    {
        if (owner->properties.find(name) != owner->properties.end() ||
            owner->accessors.find(name) != owner->accessors.end() ||
            owner->methods.find(name) != owner->methods.end())
            break;
        owner = owner->prototype;
    }
    if (!owner)
        return FVM_UNSUPPORTED;
    FVM_Value member;
    result = fvm_object_get_from(c, start, receiver, name, &member, false);
    if (result != FVM_OK)
        return result;
    if (member.type != FVM_VALUE_OBJECT)
        return FVM_UNSUPPORTED;
    /* [GC:] Root the selected owner and preserve it across recursive superclass calls. */
    FVM_CallScope scope(c, 0, member.object, owner);
    return fvm_function_invoke(c, member.object, receiver, count, args, out);
}

/* Assign a parent-visible property using the live ancestor setter and actual receiver. */
FVM_Result fvm_super_set(FVM_Context *c, FVM_Object *facade, const fvm_detail::string &name,
                         const FVM_Value *value)
{
    FVM_Object *receiver = 0, *owner = 0, *start = 0;
    FVM_Result result = unpack(c, facade, &receiver, &owner, &start);
    if (result != FVM_OK)
        return result;
    unsigned long index;
    if (fvm_property_terminated(name) && fvm_as_index(name.c_str(), &index))
        return FVM_UNSUPPORTED;
    return fvm_object_set_from(c, receiver, start, name, value, false);
}
