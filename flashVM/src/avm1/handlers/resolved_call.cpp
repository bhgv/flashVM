/* FlashVM: named invocation/construction from resolved values for interpreter and generated code.
 */
#include "support.hpp"
#include "resolved_call_constants.h"
#include "script_call.hpp"
#include "../../standard/construction.hpp"
#include "../../core/property_key.hpp"
#include "../../core/call_scope.hpp"
#include "../../core/function_call_constants.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"


/* Dispatch a found binding or existing named fallback without lexical/bytecode dependencies. */
extern "C" FVM_Result fvm_exec_resolved_call(FVM_Context *c,
                                             int construct,
                                             const FVM_Value *name,
                                             const FVM_Value *function,
                                             int found,
                                             FVM_Object *receiver,
                                             unsigned int argc,
                                             const FVM_Value *argv,
                                             FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, name) != FVM_OK || name->type != FVM_VALUE_STRING ||
        fvm_validate_value(c, function) != FVM_OK || (receiver && receiver->owner != c) ||
        argc > (construct ? FVM_EXEC_MAX_CONSTRUCTOR_ARGUMENTS : FVM_EXEC_MAX_METHOD_ARGUMENTS) || (argc && !argv))
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


        FVM_ExecutionRoots roots(c, (size_t)argc + FVM_RESOLVED_ARGUMENT_BASE);
        roots.values[FVM_RESOLVED_NAME_ROOT] = *name;
        roots.values[FVM_RESOLVED_FUNCTION_ROOT] = *function;
        if (receiver)
        {
            roots.values[FVM_RESOLVED_RECEIVER_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_RESOLVED_RECEIVER_ROOT].object = receiver;
        }
        for (unsigned int i = 0; i < argc; ++i)
        {
            roots.values[FVM_RESOLVED_ARGUMENT_BASE + i] = argv[i];
        }
        const FVM_Value *args = argc ? roots.values.begin() + FVM_RESOLVED_ARGUMENT_BASE : 0;
        const FVM_Value &callable = roots.values[FVM_RESOLVED_FUNCTION_ROOT];
        const FVM_Value &key_value = roots.values[FVM_RESOLVED_NAME_ROOT];
        fvm_detail::string key;
        FVM_Result status = fvm_property_key(key_value.string, fvm_string_size(key_value), key);
        if (status != FVM_OK)
        {
            return status;
        }
        FVM_Value result;
        fvm_value_undefined(&result);
        if (construct)
        {
            FVM_Object *instance = 0;
            if (found)
            {
                status = callable.type == FVM_VALUE_OBJECT
                             ? fvm_function_construct_value(c, callable.object, argc, args, &result)
                             : FVM_UNSUPPORTED;
            }
            else
            {
                status = fvm_construct_bytes(c, key.data(), key.size(), argc, args, &instance);
            }
            if (!found && status == FVM_OK)
            {
                result.type = FVM_VALUE_OBJECT;
                result.object = instance;
            }
        }
        else if (callable.type == FVM_VALUE_OBJECT && callable.object->invocable.callback)
        {
            if (c->swf_loaded)
            {


                /* [GC:] Qualified lookup owners remain in caller roots; function-value
                 * invocation has its own identity and explicit depth zero. */
                FVM_CallScope call(c, 0, callable.object, 0, FVM_FUNCTION_CALL_SUPER_DEPTH);
                status = fvm_function_invoke(c, callable.object, receiver, argc, args, &result);
            }
            else
            {
                status = fvm_function_invoke(c, callable.object, receiver, argc, args, &result);
            }
        }
        else if (found)
        {
            status = FVM_UNSUPPORTED;
        }
        else
        {
            fvm_detail::map<fvm_detail::string, FVM_ScriptFunction>::const_iterator script =
                c->script_functions.find(key);
            if (script != c->script_functions.end())
            {
                status = fvm_avm1_call_script(c, script->first, script->second, argc, args, c->global, &result);
            }
            else
            {
                status = fvm_call_bytes(c, key.data(), key.size(), argc, args, &result);
            }
        }
        if (status == FVM_OK)
        {
            *out = result;
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
