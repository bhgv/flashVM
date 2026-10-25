/* FlashVM: loaded interface prototype snapshots and legacy host interface declarations. */
#include "support.hpp"
#include "implements_constants.h"
#include "class_prototype.hpp"
#include "../../core/execution_work_scope.hpp"
#include "../../standard/inheritance.hpp"


/* Install the first SWF7+ snapshot list without getters; primitive/invalid entries are ignored. */
extern "C" FVM_Result
fvm_exec_implements(FVM_Context *c, const FVM_Value *constructor, unsigned int count, const FVM_Value *interfaces)
{
    if (!c || fvm_validate_value(c, constructor) != FVM_OK || count > FVM_EXEC_MAX_INTERFACE_ARGUMENTS ||
        (count && !interfaces))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (unsigned int i = 0; i < count; ++i)
    {
        if (fvm_validate_value(c, interfaces + i) != FVM_OK)
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    /* Version/primitive/zero-count noops need validation, but no captured graph or allocation. */
    if (c->swf_loaded && (!count || c->swf_info.version < FVM_SWF_VERSION_7 || constructor->type != FVM_VALUE_OBJECT))
    {
        return FVM_OK;
    }
    try
    {


        FVM_ExecutionWorkScope work(c, c->swf_loaded);
        FVM_Result status = work.step();
        if (status != FVM_OK)
        {
            return status;
        }


        FVM_ExecutionRoots roots(c, (size_t)count + FVM_IMPLEMENTS_ARGUMENT_BASE);
        roots.values[FVM_IMPLEMENTS_CLASS_ROOT] = *constructor;
        for (unsigned int i = 0; i < count; ++i)
        {
            status = work.step();
            if (status != FVM_OK)
            {
                return status;
            }
            roots.values[FVM_IMPLEMENTS_ARGUMENT_BASE + i] = interfaces[i];
        }
        if (!c->swf_loaded)
        {
            return constructor->type == FVM_VALUE_OBJECT
                       ? fvm_class_implements(c,
                                              constructor->object,
                                              count,
                                              count ? roots.values.begin() + FVM_IMPLEMENTS_ARGUMENT_BASE : 0)
                       : FVM_INVALID_ARGUMENT;
        }
        fvm_detail::vector<FVM_Object *> snapshots;
        for (unsigned int i = 0; i < count; ++i)
        {
            status = work.step();
            if (status != FVM_OK)
            {
                return status;
            }
            const FVM_Value &input = roots.values[FVM_IMPLEMENTS_ARGUMENT_BASE + i];
            FVM_Value prototype;
            fvm_value_undefined(&prototype);
            if (input.type == FVM_VALUE_OBJECT)
            {
                status = fvm_avm1_stored_prototype(c, input.object, prototype);
                if (status != FVM_OK)
                {
                    return status;
                }
                if (prototype.type == FVM_VALUE_OBJECT)
                {
                    snapshots.push_back(prototype.object);
                }
            }
        }
        FVM_Value target;
        fvm_value_undefined(&target);
        status = fvm_avm1_stored_prototype(c, constructor->object, target);
        if (status != FVM_OK)
        {
            return status;
        }
        if (target.type == FVM_VALUE_OBJECT && !target.object->interfaces_are_prototypes)
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
