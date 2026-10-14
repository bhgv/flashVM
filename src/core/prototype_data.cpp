/* FlashVM: transactional raw __proto__ storage for loaded SWF objects. */
#include "prototype_data.hpp"
#include <new>
/* [GC:] Publish owned data, attributes and the inheritance edge only after preparation succeeds. */
FVM_Result fvm_object_store_raw_prototype(FVM_Context *c, FVM_Object *object,
                                          const FVM_Value &value)
{
    if (!c || !object || object->owner != c || fvm_validate_value(c, &value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *prototype = value.type == FVM_VALUE_OBJECT ? value.object : 0;
    FVM_Object *cursor = prototype;
    unsigned int depth = 0;
    while (cursor && depth++ < FVM_PROTOTYPE_MAX_DEPTH)
    {
        if (cursor == object)
            return FVM_INVALID_ARGUMENT;
        cursor = cursor->prototype;
    }
    if (cursor)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::map<fvm_detail::string, FVM_Value> properties = object->properties;
        fvm_detail::map<fvm_detail::string, unsigned int> attributes = object->attributes;
        properties["__proto__"] = value;
        if (object->properties.find("__proto__") == object->properties.end() &&
            attributes.find("__proto__") == attributes.end())
            attributes["__proto__"] = 3;
        /* [GC:] No callback or allocation may separate the raw slot from its managed edge. */
        object->properties.swap(properties);
        object->attributes.swap(attributes);
        object->prototype = prototype;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
