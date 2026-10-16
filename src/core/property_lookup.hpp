/* FlashVM: allocation-free stored-name selection for versioned property queries. */
#ifndef FVM_CORE_PROPERTY_LOOKUP_HPP
#define FVM_CORE_PROPERTY_LOOKUP_HPP /* Guard for the private property-name selector. */
#include "../avm1/scope_keywords.hpp"

/* Select exact spelling first; loaded SWF4–6 also use Flash UTF-16 lowercase
 * equivalence. Host/SWF7–8 retain exact names. No value/getter is evaluated. */
template <class Entries>
typename Entries::iterator fvm_property_find(FVM_Context *context, Entries &entries,
                                             const fvm_detail::string &name)
{
    typename Entries::iterator found = entries.find(name);
    if (found != entries.end() || !context->swf_loaded ||
        context->swf_info.version >= FVM_SWF_VERSION_7)
        return found;
    for (found = entries.begin(); found != entries.end(); ++found)
        if (fvm_avm1_folded_name_equal(found->first, name))
            return found;
    return found;
}

/* Preserve the stored own spelling before a mutating operation can invalidate
 * table iterators. The caller owns name and handles a possible copy failure. */
inline bool fvm_property_select_own_name(FVM_Context *context, FVM_Object *object,
                                         fvm_detail::string &name)
{
    if (!context->swf_loaded || context->swf_info.version >= FVM_SWF_VERSION_7)
        return false;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator data =
        fvm_property_find(context, object->properties, name);
    if (data != object->properties.end())
    {
        name = data->first;
        return true;
    }
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor =
        fvm_property_find(context, object->accessors, name);
    if (accessor != object->accessors.end())
    {
        name = accessor->first;
        return true;
    }
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method =
        fvm_property_find(context, object->methods, name);
    if (method == object->methods.end())
        return false;
    name = method->first;
    return true;
}
#endif
