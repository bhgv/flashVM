/* Copyright (c) 2026 bhgv
 * EN: Check live alias forwarding with the named-channel value codec.
 * RU: Проверить пересылку живой привязки кодеком именованного канала.
 */
#include "../../src/styxrpc/session_named_values.hpp"
#include "../../src/capi/variable_refs.hpp"
#include "flashvm/property.h"
#include "styxrpc/named_channel.h"
#include <stdlib.h>


/* [GC:] EN: Check a detached Int32 field while the original remains a live alias.
 * RU: Проверить независимое поле Int32, пока исходное остаётся живой привязкой. */
static int sampled_int(FVM_Context *context, FVM_StyxRpcSession *session, const FVM_Value *ref)
{
    const FVM_NamedRoute route = {session, session->client, NULL};
    FVM_Value source, copy, value;
    fvm_value_undefined(&source);
    if (fvm_object_create(context, &source.object) != FVM_OK)
    {
        return 0;
    }
    source.type = FVM_VALUE_OBJECT;
    if (fvm_set_global(context, "snapshotOrigin", &source) != FVM_OK ||
        fvm_object_set(context, source.object, "live", ref) != FVM_OK)
    {
        return 0;
    }
    unsigned char *wire;
    size_t size;
    if (fvm_named_encode(context, route, &source, &wire, &size) != FVM_OK)
    {
        return 0;
    }
    int ok = size > 1 && wire[0] == STYXRPC_NAMED_VALUE &&
        fvm_named_decode(context, route, wire, size, &copy) == FVM_OK &&
        copy.type == FVM_VALUE_OBJECT &&
        fvm_object_get(context, copy.object, "live", &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == -32;
    free(wire);
    fvm_value_number(&value, 42);
    return ok && fvm_object_set(context, copy.object, "live", &value) == FVM_OK &&
        fvm_object_get(context, source.object, "live", &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == -32;
}


/* EN: Observe calls via userdata and return a number if accidentally dispatched.
 * RU: Отметить вызов через userdata и вернуть число при ошибочном вызове. */
static FVM_Result custom_getter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *,
    FVM_Value *out, void *state)
{
    ++*static_cast<int *>(state);
    fvm_value_number(out, 9);
    return FVM_OK;
}


/* [GC:] EN: An alias whose target changed into a host getter must fail without dispatch.
 * RU: Если цель привязки стала геттером хоста, отклонить снимок без вызова. */
static int refused_getter(FVM_Context *context, FVM_StyxRpcSession *session)
{
    const FVM_NamedRoute route = {session, session->client, NULL};
    FVM_Value owner, container, args[2], value, ref;
    fvm_value_undefined(&owner);
    fvm_value_undefined(&container);
    if (fvm_object_create(context, &owner.object) != FVM_OK)
    {
        return 0;
    }
    owner.type = FVM_VALUE_OBJECT;
    if (fvm_set_global(context, "getterOrigin", &owner) != FVM_OK ||
        fvm_object_create(context, &container.object) != FVM_OK)
    {
        return 0;
    }
    container.type = FVM_VALUE_OBJECT;
    args[0] = owner;
    fvm_value_string(args + 1, "target");
    fvm_value_number(&value, 42);
    int deleted = 0;
    int calls = 0;
    if (fvm_set_global(context, "getterContainer", &container) != FVM_OK ||
        fvm_object_set(context, owner.object, "target", &value) != FVM_OK ||
        fvm_capture_variable_ref(context, NULL, 2, args, &ref, NULL) != FVM_OK ||
        fvm_object_set(context, container.object, "alias", &ref) != FVM_OK ||
        fvm_object_delete(context, owner.object, "target", &deleted) != FVM_OK || !deleted ||
        fvm_object_define_accessor_rooted(context, owner.object, "target", custom_getter, NULL, &calls, NULL) != FVM_OK)
    {
        return 0;
    }
    unsigned char *wire = NULL;
    size_t size = 0;
    FVM_Result status = fvm_named_encode(context, route, &container, &wire, &size);
    bool empty = !wire && !size;
    free(wire);
    return fvm_object_delete(context, owner.object, "target", &deleted) == FVM_OK && deleted &&
        status == FVM_UNSUPPORTED && empty && !calls;
}


/* [GC:] EN: Forward ref and write through the returned alias to its original issuer.
 * RU: Переслать ref и записать через полученную привязку исходному владельцу. */
extern "C" int test_named_channel_ref(FVM_Context *context, FVM_StyxRpcSession *session,
    const FVM_Value *ref)
{
    const FVM_NamedRoute route = {session, session->client, NULL};
    unsigned char *wire;
    size_t size;
    if (fvm_named_encode(context, route, ref, &wire, &size) != FVM_OK)
    {
        return 0;
    }
    FVM_Value alias;
    int ok = size > 1 && wire[0] == STYXRPC_NAMED_INT_REF &&
        fvm_named_decode(context, route, wire, size, &alias) == FVM_OK &&
        fvm_set_global(context, "namedAlias", &alias) == FVM_OK;
    free(wire);
    fvm_value_number(&alias, -32);
    return ok && fvm_set_global(context, "namedAlias", &alias) == FVM_OK &&
        sampled_int(context, session, ref) && refused_getter(context, session);
}
