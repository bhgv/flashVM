/* FlashVM: getter-free own class prototype data, including retained virtual-property values. */
#ifndef FVM_CLASS_PROTOTYPE_HPP
#define FVM_CLASS_PROTOTYPE_HPP /* Shared class prototype helper guard. */
#include "../scope_keywords.hpp"
#include "class_prototype_constants.h"
#include "../../core/property_lookup.hpp"
#include "../../standard/base_state.hpp"
/* Read the own property's stored data; skip getters, inherited slots and resolver callbacks. */
inline bool fvm_avm1_stored_prototype(FVM_Context *c, FVM_Object *object, FVM_Value &out)
{
    fvm_detail::string name(FVM_CLASS_PROTOTYPE_NAME);
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator data =
        fvm_property_find(c, object->properties, name);
    if (data != object->properties.end())
    {
        out = data->second;
        return true;
    }
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor =
        fvm_property_find(c, object->accessors, name);
    if (accessor == object->accessors.end())
        return false;
    /* Script virtual properties retain the data overwritten by addProperty. Native
       accessors without such stored data represent undefined here, never a getter call. */
    fvm_value_undefined(&out);
    fvm_base_accessor_data(accessor->second, out);
    return true;
}
#endif
