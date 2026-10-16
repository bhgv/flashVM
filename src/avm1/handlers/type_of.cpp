/* FlashVM: shared existing AVM1 type-name projection independent of operand encoding. */
#include "support.hpp"
/* Classify validated values without allocation, coercion callbacks or collector entry. */
extern "C" FVM_Result fvm_exec_type_of(FVM_Context *c, const FVM_Value *value, FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    const char *name = 0;
    switch (value->type)
    {
    case FVM_VALUE_UNDEFINED:
        name = "undefined";
        break;
    case FVM_VALUE_NULL:
        name = "null";
        break;
    case FVM_VALUE_BOOLEAN:
        name = "boolean";
        break;
    case FVM_VALUE_NUMBER:
        name = "number";
        break;
    case FVM_VALUE_STRING:
        name = "string";
        break;
    case FVM_VALUE_OBJECT:
        /* A callable parent facade is not an ordinary loaded Function value. */
        if (c->swf_loaded && value->object->resource_kind == FVM_RESOURCE_SUPER)
        {
            name = "object";
            break;
        }
        name = value->object->invocable.callback || value->object->callable.callback ? "function"
               : value->object->resource_kind == FVM_RESOURCE_MOVIECLIP              ? "movieclip"
                                                                                     : "object";
        break;
    default:
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_value_string(out, name);
}
