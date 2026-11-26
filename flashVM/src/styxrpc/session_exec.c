/* Copyright (c) 2026 bhgv
 * EN: Invoke a peer's executable VFS node with a copied AS byte array.
 * RU: Вызвать исполняемый узел другой стороны со снимком массива байтов AS.
 */
#include "session_methods.h"
#include "managed_values.h"
#include "flashvm/execution.h"
#include "styxrpc/nodes.h"
#include <stdlib.h>


/* [ASINC] [GC:] EN: Resolve args against this binding and return a new byte Array.
 * The optional input is borrowed; the request runs synchronously on the movie owner.
 * RU: Разрешить args в этой сессии и вернуть новый Array. Вход заимствуется.
 * Вызов блокирует поток владельца до ответа или ошибки транспорта. */
static FVM_Result exec_node(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    const FVM_StyxRpcSession *binding = state;
    char path[FVM_STYXRPC_ABSOLUTE_PATH];
    unsigned char *bytes = NULL;
    FVM_Value *values;
    FVM_Result status;
    StyxRpcIoOutcome outcome;
    size_t size = 0;
    size_t used, i;
    (void)self;
    if (!argc || argc > 2 || !fvm_styxrpc_resolve_path(binding, args, path) || !path[1])
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (argc == 2)
    {
        status = fvm_styxrpc_alloc_bytes(context, args + 1, STYXRPC_NODE_BUFFER_MAX, &bytes, &size);
        if (status != FVM_OK)
        {
            return status;
        }
    }
    values = size ? malloc(size * sizeof(*values)) : NULL;
    if (size && !values)
    {
        free(bytes);
        return FVM_OUT_OF_MEMORY;
    }
    outcome = styxrpc_node_exec(binding->client, path + 1, bytes, size, &used);
    if (outcome == STYXRPC_IO_OK)
    {
        for (i = 0; i < used; ++i)
        {
            fvm_value_number(values + i, bytes[i]);
        }
        status = fvm_exec_literal_array(context, (unsigned)used, values, result);
    }
    else
    {
        const char *message = outcome == STYXRPC_IO_UNCERTAIN ? "VFS execute outcome is uncertain" :
            outcome == STYXRPC_IO_NOT_OPEN ? "VFS execute service is unavailable" : "VFS execute failed";
        status = fvm_styxrpc_managed_fail(context, message);
    }
    free(values);
    free(bytes);
    return status;
}


/* EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_exec_member(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding)
{
    return fvm_object_define_method_rooted(context, object, "exec", exec_node, binding, NULL);
}
