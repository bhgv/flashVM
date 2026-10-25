/* FlashVM: named construction over explicit scopes and values, independent of bytecode. */
#include "support.hpp"
#include "new_object_constants.h"


/* Loaded NewObject resolves a literal scope name; host mode retains qualified lookup. */
extern "C" FVM_Result fvm_exec_new_object(FVM_Context *c,
                                          FVM_Object *scope,
                                          FVM_Object *target,
                                          FVM_Object *receiver,
                                          int local_scope,
                                          const FVM_Value *name,
                                          unsigned int argc,
                                          const FVM_Value *argv,
                                          FVM_Value *out)
{
    if (!c || !out || (scope && scope->owner != c) || (target && target->owner != c) ||
        (receiver && receiver->owner != c) || fvm_validate_value(c, name) != FVM_OK || name->type != FVM_VALUE_STRING ||
        argc > FVM_EXEC_MAX_CONSTRUCTOR_ARGUMENTS || (argc && !argv))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (unsigned int i = 0; i < argc; ++i)
    {
        if (fvm_validate_value(c, argv + i) != FVM_OK)
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    try
    {


        /* [GC:] Copy all caller inputs before lookup can invoke a getter or collect. */
        FVM_ExecutionRoots roots(c, (size_t)argc + FVM_NEW_OBJECT_ARGUMENT_BASE);
        FVM_Object *objects[] = {scope, target, receiver};
        const unsigned int slots[] = {FVM_NEW_OBJECT_SCOPE_ROOT, FVM_NEW_OBJECT_TARGET_ROOT, FVM_NEW_OBJECT_THIS_ROOT};
        for (size_t i = 0; i < sizeof(objects) / sizeof(objects[0]); ++i)
        {
            if (objects[i])
            {
                roots.values[slots[i]].type = FVM_VALUE_OBJECT;
                roots.values[slots[i]].object = objects[i];
            }
        }
        roots.values[FVM_NEW_OBJECT_NAME_ROOT] = *name;
        for (unsigned int i = 0; i < argc; ++i)
        {
            roots.values[FVM_NEW_OBJECT_ARGUMENT_BASE + i] = argv[i];
        }
        FVM_Value &constructor = roots.values[FVM_NEW_OBJECT_FUNCTION_ROOT];
        const FVM_Value &key = roots.values[FVM_NEW_OBJECT_NAME_ROOT];
        int handled = 0, present = 0;
        FVM_Result status = FVM_OK;
        if (!c->swf_loaded)
        {
            status = fvm_exec_variable_path(c, scope, target, receiver, &key, &constructor, &handled, &present);
        }
        if (status == FVM_OK && !handled)
        {
            FVM_Object *ignored_receiver = 0;
            status = fvm_exec_variable_lookup(
                c, scope, target, receiver, 0, local_scope, &key, &constructor, &ignored_receiver, &present);
        }
        if (status != FVM_OK)
        {
            return status;
        }
        if (c->swf_loaded && (constructor.type != FVM_VALUE_OBJECT ||
                              (!constructor.object->constructable.callback &&
                               (constructor.object->resource_kind != FVM_RESOURCE_SCRIPT_FUNCTION ||
                                !constructor.object->invocable.callback))))
        {
            fvm_value_undefined(out);
            return FVM_OK;
        }
        const FVM_Value *args = argc ? roots.values.begin() + FVM_NEW_OBJECT_ARGUMENT_BASE : 0;
        return fvm_exec_resolved_call(c, 1, &key, &constructor, present, 0, argc, args, out);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
