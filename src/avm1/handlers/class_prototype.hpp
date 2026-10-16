/* FlashVM: getter-free own class prototype data, including retained virtual-property values. */
#ifndef FVM_CLASS_PROTOTYPE_HPP
#define FVM_CLASS_PROTOTYPE_HPP /* Shared class prototype helper guard. */
#include "../scope_keywords.hpp"
#include "class_prototype_constants.h"
#include "../../standard/base_state.hpp"
#include "../../standard/function_state.hpp"
/* Select the prototype field under an explicit source-movie policy or the ambient context. */
template <class T>
inline typename fvm_detail::map<fvm_detail::string, T>::iterator fvm_avm1_prototype_slot(
    FVM_Context *c, fvm_detail::map<fvm_detail::string, T> &entries, unsigned int movie_version)
{
    typename fvm_detail::map<fvm_detail::string, T>::iterator found =
        entries.find(FVM_CLASS_PROTOTYPE_NAME);
    bool folded = movie_version ? movie_version < FVM_SWF_VERSION_7
                                 : c->swf_loaded && c->swf_info.version < FVM_SWF_VERSION_7;
    if (found != entries.end() || !folded)
        return found;
    for (found = entries.begin(); found != entries.end(); ++found)
        if (fvm_avm1_keyword_name(found->first, FVM_CLASS_PROTOTYPE_NAME,
                                   FVM_CLASS_PROTOTYPE_UNITS, false))
            return found;
    return found;
}
/* Read stored own data; an explicit source version also works in an unloaded receiving context. */
inline FVM_Result fvm_avm1_stored_prototype(FVM_Context *c, FVM_Object *object, FVM_Value &out,
                                       unsigned int movie_version = 0)
{
    fvm_value_undefined(&out);
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator data =
        fvm_avm1_prototype_slot(c, object->properties, movie_version);
    if (data != object->properties.end())
    {
        out = data->second;
        return FVM_OK;
    }
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor =
        fvm_avm1_prototype_slot(c, object->accessors, movie_version);
    if (accessor == object->accessors.end())
    {
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method =
            fvm_avm1_prototype_slot(c, object->methods, movie_version);
        if (method != object->methods.end())
            return fvm_function_wrap_entry(c, object, method->first, method->second, &out);
        return FVM_OK;
    }
    /* Script virtual properties retain the data overwritten by addProperty. Native
       accessors without such stored data represent undefined here, never a getter call. */
    fvm_value_undefined(&out);
    fvm_base_accessor_data(accessor->second, out);
    return FVM_OK;
}
#endif
