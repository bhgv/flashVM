/* Copyright (c) 2026 bhgv
 * EN: Check live alias forwarding with the named-channel value codec.
 * RU: Проверить пересылку живой привязки кодеком именованного канала.
 */
#include "../../src/styxrpc/session_named_values.hpp"
#include "styxrpc/named_channel.h"
#include <stdlib.h>


/* [GC:] EN: Forward ref and write through the returned alias to its original issuer.
 * RU: Переслать ref и записать через полученную привязку исходному владельцу. */
extern "C" int test_named_channel_ref(FVM_Context *context, FVM_StyxRpcSession *session,
    const FVM_Value *ref)
{
    unsigned char *wire;
    size_t size;
    if (fvm_named_encode(context, session, ref, &wire, &size) != FVM_OK)
    {
        return 0;
    }
    FVM_Value alias;
    int ok = size > 1 && wire[0] == STYXRPC_NAMED_INT_REF &&
        fvm_named_decode(context, session, wire, size, &alias) == FVM_OK &&
        fvm_set_global(context, "namedAlias", &alias) == FVM_OK;
    free(wire);
    fvm_value_number(&alias, -32);
    return ok && fvm_set_global(context, "namedAlias", &alias) == FVM_OK;
}
