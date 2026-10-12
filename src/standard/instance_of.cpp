/* FlashVM: public instanceof bridge for native classes, functions and interfaces. */
#include "inheritance.hpp"
#include "flashvm/standard.h"

/* Validate inputs and evaluate class/interface membership by retained graph identity. */
extern "C" FVM_Result fvm_value_instance_of(FVM_Context *c, const FVM_Value *value,
                                            const FVM_Value *constructor, int *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    /* [GC:] Reject operations during context destruction or resource-release callbacks. */
    if (!c || c->destroying || c->gc_releasing || fvm_validate_value(c, value) != FVM_OK ||
        fvm_validate_value(c, constructor) != FVM_OK || constructor->type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    if (!fvm_is_class_constructor(constructor->object))
        return FVM_UNSUPPORTED;
    return fvm_class_instance_of(c, value, constructor->object, out);
}
