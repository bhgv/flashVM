/* FlashVM: select the live registered class used by native attachment and duplication. */
#include "movieclip_construction.hpp"
#include "movieclip_storage.hpp"
#include "base_state.hpp"
#include "movieclip_state.hpp"
#include "../avm1/handlers/class_prototype.hpp"
using namespace fvm_movieclip_storage;
/* [GC:] Caller retains library/constructor; only legacy host libraries can invoke prototype getters. */
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
        unsigned int version = 0;
        r = fvm_movieclip_library_version(c, library, &version);
        if (r != FVM_OK)
            return r;
        r = fvm_base_registered_class_version(c, classes, linkage, version, &constructor);
        if (r != FVM_OK && r != FVM_NOT_FOUND)
            return r;
        if (constructor)
        {
            FVM_Value prototype;
            if (version || c->swf_loaded)
            {
                /* Source SWF construction reads own stored data, never a getter or resolver. */
                fvm_value_undefined(&prototype);
                fvm_avm1_stored_prototype(c, constructor, prototype, version);
                proto = prototype.type == FVM_VALUE_OBJECT ? prototype.object : 0;
            }
            else
            {
                /* Libraries without SWF provenance retain the unloaded embedding contract. */
                r = fvm_object_get(c, constructor, "prototype", &prototype);
                if (r != FVM_OK)
                    return r;
                if (prototype.type != FVM_VALUE_OBJECT)
                    return FVM_INVALID_ARGUMENT;
                proto = prototype.object;
            }
        }
    }
    return FVM_OK;
}
