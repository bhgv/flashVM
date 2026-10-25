/* FlashVM: ASSetPropFlags selects stored own names case-insensitively in every loaded SWF. */
#ifndef FVM_STANDARD_PROPERTY_FLAG_LOOKUP_HPP
#define FVM_STANDARD_PROPERTY_FLAG_LOOKUP_HPP /* Private flag-only name selection guard. */
#include "../core/runtime.hpp"
#include "../avm1/scope_keywords.hpp"


/* Select a canonical key without reading data or invoking a getter; copies may throw. */
template <class Entries> bool fvm_property_flag_select_name(Entries &entries, fvm_detail::string &name)
{
    if (entries.find(name) != entries.end())
    {
        return true;
    }
    for (typename Entries::iterator entry = entries.begin(); entry != entries.end(); ++entry)
    {
        if (fvm_avm1_folded_name_equal(entry->first, name))
        {
            name = entry->first;
            return true;
        }
    }
    return false;
}


/* Attribute mutation targets an own slot and must never select an inherited property. */
inline bool fvm_property_flag_select_own_name(FVM_Object *object, fvm_detail::string &name)
{
    return fvm_property_flag_select_name(object->properties, name) ||
           fvm_property_flag_select_name(object->accessors, name) ||
           fvm_property_flag_select_name(object->methods, name);
}
#endif
