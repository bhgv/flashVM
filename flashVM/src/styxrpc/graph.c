/* Copyright (c) 2026 bhgv
 * EN: Synchronous FlashVM graph adapter over the independent positional RPC codec.
 * RU: Синхронный адаптер графов FlashVM поверх независимого позиционного RPC.
 */
#include "module_import.h"
#include "wire_wait.h"
#include "object_handlers.h"
#include "flashvm/styxrpc_graph.h"
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include "styxrpc/graph_client.h"
#include "graph_refs.h"
#include "managed_values.h"
#include <stdlib.h>


/* EN: Allocate the bounded shared identity table independently for one invocation.
 * RU: Выделить ограниченную общую таблицу идентификаторов на один вызов. */
int fvm_styxrpc_graph_init(FVM_StyxRefs *refs)
{
    refs->definitions = calloc(STYXRPC_GRAPH_KEY_LIMIT, sizeof(*refs->definitions));
    refs->capacity = STYXRPC_GRAPH_KEY_LIMIT;
    refs->graph = 1;
    return refs->definitions != NULL;
}


/* EN: Release uncommitted staged tables or retired contents after the commit.
 * RU: Освободить подготовленные таблицы при отказе или старые данные после записи. */
void fvm_styxrpc_graph_clear(FVM_StyxRefs *refs)
{
    unsigned i;
    for (i = 0; i < refs->count; ++i)
    {
        styxrpc_hybrid_close(refs->definitions[i].table_stage);
    }
    free(refs->definitions);
}


/* [GC:] EN: Decode result identities before applying any checked sparse updates.
 * RU: Декодировать идентификаторы результата перед применением разреженных изменений. */
int fvm_styxrpc_graph_result(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    const StyxRpcSchema *schema, const FVM_SRecord *arguments, const StyxRpcGraphReply *reply,
    FVM_Value *out)
{
    unsigned originals = refs->count;
    unsigned count;
    FVM_Value value;
    if (reply->result.tag == STYXRPC_VALUE_NIL)
    {
        fvm_value_undefined(&value);
    }
    else if (!fvm_styxrpc_values_decode(context, roots, refs, schema->result, &reply->result,
        &value, 1, &count) || count != 1)
    {
        return 0;
    }
    if (!fvm_styxrpc_graph_stage(context, roots, refs, schema, arguments, originals, &reply->updates) ||
        !fvm_styxrpc_refs_commit(context, refs))
    {
        return 0;
    }
    *out = value;
    return 1;
}


/* [GC:] EN: Encode once, exchange one graph, then validate and commit under roots.
 * RU: Однократно кодировать и обменять граф, затем проверить и применить под GC-корнями. */
FVM_Result fvm_styxrpc_graph_at(FVM_Context *context, struct StyxFid *directory, const StyxRpcManagedTarget *target,
    unsigned argc, const FVM_Value *argv, FVM_Value *out, StyxRpcServer *reverse)
{
    FVM_StyxRefs refs = {0};
    FVM_GCRootScope *roots;
    FVM_SRecord arguments;
    StyxRpcGraphReply reply;
    unsigned char *buffer;
    const char *error = NULL;
    FVM_Result status;
    if (!fvm_styxrpc_graph_supported(&target->schema))
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC graph schema is unsupported");
    }
    buffer = malloc(2U * STYXRPC_MANAGED_LIMIT);
    if (!buffer || !fvm_styxrpc_graph_init(&refs))
    {
        free(buffer);
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        arguments.tag = STYXRPC_GRAPH_REQUEST_TAG;
        arguments.payload = buffer;
        arguments.encoded_size = 0;
        if (!fvm_styxrpc_values_encode(context, roots, &refs, target->schema.arguments, argv, argc,
            buffer, STYXRPC_MANAGED_LIMIT, &arguments.size))
        {
            error = "Styx RPC graph arguments do not fit the schema";
        }
        else
        {
            FVM_StyxWireCall call = {0};
            call.kind = FVM_STYX_WIRE_GRAPH;
            call.directory = directory;
            call.target = target;
            call.arguments = buffer;
            call.size = arguments.size;
            call.reply = buffer + STYXRPC_MANAGED_LIMIT;
            call.capacity = STYXRPC_MANAGED_LIMIT;
            call.result = &reply;
            error = fvm_styxrpc_call_error(fvm_styxrpc_wire_call(context, reverse, &call));
            if (!error && !fvm_styxrpc_graph_result(context, roots, &refs, &target->schema, &arguments, &reply, out))
            {
                error = "Styx RPC graph result could not be committed";
            }
        }
        fvm_gc_roots_end(roots);
    }
    fvm_styxrpc_graph_clear(&refs);
    free(buffer);
    return error ? fvm_styxrpc_managed_fail(context, error) : status;
}


