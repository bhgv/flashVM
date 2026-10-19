/* FlashVM: explicit SWF5--8 cursor methods and version-gated Mouse listener namespace over host-delivered pointer events. */
#include "../core/runtime.hpp"
#include "mouse_listeners.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/property_lookup.hpp"
#include "flashvm/mouse.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include <new>

namespace
{
/* Native method definition used for both initial installation and idempotency checks. */
struct Method
{
    const char *name;            /* Static ActionScript method name. */
    FVM_NativeFunction callback; /* Stateless callback; reads the invoking context. */
};
const Method methods[] = {{"hide", fvm_mouse_hide}, {"show", fvm_mouse_show},
                          {"addListener", fvm_mouse_add_listener},
                          {"removeListener", fvm_mouse_remove_listener}};

/* Recognize our unshadowed methods without executing getters or allocating a new namespace. */
bool installed(FVM_Context *context, FVM_Object *object)
{
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
    {
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method =
            fvm_property_find(context, object->methods, fvm_detail::string(methods[i].name));
        if (method == object->methods.end() || method->second.callback != methods[i].callback ||
            method->second.userdata || method->second.root || method->second.collector_entry ||
            fvm_property_find(context, object->properties, fvm_detail::string(methods[i].name)) !=
                object->properties.end() ||
            fvm_property_find(context, object->accessors, fvm_detail::string(methods[i].name)) !=
                object->accessors.end())
            return false;
    }
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator listeners =
        fvm_property_find(context, object->accessors, fvm_detail::string("_listeners"));
    return listeners != object->accessors.end() &&
           listeners->second.getter == fvm_mouse_listener_array && !listeners->second.setter &&
           !listeners->second.userdata && !listeners->second.root &&
           fvm_property_find(context, object->properties, fvm_detail::string("_listeners")) ==
               object->properties.end() &&
           fvm_property_find(context, object->methods, fvm_detail::string("_listeners")) ==
               object->methods.end();
}
} // namespace

/* Publish only a fully prepared namespace; conflicts and allocation failures preserve globals. */
extern "C" FVM_Result fvm_standard_mouse_install(FVM_Context *context)
{
    if (!context || context->destroying || context->gc_collecting || context->gc_releasing ||
        !context->standard_library)
        return FVM_INVALID_ARGUMENT;
    if (context->swf_loaded && context->swf_info.version < FVM_MOUSE_CURSOR_MIN_SWF_VERSION)
        return FVM_UNSUPPORTED;
    try
    {
        if (fvm_property_find(context, context->global->accessors, fvm_detail::string("Mouse")) !=
                context->global->accessors.end() ||
            fvm_property_find(context, context->global->methods, fvm_detail::string("Mouse")) !=
                context->global->methods.end() ||
            fvm_property_find(context, context->functions, fvm_detail::string("Mouse")) !=
                context->functions.end() ||
            fvm_property_find(context, context->constructors, fvm_detail::string("Mouse")) !=
                context->constructors.end() ||
            fvm_property_find(context, context->stack_functions, fvm_detail::string("Mouse")) !=
                context->stack_functions.end() ||
            fvm_property_find(context, context->script_functions, fvm_detail::string("Mouse")) !=
                context->script_functions.end())
            return FVM_INVALID_ARGUMENT;
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator existing =
            fvm_property_find(context, context->global->properties, fvm_detail::string("Mouse"));
        if (existing != context->global->properties.end())
            return existing->second.type == FVM_VALUE_OBJECT && existing->second.object &&
                           installed(context, existing->second.object)
                       ? FVM_OK
                       : FVM_INVALID_ARGUMENT;
        FVM_Object *object = 0;
        FVM_Result result = fvm_object_create(context, &object);
        if (result != FVM_OK)
            return result;
        /* [GC:] Keep the unpublished namespace alive throughout all allocating preparation. */
        FVM_ObjectRootScope root(context, object);
        for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
        {
            FVM_NativeEntry entry;
            entry.callback = methods[i].callback;
            object->methods[methods[i].name] = entry;
            object->attributes[methods[i].name] = FVM_PROPERTY_DONT_ENUM;
        }
        FVM_AccessorEntry listeners;
        listeners.getter = fvm_mouse_listener_array;
        object->accessors["_listeners"] = listeners;
        object->attributes["_listeners"] = FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE;
        /* Copy before publication so map growth cannot leave a partial global binding. */
        fvm_detail::map<fvm_detail::string, FVM_Value> globals = context->global->properties;
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = object;
        globals["Mouse"] = value;
        context->global->properties.swap(globals);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
