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
#endif
