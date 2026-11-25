/* Copyright (c) 2026 bhgv
 * EN: Adapt an explicitly selected byte channel to send and receive live Int32 references.
 * RU: Превратить выбранный байтовый канал в канал живых ссылок Int32.
 */
#include "../core/runtime.hpp"
#include "../core/object_root_scope.hpp"
#include "flashvm/styxrpc_session.h"
extern "C"
{
#include "variable_ref_adapter.h"
}

namespace
{
/* [GC:] EN: Encode the existing ref and send one descriptor through the held carrier.
 * RU: Закодировать имеющуюся ссылку и отправить один дескриптор через удерживаемый канал. */
FVM_Result send(FVM_Context *c, FVM_Object *self, unsigned argc, const FVM_Value *args, FVM_Value *out, void *state)
{
    if (!self || self->resource_kind != FVM_RESOURCE_STYXRPC_REF_CHANNEL)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Value bytes;
    FVM_Result status = fvm_styxrpc_int_ref_bytes(c, self, argc, args, &bytes, state);
    return status == FVM_OK ?
        fvm_object_call(c, self->resource_result.object, "send", 1, &bytes, out) : status;
}


/* [GC:] EN: Receive at most one descriptor. Preserve the carrier's empty result.
 * RU: Принять не более одного дескриптора. Сохранить пустой результат канала. */
FVM_Result recv(FVM_Context *c, FVM_Object *self, unsigned argc, const FVM_Value *, FVM_Value *out, void *state)
{
    FVM_StyxRpcSession *session = static_cast<FVM_StyxRpcSession *>(state);
    if (!self || self->resource_kind != FVM_RESOURCE_STYXRPC_REF_CHANNEL || argc || !session->client)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Value bytes;
    FVM_Result status = fvm_object_call(c, self->resource_result.object, "recv", 0, NULL, &bytes);
    if (status != FVM_OK)
    {
        return status;
    }
    if (bytes.type == FVM_VALUE_UNDEFINED)
    {
        *out = bytes;
        return FVM_OK;
    }
    FVM_ObjectRootScope root(c, bytes.type == FVM_VALUE_OBJECT ? bytes.object : NULL);
    return fvm_styxrpc_accept_int_ref(c, self, 1, &bytes, out, state);
}
} // namespace


/* [GC:] EN/RU: See variable_ref_adapter.h. / См. variable_ref_adapter.h. */
extern "C" FVM_Result fvm_styxrpc_int_ref_channel(FVM_Context *c, FVM_Object *, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcSession *session = static_cast<FVM_StyxRpcSession *>(state);
    if (!session->client || argc != 1 || args[0].type != FVM_VALUE_OBJECT || !args[0].object)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *wrapper;
    FVM_Result status = fvm_object_create(c, &wrapper);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_ObjectRootScope root(c, wrapper);
    wrapper->resource_kind = FVM_RESOURCE_STYXRPC_REF_CHANNEL;
    wrapper->resource_result = args[0];
    wrapper->resource_result_valid = true;
    status = fvm_object_define_method_rooted(c, wrapper, "send", send, state, wrapper);
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(c, wrapper, "recv", recv, state, wrapper);
    }
    if (status == FVM_OK)
    {
        fvm_value_undefined(out);
        out->type = FVM_VALUE_OBJECT;
        out->object = wrapper;
    }
    return status;
}
