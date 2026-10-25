/* FlashVM: transactional raw __proto__ storage for loaded SWF objects. */
#include "prototype_data.hpp"
#include "property_lookup.hpp"
#include <new>
#include "flashvm/property.h"
#include "../standard/base_state.hpp"


/* [GC:] Publish owned data, attributes and the inheritance edge only after preparation succeeds. */
FVM_Result fvm_object_store_raw_prototype(FVM_Context *c, FVM_Object *object, const FVM_Value &value)
{
    if (!c || !object || object->owner != c || fvm_validate_value(c, &value) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *prototype = value.type == FVM_VALUE_OBJECT ? value.object : 0;
    FVM_Object *cursor = prototype;
    unsigned int depth = 0;
    while (cursor && depth++ < FVM_PROTOTYPE_MAX_DEPTH)
    {
        if (cursor == object)
        {
            return FVM_INVALID_ARGUMENT;
        }
        cursor = cursor->prototype;
    }
    if (cursor)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {
        fvm_detail::map<fvm_detail::string, FVM_Value> properties = object->properties;
        fvm_detail::map<fvm_detail::string, unsigned int> attributes = object->attributes;
        properties["__proto__"] = value;
        if (object->properties.find("__proto__") == object->properties.end() &&
            attributes.find("__proto__") == attributes.end())
        {
            attributes["__proto__"] = FVM_PROPERTY_BUILTIN_FLAGS;
        }
        /* [GC:] No callback or allocation may separate the raw slot from its managed edge. */
        object->properties.swap(properties);
        object->attributes.swap(attributes);
        object->prototype = prototype;
        fvm_property_clear_versions(c, object, "__proto__");
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}


/* Only this intrinsic name is folded here; ordinary property casing remains separate. */
bool fvm_object_prototype_name(FVM_Context *c, const fvm_detail::string &key)
{
    const char expected[] = "__proto__";
    if (!c->swf_loaded || c->swf_info.version >= FVM_SWF_VERSION_7)
    {
        return key == fvm_detail::string(expected);
    }
    if (key.size() != sizeof(expected) - 1)
    {
        return false;
    }
    for (size_t i = 0; i < key.size(); ++i)
    {
        char unit = key[i];
        if (unit >= 'A' && unit <= 'Z')
        {
            unit = (char)(unit + ('a' - 'A'));
        }
        if (unit != expected[i])
        {
            return false;
        }
    }
    return true;
}


/* A custom descriptor or raw slot takes precedence over the inherited intrinsic. */
bool fvm_object_intrinsic_prototype(FVM_Context *c, FVM_Object *object, const fvm_detail::string *alternate)
{
    if (!c->swf_loaded)
    {
        return false;
    }
    for (unsigned int depth = 0; object && depth < FVM_PROTOTYPE_MAX_DEPTH; ++depth, object = object->prototype)
    {
        if (alternate && *alternate != fvm_detail::string("__proto__") &&
            (object->properties.find(*alternate) != object->properties.end() ||
             object->methods.find(*alternate) != object->methods.end() ||
             object->accessors.find(*alternate) != object->accessors.end()))
        {
            return false;
        }
        if (object->properties.find("__proto__") != object->properties.end() ||
            object->methods.find("__proto__") != object->methods.end())
        {
            return false;
        }
        fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator entry = object->accessors.find("__proto__");
        if (entry != object->accessors.end())
        {
            return fvm_base_prototype_accessor(entry->second);
        }
    }
    return false;
}
