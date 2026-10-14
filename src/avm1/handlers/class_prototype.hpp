/* FlashVM: getter-free own class-prototype data lookup with versioned field spelling. */
#ifndef FVM_CLASS_PROTOTYPE_HPP
#define FVM_CLASS_PROTOTYPE_HPP /* Shared class prototype helper guard. */
#include "../scope_keywords.hpp"
#include "class_prototype_constants.h"
/* Read only stored own prototype data; accessors and inherited fields do not participate. */
inline bool fvm_avm1_stored_prototype(FVM_Context *c, FVM_Object *object, FVM_Value &out)
{
    fvm_detail::map<fvm_detail::string, FVM_Value>::const_iterator i =
        object->properties.find(FVM_CLASS_PROTOTYPE_NAME);
    if (i != object->properties.end())
    {
        out = i->second;
        return true;
    }
    if (c->swf_info.version < FVM_SWF_VERSION_7)
        for (i = object->properties.begin(); i != object->properties.end(); ++i)
            if (fvm_avm1_keyword_name(i->first, FVM_CLASS_PROTOTYPE_NAME, FVM_CLASS_PROTOTYPE_UNITS,
                                      false))
            {
                out = i->second;
                return true;
            }
    return false;
}
#endif
