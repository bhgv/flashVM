/* FlashVM: scoped named function lookup and invocation shared by interpreted/native bodies. */
#include "support.hpp"
#include "call_function_constants.h"
#include "variable_path.hpp"
#include "../../core/call_scope.hpp"


/* Resolve an already-coerced name, retaining qualified owners and ordinary With/default receivers.
 */
extern "C" FVM_Result fvm_exec_call_function(FVM_Context *c,
                                             FVM_Object *scope,
                                             FVM_Object *target,
                                             FVM_Object *receiver,
                                             FVM_Object *default_receiver,
                                             int local_scope,
                                             const FVM_Value *name,
                                             unsigned int argc,
                                             const FVM_Value *argv,
                                             FVM_Value *out)
{
    if (!c || !out || (scope && scope->owner != c) || (target && target->owner != c) ||
        (receiver && receiver->owner != c) || (default_receiver && default_receiver->owner != c) ||
        fvm_validate_value(c, name) != FVM_OK || name->type != FVM_VALUE_STRING ||
        argc > FVM_EXEC_MAX_METHOD_ARGUMENTS || (argc && !argv))
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


        FVM_ExecutionRoots roots(c, (size_t)argc + FVM_CALL_FUNCTION_ARGUMENT_BASE);
        FVM_Object *objects[] = {scope, target, receiver, default_receiver};
        const unsigned int slots[] = {FVM_CALL_FUNCTION_SCOPE_ROOT,
                                      FVM_CALL_FUNCTION_TARGET_ROOT,
                                      FVM_CALL_FUNCTION_THIS_ROOT,
                                      FVM_CALL_FUNCTION_DEFAULT_ROOT};
        for (size_t i = 0; i < sizeof(objects) / sizeof(objects[0]); ++i)
        {
            if (objects[i])
            {
                roots.values[slots[i]].type = FVM_VALUE_OBJECT;
                roots.values[slots[i]].object = objects[i];
            }
        }
        roots.values[FVM_CALL_FUNCTION_NAME_ROOT] = *name;
        for (unsigned int i = 0; i < argc; ++i)
        {
            roots.values[FVM_CALL_FUNCTION_ARGUMENT_BASE + i] = argv[i];
        }
        FVM_Value &callable = roots.values[FVM_CALL_FUNCTION_VALUE_ROOT];
        FVM_Object *selected_receiver = 0, *source_owner = 0;
        int handled = 0, present = 0;
        FVM_Result status = FVM_OK;
        if (c->swf_loaded)
        {
            status = fvm_avm1_read_variable_path(c,
                                                 scope,
                                                 target,
                                                 receiver,
                                                 &roots.values[FVM_CALL_FUNCTION_NAME_ROOT],
                                                 &callable,
                                                 &selected_receiver,
                                                 &source_owner,
                                                 &handled,
                                                 &present);
        }
        if (status == FVM_OK && !handled)
        {
            status = fvm_exec_variable_lookup(c,
                                              scope,
                                              target,
                                              receiver,
                                              default_receiver,
                                              local_scope,
                                              &roots.values[FVM_CALL_FUNCTION_NAME_ROOT],
                                              &callable,
                                              &selected_receiver,
                                              &present);
        }
        if (status != FVM_OK)
        {
            return status;
        }
        if (c->swf_loaded && (callable.type != FVM_VALUE_OBJECT || !callable.object->invocable.callback))
        {
            fvm_value_undefined(out);
            return FVM_OK;
        }
        if (!selected_receiver)
        {
            selected_receiver = default_receiver ? default_receiver : c->global;
        }
        roots.values[FVM_CALL_FUNCTION_RECEIVER_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_CALL_FUNCTION_RECEIVER_ROOT].object = selected_receiver;
        const FVM_Value *args = argc ? roots.values.begin() + FVM_CALL_FUNCTION_ARGUMENT_BASE : 0;
        if (source_owner)
        {
            roots.values[FVM_CALL_FUNCTION_SOURCE_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_CALL_FUNCTION_SOURCE_ROOT].object = source_owner;


            FVM_CallScope method(c, 0, callable.object, source_owner);
            return fvm_exec_resolved_call(c,
                                          0,
                                          &roots.values[FVM_CALL_FUNCTION_NAME_ROOT],
                                          &callable,
                                          present,
                                          selected_receiver,
                                          argc,
                                          args,
                                          out);
        }
        return fvm_exec_resolved_call(
            c, 0, &roots.values[FVM_CALL_FUNCTION_NAME_ROOT], &callable, present, selected_receiver, argc, args, out);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
