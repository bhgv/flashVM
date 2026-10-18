/* FlashVM: explicit AS1/AS2 Key polling namespace over host-published native state. */
#include "../core/runtime.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/property_lookup.hpp"
#include "flashvm/keyboard.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include <new>

namespace
{
/* Return the latest host key-down code without querying the operating system. */
FVM_Result get_code(FVM_Context *context, FVM_Object *, unsigned int, const FVM_Value *,
                    FVM_Value *out, void *)
{
    fvm_value_number(out, context->keyboard.last_code);
    return FVM_OK;
}

/* Return the English character code of the latest accepted transition. */
FVM_Result get_ascii(FVM_Context *context, FVM_Object *, unsigned int, const FVM_Value *,
                     FVM_Value *out, void *)
{
    fvm_value_number(out, context->keyboard.last_ascii);
    return FVM_OK;
}

/* Coerce the requested virtual-key slot before reading the latest native state. */
FVM_Result query(FVM_Context *context, unsigned int count, const FVM_Value *args, FVM_Value *out,
                 bool toggled)
{
    double number = 0;
    if (count)
    {
        FVM_Result result = fvm_value_to_number(context, args, &number);
        if (result != FVM_OK)
            return result;
    }
    bool active = false;
    if (number >= FVM_KEYBOARD_FIRST_CODE && number < FVM_KEYBOARD_CODE_COUNT)
    {
        unsigned int code = (unsigned int)number;
        active = toggled ? (code == FVM_KEY_CAPSLOCK  ? context->keyboard.caps_lock != 0
                            : code == FVM_KEY_NUMLOCK ? context->keyboard.num_lock != 0
                                                      : false)
                         : context->keyboard.down[code] != 0;
    }
    fvm_value_boolean(out, active ? 1 : 0);
    return FVM_OK;
}

/* Read one held slot; additional arguments are ignored, absent/invalid slots are false. */
FVM_Result is_down(FVM_Context *context, FVM_Object *, unsigned int count, const FVM_Value *args,
                   FVM_Value *out, void *)
{
    return query(context, count, args, out, false);
}

/* Read Caps Lock or Num Lock toggle independently of the physical key state. */
FVM_Result is_toggled(FVM_Context *context, FVM_Object *, unsigned int count, const FVM_Value *args,
                      FVM_Value *out, void *)
{
    return query(context, count, args, out, true);
}

/* Native method definition used for both initial installation and idempotency checks. */
struct Method
{
    const char *name;            /* Static ActionScript method name. */
    FVM_NativeFunction callback; /* Stateless callback; reads the invoking context. */
};
const Method methods[] = {
    {"getCode", get_code}, {"getAscii", get_ascii}, {"isDown", is_down}, {"isToggled", is_toggled}};

/* Public Key constants correspond to documented virtual-key identities. */
struct Constant
{
    const char *name;  /* Static namespace property name. */
    unsigned int code; /* Host ABI key code, independent of SDL scancodes. */
};
const Constant constants[] = {
    {"BACKSPACE", FVM_KEY_BACKSPACE}, {"TAB", FVM_KEY_TAB},         {"ENTER", FVM_KEY_ENTER},
    {"SHIFT", FVM_KEY_SHIFT},         {"CONTROL", FVM_KEY_CONTROL}, {"CAPSLOCK", FVM_KEY_CAPSLOCK},
    {"ESCAPE", FVM_KEY_ESCAPE},       {"SPACE", FVM_KEY_SPACE},     {"PGUP", FVM_KEY_PGUP},
    {"PGDN", FVM_KEY_PGDN},           {"END", FVM_KEY_END},         {"HOME", FVM_KEY_HOME},
    {"LEFT", FVM_KEY_LEFT},           {"UP", FVM_KEY_UP},           {"RIGHT", FVM_KEY_RIGHT},
    {"DOWN", FVM_KEY_DOWN},           {"INSERT", FVM_KEY_INSERT},   {"DELETEKEY", FVM_KEY_DELETE}};

/* Recognize our unshadowed methods without executing getters or allocating a new namespace. */
bool installed(FVM_Context *context, FVM_Object *object)
{
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
    {
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method =
            fvm_property_find(context, object->methods, fvm_detail::string(methods[i].name));
        if (method == object->methods.end() || method->second.callback != methods[i].callback ||
            method->second.userdata || method->second.root ||
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
extern "C" FVM_Result fvm_standard_keyboard_install(FVM_Context *context)
{
    if (!context || context->destroying || context->gc_collecting || context->gc_releasing ||
        !context->standard_library)
        return FVM_INVALID_ARGUMENT;
    try
    {
        if (fvm_property_find(context, context->global->accessors, fvm_detail::string("Key")) !=
                context->global->accessors.end() ||
            fvm_property_find(context, context->global->methods, fvm_detail::string("Key")) !=
                context->global->methods.end() ||
            fvm_property_find(context, context->functions, fvm_detail::string("Key")) !=
                context->functions.end() ||
            fvm_property_find(context, context->constructors, fvm_detail::string("Key")) !=
                context->constructors.end() ||
            fvm_property_find(context, context->stack_functions, fvm_detail::string("Key")) !=
                context->stack_functions.end() ||
            fvm_property_find(context, context->script_functions, fvm_detail::string("Key")) !=
                context->script_functions.end())
            return FVM_INVALID_ARGUMENT;
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator existing =
            fvm_property_find(context, context->global->properties, fvm_detail::string("Key"));
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
        for (size_t i = 0; i < sizeof(constants) / sizeof(constants[0]); ++i)
        {
            FVM_Value value;
            fvm_value_number(&value, constants[i].code);
            object->properties[constants[i].name] = value;
            object->attributes[constants[i].name] =
                FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE | FVM_PROPERTY_READ_ONLY;
        }
        /* Copy before publication so map growth cannot leave a partial global binding. */
        fvm_detail::map<fvm_detail::string, FVM_Value> globals = context->global->properties;
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = object;
        globals["Key"] = value;
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
