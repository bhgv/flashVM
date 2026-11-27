/* Copyright (c) 2026 bhgv
 * EN: Connect managed AS values to compiled Dis calls on the context owner thread.
 * RU: Связь управляемых значений AS с вызовами Dis в потоке владельца контекста.
 */
#include "module_import.h"
#include "wire_wait.h"
#include "object_handlers.h"
#include "flashvm/styxrpc_managed.h"
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include "styxrpc/managed_module.h"
#include "managed_values.h"
#include <stdlib.h>
#include <string.h>


/* EN: Raise a local bridge diagnostic, retaining any exception from a getter.
 * RU: Выдать диагностику моста, сохраняя исключение геттера. */
FVM_Result fvm_styxrpc_managed_fail(FVM_Context *context, const char *message)
{
    FVM_Value error;
    FVM_Result status = fvm_exception_get(context, &error);
    if (status != FVM_NOT_FOUND)
    {
        return status == FVM_OK ? FVM_EXCEPTION : status;
    }
    status = fvm_value_string(&error, message);
    return status == FVM_OK ? fvm_exception_raise(context, &error) : status;
}


/* [GC:] EN: Encode checked arguments, perform exactly one call and root its decoded result.
 * RU: Закодировать аргументы, выполнить один вызов и удержать декодированный результат. */
FVM_Result fvm_styxrpc_managed_at(FVM_Context *context, struct StyxFid *directory, const StyxRpcManagedTarget *target,
    unsigned argc, const FVM_Value *argv, FVM_Value *value, StyxRpcServer *reverse)
{
    FVM_GCRootScope *roots;
    unsigned char *buffers;
    unsigned char *reply;
    size_t size;
    unsigned count;
    FVM_StyxRef definitions[FVM_STYXRPC_REF_SLOTS] = {0};
    FVM_StyxRefs refs = {0};
    FVM_Value decoded;
    StyxRpcManagedReply result;
    const char *error = NULL;
    FVM_Result status;
    if (!fvm_styxrpc_values_supported(&target->schema))
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC managed schema is unsupported");
    }
    buffers = malloc((strchr(target->schema.arguments, 'r') ? 3U : 2U) * STYXRPC_MANAGED_LIMIT);
    if (!buffers)
    {
        return FVM_OUT_OF_MEMORY;
    }
    reply = buffers + STYXRPC_MANAGED_LIMIT;
    refs.definitions = definitions;
    refs.capacity = FVM_STYXRPC_REF_SLOTS;
    refs.storage = strchr(target->schema.arguments, 'r') ? reply + STYXRPC_MANAGED_LIMIT : NULL;
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        if (!fvm_styxrpc_values_encode(context, roots, &refs, target->schema.arguments, argv, argc,
            buffers, STYXRPC_MANAGED_LIMIT, &size))
        {
            error = "Styx RPC managed arguments do not fit the schema";
        }
        else
        {
            FVM_StyxWireCall call = {0};
            call.kind = FVM_STYX_WIRE_MANAGED;
            call.directory = directory;
            call.target = target;
            call.arguments = buffers;
            call.size = size;
            call.reply = reply;
            call.capacity = STYXRPC_MANAGED_LIMIT;
            call.result = &result;
            switch (fvm_styxrpc_wire_call(reverse, &call))
            {
            case STYXRPC_CALL_OK:
                if (result.void_result)
                {
                    fvm_value_undefined(&decoded);
                }
                else if (!fvm_styxrpc_values_decode(context, roots, NULL, target->schema.result, &result.result,
                    &decoded, 1, &count) || count != 1)
                {
                    error = "Styx RPC managed result cannot be represented";
                }
                if (!error && (!fvm_styxrpc_refs_stage(context, roots, &refs, &result) ||
                    !fvm_styxrpc_refs_commit(context, &refs)))
                {
                    error = "Styx RPC reference update could not be committed";
                }
                if (!error)
                {
                    *value = decoded;
                }
                break;
            case STYXRPC_CALL_NOT_SENT:
                error = "Styx RPC call was not sent";
                break;
            case STYXRPC_CALL_REMOTE_ERROR:
                error = "Styx RPC remote function failed";
                break;
            default:
                error = "Styx RPC outcome is uncertain";
                break;
            }
        }
        fvm_gc_roots_end(roots);
    }
    free(buffers);
    return error ? fvm_styxrpc_managed_fail(context, error) : status;
}


