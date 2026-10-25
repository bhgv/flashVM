/* FlashVM: publish loaded constructor identities without invoking inherited setters or watchers. */
#include "constructor_identity.hpp"
#include "../avm1/handlers/class_prototype.hpp"
#include "constructor_identity_constants.h"
#include "../core/object_root_scope.hpp"
#include "../swf/format_constants.h"
#include "flashvm/property.h"
#include <new>

namespace
{


/* A forced stored property replaces the exact own accessor/method representation. */
void discard_virtual_slot(FVM_Object *receiver, const fvm_detail::string &name)
{
    receiver->accessors.erase(name);
    receiver->methods.erase(name);
    receiver->method_values.erase(name);
}
}


/* Stage the complete property/flag update before changing an observable receiver. */
FVM_Result
fvm_function_define_constructor(FVM_Context *c, FVM_Object *receiver, FVM_Object *callee, unsigned int movie_version)
{
    if (!c || !receiver || receiver->owner != c || !callee || callee->owner != c)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!movie_version)
    {
        if (!c->swf_loaded)
        {
            return FVM_OK;
        }
        movie_version = c->swf_info.version;
    }
    if (receiver->resource_kind == FVM_RESOURCE_SUPER)
    {
        return FVM_OK;
    }
    try
    {


        /* [GC:] Metadata links the receiver to the active constructor even if outer bindings vanish. */
        FVM_ObjectRootScope receiver_root(c, receiver);


        FVM_ObjectRootScope callee_root(c, callee);


        fvm_detail::string internal_name(FVM_CONSTRUCTOR_IDENTITY_INTERNAL_NAME);


        fvm_detail::string legacy_name(FVM_CONSTRUCTOR_IDENTITY_LEGACY_NAME);
        bool legacy = movie_version < FVM_SWF_VERSION_7;
        fvm_detail::map<fvm_detail::string, FVM_Value> properties = receiver->properties;
        fvm_detail::map<fvm_detail::string, unsigned int> attributes = receiver->attributes;
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = callee;
        properties[internal_name] = value;
        attributes[internal_name] = FVM_PROPERTY_DONT_ENUM;
        if (legacy)
        {
            properties[legacy_name] = value;
            attributes[legacy_name] = FVM_PROPERTY_DONT_ENUM;
        }
        /* [GC:] Swapping publishes both identities before callbacks can observe them; all
           allocating work is complete before obsolete descriptor/capture edges are removed. */
        receiver->properties.swap(properties);
        receiver->attributes.swap(attributes);
        discard_virtual_slot(receiver, internal_name);
        if (legacy)
        {
            discard_virtual_slot(receiver, legacy_name);
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}


/* [GC:] Publish only a complete receiver; callers retain it while coercing native arguments. */
FVM_Result fvm_function_allocate_instance(FVM_Context *c, FVM_Object *callee, int resource_kind, FVM_Object **out)
{
    if (out)
    {
        *out = 0;
    }
    if (!c || !c->swf_loaded || !callee || callee->owner != c || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        FVM_ObjectRootScope callee_root(c, callee);
        FVM_Value prototype;
        FVM_Result status = fvm_avm1_stored_prototype(c, callee, prototype);
        if (status != FVM_OK)
        {
            return status;
        }
        FVM_Object *instance = 0;
        status = fvm_object_create(c, &instance);
        if (status != FVM_OK)
        {
            return status;
        }


        FVM_ObjectRootScope instance_root(c, instance);
        instance->resource_kind = resource_kind;
        instance->prototype = prototype.type == FVM_VALUE_OBJECT ? prototype.object : 0;
        status = fvm_function_define_constructor(c, instance, callee);
        if (status == FVM_OK)
        {
            *out = instance;
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
