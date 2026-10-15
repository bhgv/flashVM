/* FlashVM: select the live registered class used by native attachment and duplication. */
#include "movieclip_construction.hpp"
#include "movieclip_storage.hpp"
#include "base_state.hpp"
using namespace fvm_movieclip_storage;
/* [GC:] The caller retains the library/constructor and blocks collection through prototype getters.
 */
FVM_Result fvm_movieclip_sprite_class(FVM_Context *c, FVM_Object *library, const char *linkage,
                                      FVM_Object *&proto, FVM_Object *&constructor)
{
    FVM_Object *classes = 0;
    constructor = 0;
    FVM_Result r = private_object(library, "prototype", proto);
    if (r != FVM_OK)
        return r;
    r = fvm_base_class_registry(c, &classes);
    if (r == FVM_NOT_FOUND)
    {
        r = library->properties.find("classes") == library->properties.end()
                ? FVM_OK
                : private_object(library, "classes", classes);
    }
    if (r != FVM_OK)
        return r;
    if (classes)
    {
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator registered =
            classes->properties.find(linkage);
        if (registered != classes->properties.end())
        {
            if (registered->second.type != FVM_VALUE_OBJECT ||
                !registered->second.object->invocable.callback)
                return FVM_INVALID_ARGUMENT;
            constructor = registered->second.object;
            FVM_Value prototype;
            r = fvm_object_get(c, constructor, "prototype", &prototype);
            if (r != FVM_OK)
                return r;
            if (prototype.type != FVM_VALUE_OBJECT)
                return FVM_INVALID_ARGUMENT;
            proto = prototype.object;
        }
    }
    return FVM_OK;
}
