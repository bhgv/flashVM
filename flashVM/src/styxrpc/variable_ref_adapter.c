/* Copyright (c) 2026 bhgv
 * EN: Turn a received Int32 reference payload into a transparent ActionScript binding operand.
 * RU: Превратить полученный дескриптор Int32 в прозрачную ссылку ActionScript.
 */
#include "session_methods.h"
#include "variable_ref_adapter.h"


/* [ASINC] [GC:] EN/RU: See variable_ref_adapter.h. / См. variable_ref_adapter.h. */
FVM_Result fvm_styxrpc_accept_int_ref(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcSession *binding = state;
    unsigned char bytes[STYXRPC_INT_REF_SIZE];
    size_t size;
    StyxRpcIntRef reference;
    FVM_GCRootScope *roots;
    FVM_Value capture[2];
    FVM_Result status;
    (void)self;
    if (!binding->client || argc != 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_styxrpc_bytes(context, args, bytes, sizeof(bytes), &size);
    if (status != FVM_OK)
    {
        return status;
    }
    if (!styxrpc_int_ref_decode(bytes, size, &reference))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_value_undefined(capture);
    status = fvm_object_create(context, &capture[0].object);
    if (status == FVM_OK)
    {
        capture[0].type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, capture);
    }
    if (status == FVM_OK)
    {
        status = fvm_styxrpc_int_ref_bind(context, capture[0].object, binding->client, &reference);
    }
    if (status == FVM_OK)
    {
        status = fvm_styxrpc_remember_int_ref(capture[0].object, binding->client, &reference);
    }
    if (status == FVM_OK)
    {
        fvm_value_string(capture + 1, "value");
        status = fvm_capture_variable_ref(context, self, 2, capture, out, NULL);
    }
    fvm_gc_roots_end(roots);
    return status;
}