/* [GC:] EN: Invoke a registered global using its literal mounted target path.
 * RU: Вызвать зарегистрированную глобальную функцию по её смонтированному пути. */
FVM_Result fvm_styxrpc_invoke_managed(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *argv, FVM_Value *value, void *state)
{
    (void)self;
    return fvm_styxrpc_managed_at(context, NULL, state, argc, argv, value, NULL);
}


/* EN/RU: See flashvm/styxrpc_managed.h. / См. flashvm/styxrpc_managed.h. */
FVM_Result fvm_styxrpc_import_managed(FVM_Context *context, const char *global, StyxRpcManagedTarget *target)
{
    if (!context || !global || !*global || !target || !target->session || !target->module ||
        !*target->module || !target->name || !*target->name || !target->signature ||
        !target->wait_ms || target->wait_ms > STYXRPC_SCALAR_WAIT_MAX_MS ||
        !fvm_styxrpc_values_supported(&target->schema))
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_register_function(context, global, fvm_styxrpc_invoke_managed, target);
}


/* [GC:] [ASINC] EN: Run a named AS export and stage its rooted result and ref changes.
 * RU: Вызвать экспорт AS и подготовить результат и изменения ref под корнями. */
StyxRpcStatus fvm_styxrpc_run_managed(FVM_StyxRpcManagedExport *binding, FVM_Object *receiver,
    const FVM_SRecord *arguments, StyxRpcOutput *output)
{
    FVM_Context *context = binding->context;
    FVM_GCRootScope *roots;
    FVM_Value values[FVM_STYXRPC_LIST_ITEMS], result;
    FVM_StyxRef definitions[FVM_STYXRPC_REF_SLOTS] = {0};
    FVM_StyxRefs refs = {0};
    unsigned count;
    int ok;
    if (fvm_exception_get(context, &result) != FVM_NOT_FOUND ||
        fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    refs.definitions = definitions;
    refs.capacity = FVM_STYXRPC_REF_SLOTS;
    ok = fvm_styxrpc_values_decode(context, roots, &refs, binding->schema.arguments, arguments,
        values, FVM_STYXRPC_LIST_ITEMS, &count) &&
        (receiver ? fvm_object_call(context, receiver, binding->function, count, values, &result) :
            fvm_avm1_call(context, binding->function, NULL, count, values, &result)) == FVM_OK &&
        fvm_gc_roots_add(roots, &result) == FVM_OK &&
        fvm_styxrpc_values_encode(context, roots, NULL, binding->schema.result, &result,
            binding->schema.result[1] == ')' ? 0 : 1, binding->result_buffer,
            binding->result_capacity, &output->result_size) &&
        fvm_styxrpc_refs_reply(context, roots, &refs, binding->result_buffer + output->result_size,
            binding->result_capacity - output->result_size, output);
    fvm_gc_roots_end(roots);
    if (!ok)
    {
        fvm_exception_clear(context);
        return STYXRPC_REMOTE_ERROR;
    }
    output->result = binding->result_buffer;
    return STYXRPC_OK;
}


/* [ASINC] EN: Adapt the ordinary global binding to the shared export callback.
 * RU: Передать глобальную привязку общему экспортному колбэку. */
static StyxRpcStatus begin_managed(void *state, const FVM_SRecord *arguments, StyxRpcOutput *output)
{
    return fvm_styxrpc_run_managed(state, NULL, arguments, output);
}


/* EN/RU: See flashvm/styxrpc_managed.h. / См. flashvm/styxrpc_managed.h. */
FVM_Result fvm_styxrpc_export_managed(FVM_StyxRpcManagedExport *binding, const char *name,
    uint32_t signature, StyxRpcManagedExport *exported)
{
    StyxRpcManagedExport candidate;
    unsigned char profile[STYXRPC_MANAGED_PROFILE_BYTES];
    size_t size;
    if (!binding || !binding->context || !binding->function || !*binding->function || !exported ||
        !binding->result_buffer || !binding->result_capacity || binding->result_capacity > STYXRPC_MANAGED_LIMIT ||
        !fvm_styxrpc_values_supported(&binding->schema))
    {
        return FVM_INVALID_ARGUMENT;
    }
    candidate.name = name;
    candidate.signature = signature;
    candidate.schema = binding->schema;
    candidate.begin = begin_managed;
    candidate.cancel = NULL;
    candidate.context = binding;
    if (styxrpc_managed_profile(&candidate, profile, sizeof(profile), &size) != STYXRPC_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *exported = candidate;
    return FVM_OK;
}
