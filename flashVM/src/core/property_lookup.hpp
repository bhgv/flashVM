/* FlashVM: allocation-free stored-name selection for versioned property queries. */
#ifndef FVM_CORE_PROPERTY_LOOKUP_HPP
#define FVM_CORE_PROPERTY_LOOKUP_HPP /* Guard for the private property-name selector. */
#include "../avm1/scope_keywords.hpp"
#include "property_version_constants.h"

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

/* Select the current SWF exclusion mask; unloaded embedding has no version restrictions. */
inline unsigned int fvm_property_visibility_mask(FVM_Context *context)
{
    if (!context->swf_loaded)
        return 0;
    unsigned int mask = FVM_PROPERTY_HIDE_SWF4;
    switch (context->swf_info.version)
    {
    case FVM_SWF_VERSION_5: mask = FVM_PROPERTY_HIDE_SWF5; break;
    case FVM_SWF_VERSION_6: mask = FVM_PROPERTY_HIDE_SWF6; break;
    case FVM_SWF_VERSION_7: mask = FVM_PROPERTY_HIDE_SWF7; break;
    case FVM_SWF_VERSION_8: mask = FVM_PROPERTY_HIDE_SWF8; break;
    default: break;
    }
    return mask;
}

/* Check read/getter visibility, leaving own membership and raw-data enumeration intact. */
inline bool fvm_property_available(FVM_Context *context, FVM_Object *object,
                                   const fvm_detail::string &name)
{
    const unsigned int mask = fvm_property_visibility_mask(context);
    if (!mask) return true;
    fvm_detail::map<fvm_detail::string, unsigned int>::iterator flags =
        fvm_property_find(context, object->attributes, name);
    return flags == object->attributes.end() || !(flags->second & mask);
}

/* Numeric index keys are exact ASCII and need no allocating string conversion. */
inline bool fvm_property_available(FVM_Context *context, FVM_Object *object, const char *name)
{
    const unsigned int mask = fvm_property_visibility_mask(context);
    if (!mask) return true;
    const size_t size = std::strlen(name);
    for (fvm_detail::map<fvm_detail::string, unsigned int>::iterator flags = object->attributes.begin();
         flags != object->attributes.end(); ++flags)
        if (flags->first.size() == size && !std::memcmp(flags->first.data(), name, size))
            return !(flags->second & mask);
    return true;
}

/* Clear version bits only after writable own data has been successfully committed. */
inline void fvm_property_clear_versions(FVM_Context *context, FVM_Object *object,
                                        const fvm_detail::string &name)
{
    if (!context->swf_loaded)
        return;
    fvm_detail::map<fvm_detail::string, unsigned int>::iterator flags =
        fvm_property_find(context, object->attributes, name);
    if (flags != object->attributes.end())
    {
        flags->second &= ~FVM_PROPERTY_AVM1_VERSION_MASK;
        if (!flags->second)
            object->attributes.erase(flags->first);
    }
}

/* Exact native keys clear version bits after a commit without allocating temporary names. */
inline void fvm_property_clear_versions(FVM_Context *context, FVM_Object *object, const char *name)
{
    if (!context->swf_loaded) return;
    const size_t size = std::strlen(name);
    for (fvm_detail::map<fvm_detail::string, unsigned int>::iterator flags = object->attributes.begin();
         flags != object->attributes.end(); ++flags)
        if (flags->first.size() == size && !std::memcmp(flags->first.data(), name, size))
        {
            flags->second &= ~FVM_PROPERTY_AVM1_VERSION_MASK;
            if (!flags->second) object->attributes.erase(flags->first);
            return;
        }
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
