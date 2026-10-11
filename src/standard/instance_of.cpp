#include "../core/runtime.hpp"
#include "flashvm/standard.h"

extern "C" FVM_Result fvm_value_instance_of(FVM_Context *c, const FVM_Value *value,
                                            const FVM_Value *constructor, int *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || c->destroying || c->gc_releasing || fvm_validate_value(c, value) != FVM_OK ||
        fvm_validate_value(c, constructor) != FVM_OK || constructor->type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    /* Class objects are currently native metadata, not first-class AS functions.
     * Ordinary objects with a property named prototype are not constructors. */
    int kind = constructor->object->resource_kind;
    if (kind != 21 && kind != 23 && kind != 25 && kind != 1003 && kind != 1004)
        return FVM_UNSUPPORTED;
    try
    {
        FVM_Value prototype;
        FVM_Result result = fvm_object_get(c, constructor->object, "prototype", &prototype);
        if (result != FVM_OK)
            return result;
        if (prototype.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        if (value->type != FVM_VALUE_OBJECT)
            return FVM_OK;
        FVM_Object *cursor = value->object->prototype;
        unsigned int depth = 0;
        while (cursor && depth++ < 1024)
        {
            if (cursor == prototype.object)
            {
                *out = 1;
                return FVM_OK;
            }
            cursor = cursor->prototype;
        }
        return cursor ? FVM_ERROR : FVM_OK;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
