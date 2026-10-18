/* FlashVM: allocation-stage helpers for default native method property attributes. */
#ifndef FVM_STANDARD_ATTRIBUTE_STAGE_HPP
#define FVM_STANDARD_ATTRIBUTE_STAGE_HPP /* Guard the internal attribute preparation helpers. */
#include "../core/runtime.hpp"
#include "flashvm/property.h"
#define FVM_STANDARD_SCRIPT_METHOD_FLAGS 0U /* Ordinary script assignment has no attribute restrictions. */

/* [GC:] Prepared attributes reference a prototype retained by its constructor capture. */
struct FVM_MethodAttributeActivation
{
    FVM_Object *prototype; /* Candidate whose native methods still have host defaults. */
    fvm_detail::map<fvm_detail::string, unsigned int> attributes; /* Complete unpublished flag table. */
    /* Begin without a selected native prototype. */
    FVM_MethodAttributeActivation() : prototype(0) {}
};
/* Publish only prepared tables; swapping does not allocate or invoke callbacks. */
inline void fvm_commit_method_attributes(FVM_MethodAttributeActivation &prepared)
{
    if (prepared.prototype)
        prepared.prototype->attributes.swap(prepared.attributes);
}

/* Upgrade one canonical native slot in an already copied attribute table. Allocating
 * lookups may throw: callers retain the object and run this only before mutation. */
inline void fvm_stage_default_method_flags(FVM_Object *object, const char *name,
                                           FVM_NativeFunction callback, unsigned int desired,
                                           fvm_detail::map<fvm_detail::string, unsigned int> &attributes)
{
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method = object->methods.find(name);
    fvm_detail::map<fvm_detail::string, unsigned int>::iterator flags = attributes.find(name);
    if (method != object->methods.end() && method->second.callback == callback &&
        flags != attributes.end() && flags->second == FVM_PROPERTY_DONT_ENUM &&
        object->properties.find(name) == object->properties.end() &&
        object->accessors.find(name) == object->accessors.end())
        flags->second = desired;
}
#endif