/* [GC:] EN: Invoke a registered global using its literal mounted target path.
 * RU: Вызвать зарегистрированную глобальную функцию по её смонтированному пути. */
FVM_Result fvm_styxrpc_invoke_graph(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *argv, FVM_Value *out, void *userdata)
{
    (void)self;
    return fvm_styxrpc_graph_at(context, NULL, userdata, argc, argv, out, NULL);
}


/* EN/RU: See flashvm/styxrpc_graph.h. / См. flashvm/styxrpc_graph.h. */
FVM_Result fvm_styxrpc_import_graph(FVM_Context *context, const char *global, StyxRpcManagedTarget *target)
{
    unsigned char profile[STYXRPC_MANAGED_PROFILE_BYTES];
    size_t size;
    if (!context || !global || !*global || !target || !target->session || !target->module ||
        !*target->module ||
        !target->wait_ms || target->wait_ms > STYXRPC_SCALAR_WAIT_MAX_MS ||
        !fvm_styxrpc_graph_supported(&target->schema) ||
        styxrpc_graph_profile(target->name, target->signature, &target->schema, profile,
            sizeof(profile), &size) != STYXRPC_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_register_function(context, global, fvm_styxrpc_invoke_graph, target);
}


/* [GC:] [ASINC] EN: Execute an AS export, using one identity table for arguments, result and updates.
 * RU: Выполнить экспорт AS с одной таблицей идентификаторов для аргументов, результата и обновлений. */
StyxRpcStatus fvm_styxrpc_run_graph(FVM_StyxRpcManagedExport *binding, FVM_Object *receiver,
    const FVM_SRecord *arguments, StyxRpcGraphOutput *output)
{
    FVM_Context *context = binding->context;
    FVM_GCRootScope *roots;
    FVM_StyxRefs refs = {0};
    FVM_Value values[FVM_STYXRPC_LIST_ITEMS], result;
    unsigned count, originals;
    int ok;
    if (fvm_exception_get(context, &result) != FVM_NOT_FOUND ||
        fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    ok = fvm_styxrpc_graph_init(&refs) && fvm_styxrpc_values_decode(context, roots, &refs, binding->schema.arguments,
        arguments, values, FVM_STYXRPC_LIST_ITEMS, &count);
    originals = refs.count;
    ok = ok && (receiver ? fvm_object_call(context, receiver, binding->function, count, values, &result) :
            fvm_avm1_call(context, binding->function, NULL, count, values, &result)) == FVM_OK &&
        fvm_gc_roots_add(roots, &result) == FVM_OK &&
        fvm_styxrpc_values_encode(context, roots, &refs, binding->schema.result, &result,
            binding->schema.result[1] == ')' ? 0 : 1, binding->result_buffer,
            binding->result_capacity, &output->result_size);
    if (ok)
    {
        output->result = binding->result_buffer;
        output->updates = binding->result_buffer + output->result_size;
        ok = fvm_styxrpc_graph_updates(context, roots, &refs, &binding->schema, arguments, originals,
            binding->result_buffer + output->result_size, binding->result_capacity - output->result_size,
            &output->update_size);
    }
    fvm_gc_roots_end(roots);
    fvm_styxrpc_graph_clear(&refs);
    if (!ok)
    {
        fvm_exception_clear(context);
        return STYXRPC_REMOTE_ERROR;
    }
    return STYXRPC_OK;
}


/* [ASINC] EN: Adapt the ordinary global binding to the shared export callback.
 * RU: Передать глобальную привязку общему экспортному колбэку. */
static StyxRpcStatus begin_graph(void *state, const FVM_SRecord *arguments, StyxRpcGraphOutput *output)
{
    return fvm_styxrpc_run_graph(state, NULL, arguments, output);
}


/* EN/RU: See flashvm/styxrpc_graph.h. / См. flashvm/styxrpc_graph.h. */
FVM_Result fvm_styxrpc_export_graph(FVM_StyxRpcManagedExport *binding, const char *name,
    uint32_t signature, StyxRpcGraphExport *exported)
{
    StyxRpcGraphExport candidate;
    unsigned char profile[STYXRPC_MANAGED_PROFILE_BYTES];
    size_t size;
    if (!binding || !binding->context || !binding->function || !*binding->function || !exported ||
        !binding->result_buffer || !binding->result_capacity || binding->result_capacity > STYXRPC_MANAGED_LIMIT ||
        !fvm_styxrpc_graph_supported(&binding->schema))
    {
        return FVM_INVALID_ARGUMENT;
    }
    candidate.name = name;
    candidate.signature = signature;
    candidate.schema = binding->schema;
    candidate.begin = begin_graph;
    candidate.context = binding;
    if (styxrpc_graph_profile(name, signature, &candidate.schema, profile, sizeof(profile), &size) != STYXRPC_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *exported = candidate;
    return FVM_OK;
}
