/* Copyright (c) 2026 bhgv
 * EN: Keep the wire identity with an imported variable across local alias forwarding.
 * RU: Удерживать сетевую идентичность переменной при пересылке локальных привязок.
 */
#include "../core/runtime.hpp"
#include "../capi/variable_refs.hpp"
#include "flashvm/styxrpc_session.h"
#include <new>

extern "C"
{
#include "variable_ref_adapter.h"
}

namespace
{
struct WireRef
{
    StyxClient *issuer;    /* Borrowed identity of the attached session. */
    StyxRpcIntRef reference; /* Copied original descriptor. */
};


/* EN/RU: Release identity metadata with its private owner. / Освободить метаданные с владельцем. */
void release(void *state)
{
    delete static_cast<WireRef *>(state);
}
} // namespace


/* EN/RU: See variable_ref_adapter.h. / См. variable_ref_adapter.h. */
extern "C" FVM_Result fvm_styxrpc_remember_int_ref(FVM_Object *owner, StyxClient *issuer, const StyxRpcIntRef *reference)
{
    WireRef *identity = new (std::nothrow) WireRef;
    if (!identity)
    {
        return FVM_OUT_OF_MEMORY;
    }
    identity->issuer = issuer;
    identity->reference = *reference;
    owner->resource_kind = FVM_RESOURCE_STYXRPC_INT_REF;
    owner->resource = identity;
    owner->release_resource = release;
    return FVM_OK;
}


/* EN/RU: See variable_ref_adapter.h. / См. variable_ref_adapter.h. */
extern "C" int fvm_styxrpc_encode_int_ref(FVM_StyxRpcSession *binding, const FVM_Value *value,
    unsigned char *bytes, size_t capacity)
{
    if (!binding->client || !fvm_is_variable_ref(value))
    {
        return 0;
    }
    const FVM_Object *ref = value->object;
    const FVM_Object *owner = ref->resource_result.object;
    if (owner->resource_kind != FVM_RESOURCE_STYXRPC_INT_REF || ref->resource_path != "value")
    {
        return 0;
    }
    const WireRef *identity = static_cast<const WireRef *>(owner->resource);
    return identity->issuer == binding->client && styxrpc_int_ref_encode(&identity->reference, bytes, capacity);
}


/* [GC:] EN/RU: See variable_ref_adapter.h. / См. variable_ref_adapter.h. */
extern "C" FVM_Result fvm_styxrpc_int_ref_bytes(FVM_Context *context, FVM_Object *, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    unsigned char bytes[STYXRPC_INT_REF_SIZE];
    if (argc != 1 || !fvm_styxrpc_encode_int_ref(static_cast<FVM_StyxRpcSession *>(state), args, bytes, sizeof(bytes)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Value values[STYXRPC_INT_REF_SIZE];
    for (unsigned i = 0; i < STYXRPC_INT_REF_SIZE; ++i)
    {
        fvm_value_number(values + i, bytes[i]);
    }
    return fvm_exec_literal_array(context, STYXRPC_INT_REF_SIZE, values, out);
}
