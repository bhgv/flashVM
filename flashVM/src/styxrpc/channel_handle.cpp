/* Copyright (c) 2026 bhgv
 * EN: Owner and type check for script UniBus userdata before examining its payload.
 * RU: Проверка владельца и типа UniBus userdata до чтения данных.
 */
#include "channel_private.h"
#include "flashvm/execution.h"
#include "../core/runtime.hpp"


/* EN/RU: See channel_private.h. / См. channel_private.h. */
extern "C" FVM_StyxRemoteChannel *fvm_styxrpc_channel_lookup(FVM_Context *context, FVM_Object *object)
{
    return context && !context->destroying && !context->gc_releasing && object &&
        object->owner == context && object->resource_kind == FVM_RESOURCE_USERDATA &&
        object->release_resource == fvm_styxrpc_channel_release ?
        static_cast<FVM_StyxRemoteChannel *>(object->resource) : NULL;
}


/* EN/RU: See channel_private.h. / См. channel_private.h. */
extern "C" void *fvm_styxrpc_local_channel_lookup(FVM_Context *context, FVM_Object *object)
{
    return context && !context->destroying && !context->gc_releasing && object &&
        object->owner == context && object->resource_kind == FVM_RESOURCE_USERDATA &&
        object->release_resource == fvm_styxrpc_local_channel_release ? object->resource : NULL;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
extern "C" FVM_Result fvm_styxrpc_local_edges_init(FVM_Context *context, FVM_Object *owner, unsigned capacity)
{
    FVM_Value values[STYXRPC_CHANNEL_SLOTS * FVM_STYXRPC_CHANNEL_QUEUE_SLOTS], storage;
    unsigned count = capacity * STYXRPC_CHANNEL_SLOTS;
    for (unsigned index = 0; index < count; ++index)
    {
        fvm_value_undefined(values + index);
    }
    FVM_Result status = fvm_exec_literal_array(context, count, values, &storage);
    if (status == FVM_OK)
    {
        owner->resource_result = storage;
        owner->resource_result_valid = true;
    }
    return status;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
extern "C" FVM_Object *fvm_styxrpc_local_edge(FVM_Object *owner, size_t slot, size_t index,
    FVM_Object *value, int store)
{
    FVM_Object *storage = owner->resource_result.object;
    FVM_Value &edge = storage->indexed.find(slot * STYXRPC_CHANNEL_SLOTS + index)->second;
    if (store)
    {
        fvm_value_undefined(&edge);
        if (value)
        {
            edge.type = FVM_VALUE_OBJECT;
            edge.object = value;
        }
    }
    return edge.type == FVM_VALUE_OBJECT ? edge.object : NULL;
}
