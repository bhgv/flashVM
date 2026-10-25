/* FlashVM: value-based Extends with ordered boxing and rooted prototype accessors. */
#include "support.hpp"
#include "extends_constants.h"
#include "class_prototype_constants.h"
#include "../../standard/inheritance.hpp"
#include "../../standard/base_state.hpp"


/* Box superclass then subclass; define fresh own metadata before assigning subclass.prototype. */
extern "C" FVM_Result fvm_exec_extends(FVM_Context *c, const FVM_Value *subclass, const FVM_Value *superclass)
{
    if (!c || c->destroying || c->gc_releasing || fvm_validate_value(c, subclass) != FVM_OK ||
        fvm_validate_value(c, superclass) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!c->swf_loaded)
    {
        return subclass->type == FVM_VALUE_OBJECT && superclass->type == FVM_VALUE_OBJECT
                   ? fvm_class_extend(c, subclass->object, superclass->object)
                   : FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Keep both inputs, boxing results and detached property values alive through
         * callbacks. */
        FVM_ExecutionRoots roots(c, FVM_EXTENDS_ROOT_COUNT);
        roots.values[FVM_EXTENDS_SUBCLASS_ROOT] = *subclass;
        roots.values[FVM_EXTENDS_SUPERCLASS_ROOT] = *superclass;
        const unsigned int inputs[] = {FVM_EXTENDS_SUPERCLASS_ROOT, FVM_EXTENDS_SUBCLASS_ROOT};
        const unsigned int boxes[] = {FVM_EXTENDS_BOXED_SUPER_ROOT, FVM_EXTENDS_BOXED_SUB_ROOT};
        for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i)
        {
            FVM_Value &box = roots.values[boxes[i]];
            FVM_Result status = fvm_exec_to_object(c, &roots.values[inputs[i]], &box);
            if (status != FVM_OK)
            {
                return status;
            }
            if (box.type != FVM_VALUE_OBJECT)
            {
                FVM_Object *bare = 0;
                status = fvm_object_create(c, &bare);
                if (status != FVM_OK)
                {
                    return status;
                }
                box.type = FVM_VALUE_OBJECT;
                box.object = bare;
            }
        }
        const FVM_Value &super = roots.values[FVM_EXTENDS_BOXED_SUPER_ROOT];
        FVM_Value &parent = roots.values[FVM_EXTENDS_PARENT_ROOT];
        FVM_Result status = fvm_object_get(c, super.object, FVM_CLASS_PROTOTYPE_NAME, &parent);
        if (status != FVM_OK)
        {
            return status;
        }
        FVM_Object *child = 0;
        status = fvm_object_create(c, &child);
        if (status != FVM_OK)
        {
            return status;
        }
        roots.values[FVM_EXTENDS_CHILD_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_EXTENDS_CHILD_ROOT].object = child;
        child->resource_kind = FVM_RESOURCE_OBJECT;
        status = fvm_base_store_data(c, child, FVM_EXTENDS_RAW_PARENT_NAME, parent, false);
        if (status != FVM_OK)
        {
            return status;
        }
        child->prototype = parent.type == FVM_VALUE_OBJECT ? parent.object : 0;
        child->attributes[FVM_EXTENDS_RAW_PARENT_NAME] = FVM_PROPERTY_BUILTIN_FLAGS;
        const char *names[] = {FVM_EXTENDS_CONSTRUCTOR_NAME, FVM_EXTENDS_SUPER_CONSTRUCTOR_NAME};
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        { /* Fresh own data must not dispatch inherited constructor setters or watchers. */
            status = fvm_base_store_data(c, child, names[i], super, false);
            if (status != FVM_OK)
            {
                return status;
            }
            child->attributes[names[i]] = FVM_PROPERTY_DONT_ENUM;
        }
        return fvm_object_set(c,
                              roots.values[FVM_EXTENDS_BOXED_SUB_ROOT].object,
                              FVM_CLASS_PROTOTYPE_NAME,
                              &roots.values[FVM_EXTENDS_CHILD_ROOT]);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
