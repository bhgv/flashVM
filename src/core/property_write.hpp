/* FlashVM: raw own-data membership used when selecting loaded write dispatch. */
#ifndef FVM_CORE_PROPERTY_WRITE_HPP
#define FVM_CORE_PROPERTY_WRITE_HPP /* Private write-membership helper guard. */
#include "../standard/base_state.hpp"
#include "property_lookup.hpp"
#include "property_key.hpp"

/* Own data, including hidden native methods and indexed storage, prevents a
 * prototype setter search. Inherited data does not prevent that search. */
inline bool fvm_property_own_write_data(FVM_Context *c, FVM_Object *object,
                                       const fvm_detail::string &name)
{
    if (fvm_property_find(c, object->properties, name) != object->properties.end() ||
        fvm_property_find(c, object->methods, name) != object->methods.end())
        return true;
    unsigned long index = 0;
    return (object->resource_kind == FVM_RESOURCE_ARRAY || object->resource_kind == FVM_RESOURCE_OBJECT) &&
        fvm_property_terminated(name) && fvm_as_index(name.c_str(), &index) &&
        object->indexed.find(index) != object->indexed.end();
}
#endif
