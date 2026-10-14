/* FlashVM: allocation-stage helpers for default native method property attributes. */
#ifndef FVM_STANDARD_ATTRIBUTE_STAGE_HPP
#define FVM_STANDARD_ATTRIBUTE_STAGE_HPP /* Guard the internal attribute preparation helpers. */
#include "../core/runtime.hpp"

/* Upgrade one canonical native slot in an already copied attribute table. Allocating
 * lookups may throw: callers retain the object and run this only before mutation. */
inline void fvm_stage_default_method_flags(FVM_Object *object, const char *name,
                                           FVM_NativeFunction callback, unsigned int desired,
                                           fvm_detail::map<fvm_detail::string, unsigned int> &attributes)
{
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method = object->methods.find(name);
    fvm_detail::map<fvm_detail::string, unsigned int>::iterator flags = attributes.find(name);
    if (method != object->methods.end() && method->second.callback == callback &&
        flags != attributes.end() && flags->second == 1 &&
        object->properties.find(name) == object->properties.end() &&
        object->accessors.find(name) == object->accessors.end())
        flags->second = desired;
}
#endif
