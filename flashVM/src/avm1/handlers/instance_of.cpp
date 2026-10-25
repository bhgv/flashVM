/* FlashVM: value-based InstanceOf with raw class prototype reads and rooted coercions. */
#include "support.hpp"
#include "instance_of_constants.h"
#include "instance_of_visit.hpp"
#include "../../core/execution_work_scope.hpp"
#include "class_prototype.hpp"
#include "flashvm/standard.h"


/* Test loaded prototype identity; host calls retain the existing strict class API policy. */
extern "C" FVM_Result
fvm_exec_instance_of(FVM_Context *c, const FVM_Value *value, const FVM_Value *constructor, FVM_Value *out)
{
    if (!c || !out || c->destroying || c->gc_releasing || fvm_validate_value(c, value) != FVM_OK ||
        fvm_validate_value(c, constructor) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        FVM_ExecutionRoots roots(c, FVM_INSTANCE_OF_ROOT_COUNT);
        roots.values[FVM_INSTANCE_OF_VALUE_ROOT] = *value;
        roots.values[FVM_INSTANCE_OF_CLASS_ROOT] = *constructor;


        FVM_ExecutionWorkScope work(c, c->swf_loaded);
        if (!c->swf_loaded)
        {
            int match = 0;
            FVM_Result status = fvm_value_instance_of(
                c, &roots.values[FVM_INSTANCE_OF_VALUE_ROOT], &roots.values[FVM_INSTANCE_OF_CLASS_ROOT], &match);
            if (status == FVM_OK)
            {
                fvm_value_boolean(out, match);
            }
            return status;
        }
        if (value->type != FVM_VALUE_OBJECT)
        {
            fvm_value_boolean(out, 0);
            return FVM_OK;
        }
        FVM_Value &klass = roots.values[FVM_INSTANCE_OF_BOXED_CLASS_ROOT];
        FVM_Result status = fvm_exec_to_object(c, &roots.values[FVM_INSTANCE_OF_CLASS_ROOT], &klass);
        if (status != FVM_OK)
        {
            return status;
        }
        FVM_Value &prototype = roots.values[FVM_INSTANCE_OF_PROTOTYPE_ROOT];
        if (klass.type != FVM_VALUE_OBJECT)
        {
            fvm_value_boolean(out, 0);
            return FVM_OK;
        }
        status = fvm_avm1_stored_prototype(c, klass.object, prototype);
        if (status != FVM_OK)
        {
            return status;
        }
        status = fvm_exec_to_object(c, &prototype, &prototype);
        if (status != FVM_OK)
        {
            return status;
        }
        if (prototype.type != FVM_VALUE_OBJECT)
        {
            fvm_value_boolean(out, 0);
            return FVM_OK;
        }
        /* [GC:] Coercions are complete. Raw graph reads below invoke no getters or callbacks;
         * the original object/class/prototype roots retain their reachable interface graphs. */
        FVM_Object *target = prototype.object;
        fvm_detail::vector<FVM_Object *> pending;
        fvm_detail::map<fvm_instance_of_detail::ObjectKey, bool> visited;
        unsigned int depth = 0;
        for (FVM_Object *node = roots.values[FVM_INSTANCE_OF_VALUE_ROOT].object->prototype; node;
             node = node->prototype)
        {
            status = work.step();
            if (status != FVM_OK)
            {
                return status;
            }
            if (++depth > FVM_PROTOTYPE_MAX_DEPTH)
            {
                return FVM_ERROR;
            }
            pending.push_back(node);
            while (!pending.empty())
            {
                status = work.step();
                if (status != FVM_OK)
                {
                    return status;
                }
                FVM_Object *current = pending.back();
                pending.pop_back();


                fvm_instance_of_detail::ObjectKey key(current);
                if (visited.find(key) != visited.end())
                {
                    continue;
                }
                visited[key] = true;
                if (current == target)
                {
                    fvm_value_boolean(out, 1);
                    return FVM_OK;
                }
                /* Snapshot edges point directly to prototypes; legacy host edges retain
                 * constructors. Neither representation walks ordinary interface prototype ancestry.
                 */
                for (size_t i = 0; i < current->implemented_interfaces.size(); ++i)
                {
                    status = work.step();
                    if (status != FVM_OK)
                    {
                        return status;
                    }
                    FVM_Value interface_prototype;
                    fvm_value_undefined(&interface_prototype);
                    if (current->interfaces_are_prototypes)
                    {
                        pending.push_back(current->implemented_interfaces[i]);
                    }
                    else
                    {
                        status = fvm_avm1_stored_prototype(c, current->implemented_interfaces[i], interface_prototype);
                        if (status != FVM_OK)
                        {
                            return status;
                        }
                        if (interface_prototype.type == FVM_VALUE_OBJECT)
                        {
                            pending.push_back(interface_prototype.object);
                        }
                    }
                }
            }
        }
        fvm_value_boolean(out, 0);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
