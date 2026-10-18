/* FlashVM: explicit SWF6--8 Selection MovieClip subset over the host focus root. */
#include "movieclip_state.hpp"
#include "movieclip_scene.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/property_lookup.hpp"
#include "flashvm/keyboard.h"
#include "flashvm/property.h"
#include <new>

namespace
{
/* This subset does not claim the older text-field-only Selection implementation. */
bool supported(FVM_Context *context)
{
    return !context->swf_loaded || context->swf_info.version >= FVM_SELECTION_MIN_SWF_VERSION;
}

/* Read eligibility once, preserving actual getter/resolve failures and their side effects. */
FVM_Result eligible(FVM_Context *context, FVM_Object *clip, bool *out)
{
    *out = false;
    FVM_Value flag, converted;
    FVM_Result result = fvm_object_get(context, clip, "focusEnabled", &flag);
    if (result == FVM_OK)
        result = fvm_exec_to_boolean(context, &flag, &converted);
    if (result != FVM_OK)
        return result;
    if (converted.boolean)
    {
        *out = true;
        return FVM_OK;
    }
    /* The documented button-like MovieClip handlers allow programmatic selection. */
    const char *handlers[] = {"onDragOut",        "onDragOver", "onPress",   "onRelease",
                              "onReleaseOutside", "onRollOut",  "onRollOver"};
    for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); ++i)
    {
        FVM_Value handler;
        result = fvm_object_get(context, clip, handlers[i], &handler);
        if (result != FVM_OK)
            return result;
        if (handler.type == FVM_VALUE_OBJECT && handler.object &&
            handler.object->invocable.callback)
        {
            *out = true;
            return FVM_OK;
        }
    }
    return FVM_OK;
}

/* Select an eligible native clip or clear through null/undefined; never fabricate text focus. */
FVM_Result fvm_selection_set_focus(FVM_Context *context, FVM_Object *, unsigned int count,
                                   const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_boolean(out, 0);
    if (!supported(context))
        return FVM_UNSUPPORTED;
    if (!count || args[0].type == FVM_VALUE_NULL || args[0].type == FVM_VALUE_UNDEFINED)
    {
        FVM_Result result = fvm_input_set_keyboard_focus(context, 0);
        if (result == FVM_OK)
            fvm_value_boolean(out, 1);
        return result;
    }
    FVM_Object *clip = 0;
    if (args[0].type == FVM_VALUE_OBJECT)
        clip = args[0].object;
    else if (args[0].type == FVM_VALUE_STRING)
    {
        FVM_Object *start =
            context->avm1_target_clip ? context->avm1_target_clip : context->avm1_base_clip;
        if (!start)
            return FVM_OK;
        FVM_Result result = fvm_movieclip_resolve_path(context, start, args[0], &clip);
        if (result != FVM_OK)
            return result;
    }
    if (!clip || clip->owner != context || clip->resource_kind != FVM_RESOURCE_MOVIECLIP ||
        fvm_movieclip_is_class_prototype(clip))
        return FVM_OK;
    /* [GC:] Property getters may remove all ordinary references and collect the candidate. */
    FVM_ObjectRootScope root(context, clip);
    bool removed = false, allowed = false;
    FVM_Result result = fvm_movieclip_scene_removed(context, clip, &removed);
    if (result != FVM_OK || removed)
        return result;
    result = eligible(context, clip, &allowed);
    if (result != FVM_OK || !allowed)
        return result;
    /* A getter can detach the candidate; validate again immediately before publication. */
    result = fvm_input_set_keyboard_focus(context, clip);
    if (result == FVM_INVALID_ARGUMENT)
        return FVM_OK;
    if (result == FVM_OK)
        fvm_value_boolean(out, 1);
    return result;
}

/* Return a counted native dot path; detached selections are cleared by the shared query. */
FVM_Result fvm_selection_get_focus(FVM_Context *context, FVM_Object *, unsigned int,
                                   const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    out->type = FVM_VALUE_NULL;
    if (!supported(context))
        return FVM_UNSUPPORTED;
    FVM_Object *clip = 0;
    FVM_Result result = fvm_input_get_keyboard_focus(context, &clip);
    if (result != FVM_OK || !clip)
        return result;
    return fvm_movieclip_reference_string(context, clip, out);
}

/* Static method identities are used to recognize an intact explicit namespace. */
struct Method
{
    const char *name;            /* Public method name in the Selection namespace. */
    FVM_NativeFunction callback; /* Native receiver-bound entry, without owned userdata. */
};
const Method methods[] = {{"getFocus", fvm_selection_get_focus},
                          {"setFocus", fvm_selection_set_focus}};
/* Recognize our unshadowed methods without executing getters or allocating a new namespace. */
bool installed(FVM_Context *context, FVM_Object *object)
{
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
    {
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method =
            fvm_property_find(context, object->methods, fvm_detail::string(methods[i].name));
        if (method == object->methods.end() || method->second.callback != methods[i].callback ||
            method->second.userdata || method->second.root || !method->second.collector_entry ||
            fvm_property_find(context, object->properties, fvm_detail::string(methods[i].name)) !=
                object->properties.end() ||
            fvm_property_find(context, object->accessors, fvm_detail::string(methods[i].name)) !=
                object->accessors.end())
            return false;
    }
    return true;
}
} // namespace

/* Publish only a fully prepared namespace; conflicts and allocation failures preserve globals. */
extern "C" FVM_Result fvm_standard_selection_install(FVM_Context *context)
{
    if (!context || context->destroying || context->gc_collecting || context->gc_releasing ||
        !context->standard_library)
        return FVM_INVALID_ARGUMENT;
    if (!supported(context))
        return FVM_UNSUPPORTED;
    try
    {
        if (fvm_property_find(context, context->global->accessors,
                              fvm_detail::string("Selection")) !=
                context->global->accessors.end() ||
            fvm_property_find(context, context->global->methods, fvm_detail::string("Selection")) !=
                context->global->methods.end() ||
            fvm_property_find(context, context->functions, fvm_detail::string("Selection")) !=
                context->functions.end() ||
            fvm_property_find(context, context->constructors, fvm_detail::string("Selection")) !=
                context->constructors.end() ||
            fvm_property_find(context, context->stack_functions, fvm_detail::string("Selection")) !=
                context->stack_functions.end() ||
            fvm_property_find(context, context->script_functions,
                              fvm_detail::string("Selection")) != context->script_functions.end())
            return FVM_INVALID_ARGUMENT;
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator existing = fvm_property_find(
            context, context->global->properties, fvm_detail::string("Selection"));
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
            /* [GC:] Candidate/path/argument roots permit collection in nested script getters. */
            entry.collector_entry = true;
            object->methods[methods[i].name] = entry;
            object->attributes[methods[i].name] = FVM_PROPERTY_DONT_ENUM;
        }
        /* Copy before publication so map growth cannot leave a partial global binding. */
        fvm_detail::map<fvm_detail::string, FVM_Value> globals = context->global->properties;
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = object;
        globals["Selection"] = value;
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
