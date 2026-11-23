/* Copyright (c) 2026 bhgv
 * EN: Owner and type check for script UniBus userdata before examining its payload.
 * RU: Проверка владельца и типа UniBus userdata до чтения данных.
 */
#include "channel_private.h"
#include "../core/runtime.hpp"


/* EN/RU: See channel_private.h. / См. channel_private.h. */
extern "C" FVM_StyxRemoteChannel *fvm_styxrpc_channel_lookup(FVM_Context *context, FVM_Object *object)
{
    return context && !context->destroying && !context->gc_releasing && object &&
        object->owner == context && object->resource_kind == FVM_RESOURCE_USERDATA &&
        object->release_resource == fvm_styxrpc_channel_release ?
        static_cast<FVM_StyxRemoteChannel *>(object->resource) : NULL;
}
