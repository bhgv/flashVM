/* FlashVM: value-based InstanceOf with raw class prototype reads and rooted coercions. */
#include "support.hpp"
#include "instance_of_constants.h"
#include "../scope_keywords.hpp"
#include "flashvm/standard.h"
/* Read only stored own prototype data; accessors and inherited fields do not participate. */
static bool stored_prototype(FVM_Context *c, FVM_Object *object, FVM_Value &out)
{
    fvm_detail::map<fvm_detail::string, FVM_Value>::const_iterator i =
        object->properties.find(FVM_INSTANCE_OF_PROTOTYPE_NAME);
    if (i != object->properties.end())
    {
        out = i->second;
        return true;
    }
    if (c->swf_info.version < FVM_SWF_VERSION_7)
        for (i = object->properties.begin(); i != object->properties.end(); ++i)
            if (fvm_avm1_keyword_name(i->first, FVM_INSTANCE_OF_PROTOTYPE_NAME,
                                      FVM_INSTANCE_OF_PROTOTYPE_UNITS, false))
            {
                out = i->second;
                return true;
            }
    return false;
}
/* Test loaded prototype identity; host calls retain the existing strict class API policy. */
extern "C" FVM_Result fvm_exec_instance_of(FVM_Context *c, const FVM_Value *value,
                                           const FVM_Value *constructor, FVM_Value *out)
{
    if (!c || !out || c->destroying || c->gc_releasing || fvm_validate_value(c, value) != FVM_OK ||
        fvm_validate_value(c, constructor) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_INSTANCE_OF_ROOT_COUNT);
        roots.values[FVM_INSTANCE_OF_VALUE_ROOT] = *value;
        roots.values[FVM_INSTANCE_OF_CLASS_ROOT] = *constructor;
        if (!c->swf_loaded)
        {
            int match = 0;
            FVM_Result status =
                fvm_value_instance_of(c, &roots.values[FVM_INSTANCE_OF_VALUE_ROOT],
                                      &roots.values[FVM_INSTANCE_OF_CLASS_ROOT], &match);
            if (status == FVM_OK)
                fvm_value_boolean(out, match);
            return status;
        }
        if (value->type != FVM_VALUE_OBJECT)
        {
            fvm_value_boolean(out, 0);
            return FVM_OK;
        }
        FVM_Value &klass = roots.values[FVM_INSTANCE_OF_BOXED_CLASS_ROOT];
        FVM_Result status =
            fvm_exec_to_object(c, &roots.values[FVM_INSTANCE_OF_CLASS_ROOT], &klass);
        if (status != FVM_OK)
            return status;
        FVM_Value &prototype = roots.values[FVM_INSTANCE_OF_PROTOTYPE_ROOT];
        if (klass.type != FVM_VALUE_OBJECT || !stored_prototype(c, klass.object, prototype))
        {
            fvm_value_boolean(out, 0);
            return FVM_OK;
        }
        status = fvm_exec_to_object(c, &prototype, &prototype);
        if (status != FVM_OK)
            return status;
        if (prototype.type != FVM_VALUE_OBJECT)
        {
            fvm_value_boolean(out, 0);
            return FVM_OK;
        }
        /* [GC:] Coercions are complete. Raw graph reads below invoke no getters or callbacks;
         * the original object/class/prototype roots retain their reachable interface graphs. */
        FVM_Object *target = prototype.object;
        fvm_detail::vector<FVM_Object *> pending, visited;
        unsigned int depth = 0;
        for (FVM_Object *node = roots.values[FVM_INSTANCE_OF_VALUE_ROOT].object->prototype; node;
             node = node->prototype)
        {
            if (++depth > FVM_PROTOTYPE_MAX_DEPTH)
                return FVM_ERROR;
            pending.push_back(node);
            while (!pending.empty())
            {
                FVM_Object *current = pending.back();
                pending.pop_back();
                bool seen = false;
                for (size_t i = 0; i < visited.size(); ++i)
                    if (visited[i] == current)
                    {
                        seen = true;
                        break;
                    }
                if (seen)
                    continue;
                if (visited.size() >= FVM_INSTANCE_OF_MAX_INTERFACE_NODES)
                    return FVM_UNSUPPORTED;
                visited.push_back(current);
                if (current == target)
                {
                    fvm_value_boolean(out, 1);
                    return FVM_OK;
                }
                /* Existing transport metadata stores interface constructors. Adapt their own
                 * prototype fields without invoking accessors or walking interface prototypes. */
                for (size_t i = 0; i < current->implemented_interfaces.size(); ++i)
                {
                    FVM_Value interface_prototype;
                    fvm_value_undefined(&interface_prototype);
                    if (stored_prototype(c, current->implemented_interfaces[i],
                                         interface_prototype) &&
                        interface_prototype.type == FVM_VALUE_OBJECT)
                        pending.push_back(interface_prototype.object);
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
