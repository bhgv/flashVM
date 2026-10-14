/* FlashVM: loaded interface prototype snapshots and legacy host interface declarations. */
#include "support.hpp"
#include "implements_constants.h"
#include "class_prototype.hpp"
#include "../../standard/inheritance.hpp"
/* Install the first SWF7+ snapshot list without getters; primitive/invalid entries are ignored. */
extern "C" FVM_Result fvm_exec_implements(FVM_Context *c, const FVM_Value *constructor,
                                          unsigned int count, const FVM_Value *interfaces)
{
    if (!c || fvm_validate_value(c, constructor) != FVM_OK ||
        count > FVM_EXEC_MAX_INTERFACE_ARGUMENTS || (count && !interfaces))
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < count; ++i)
        if (fvm_validate_value(c, interfaces + i) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, (size_t)count + FVM_IMPLEMENTS_ARGUMENT_BASE);
        roots.values[FVM_IMPLEMENTS_CLASS_ROOT] = *constructor;
        for (unsigned int i = 0; i < count; ++i)
            roots.values[FVM_IMPLEMENTS_ARGUMENT_BASE + i] = interfaces[i];
        if (!c->swf_loaded)
            return constructor->type == FVM_VALUE_OBJECT
                       ? fvm_class_implements(
                             c, constructor->object, count,
                             count ? roots.values.begin() + FVM_IMPLEMENTS_ARGUMENT_BASE : 0)
                       : FVM_INVALID_ARGUMENT;
        if (!count || c->swf_info.version < FVM_SWF_VERSION_7 ||
            constructor->type != FVM_VALUE_OBJECT)
            return FVM_OK;
        fvm_detail::vector<FVM_Object *> snapshots;
        for (unsigned int i = 0; i < count; ++i)
        {
            const FVM_Value &input = roots.values[FVM_IMPLEMENTS_ARGUMENT_BASE + i];
            FVM_Value prototype;
            fvm_value_undefined(&prototype);
            if (input.type == FVM_VALUE_OBJECT &&
                fvm_avm1_stored_prototype(c, input.object, prototype) &&
                prototype.type == FVM_VALUE_OBJECT)
                snapshots.push_back(prototype.object);
        }
        FVM_Value target;
        fvm_value_undefined(&target);
        if (fvm_avm1_stored_prototype(c, constructor->object, target) &&
            target.type == FVM_VALUE_OBJECT && !target.object->interfaces_are_prototypes)
        {
            /* [GC:] Raw reads above invoke no callbacks. Publish retained prototype edges
             * only after all allocations succeed; an installed empty list remains installed. */
            target.object->implemented_interfaces.swap(snapshots);
            target.object->interfaces_are_prototypes = true;
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
