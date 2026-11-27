/* Copyright (c) 2026 bhgv
 * EN: Share the ActionScript UniBus descriptor and byte codecs between both roles.
 * RU: Общий кодек описателей и массивов байтов UniBus для обеих ролей ActionScript.
 */
#include "session_message_types.h"
#include "session_methods.h"
#include "flashvm/execution.h"
#include <stdlib.h>


/* [GC:] EN/RU: See session_message_types.h. / См. session_message_types.h. */
FVM_Result fvm_styxrpc_message_descriptor(FVM_Context *context, const FVM_Value *value,
    ScriptMessageNames *names, StyxRpcMessageHandler receive, void *cookie, StyxRpcMessageType *type)
{
    FVM_Value fields[4];
    FVM_GCRootScope *roots;
    uint64_t count, version, tag;
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    status = fvm_styxrpc_fields(context, roots, value, fields, 4, &count);
    if (status == FVM_OK &&
        ((count != 3 && count != 4) || !fvm_styxrpc_name(fields, names->type, sizeof(names->type)) ||
         !fvm_styxrpc_number(fields + 1, UINT32_MAX, &version) || !version ||
         !fvm_styxrpc_number(fields + 2, UINT32_MAX, &tag) ||
         (count == 4 && !fvm_styxrpc_name(fields + 3, names->method, sizeof(names->method))) ||
         (count == 3 && !tag)))
    {
        status = FVM_INVALID_ARGUMENT;
    }
    if (status == FVM_OK)
    {
        type->name = names->type;
        type->version = (uint32_t)version;
        type->send_tag = (uint32_t)tag;
        type->receive = count == 4 ? receive : NULL;
        type->context = cookie;
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* [GC:] EN/RU: See session_message_types.h. / См. session_message_types.h. */
FVM_Result fvm_styxrpc_message_queue(FVM_Context *context, StyxRpcMessagePort **port,
    unsigned argc, const FVM_Value *args, FVM_Value *out)
{
    char name[STYXRPC_MESSAGE_NAME_MAX + 1];
    unsigned char bytes[STYXRPC_MESSAGE_PAYLOAD_MAX];
    uint64_t version;
    size_t size;
    FVM_Result status;
    if (!*port || argc != 3 || !fvm_styxrpc_name(args, name, sizeof(name)) ||
        !fvm_styxrpc_number(args + 1, UINT32_MAX, &version) || !version)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_styxrpc_bytes(context, args + 2, bytes, sizeof(bytes), &size);
    if (status == FVM_OK)
    {
        if (!*port)
        {
            return FVM_INVALID_ARGUMENT;
        }
        fvm_value_number(out, styxrpc_message_send(*port, name, (uint32_t)version, bytes, size));
    }
    return status;
}


/* [GC:] EN/RU: See session_message_types.h. / См. session_message_types.h. */
FVM_Result fvm_styxrpc_message_dispatch(FVM_Context *context, const FVM_Value *receiver,
    const char *method, const unsigned char *bytes, size_t size, const FVM_Value *reply)
{
    FVM_Value args[2], name, result;
    FVM_Value *values = size ? malloc(size * sizeof(*values)) : NULL;
    FVM_Result status;
    size_t i;
    if (size && !values)
    {
        return FVM_OUT_OF_MEMORY;
    }
    for (i = 0; i < size; ++i)
    {
        fvm_value_number(values + i, bytes[i]);
    }
    status = fvm_exec_literal_array(context, (unsigned)size, values, args);
    free(values);
    if (status == FVM_OK)
    {
        if (reply)
        {
            args[1] = *reply;
        }
        fvm_value_string(&name, method);
        status = fvm_exec_call_method(context, 0, receiver, &name, NULL, reply ? 2 : 1, args, &result);
    }
    return status;
}
