/* Copyright (c) 2026 bhgv
 * EN: Keep table type checks and nonallocating payload replacement inside the VM owner.
 * RU: Проверка типа таблицы и замена данных без аллокаций внутри владельца VM.
 */
#include "hybrid.h"
#include "../core/runtime.hpp"


/* EN/RU: See hybrid.h. / См. hybrid.h. */
extern "C" StyxRpcHybrid *fvm_styxrpc_hybrid_lookup(FVM_Context *context, FVM_Object *object)
{
    return context && !context->destroying && !context->gc_releasing && object &&
        object->owner == context && object->resource_kind == FVM_RESOURCE_USERDATA &&
        object->release_resource == fvm_styxrpc_hybrid_release ?
        static_cast<StyxRpcHybrid *>(object->resource) : NULL;
}


/* EN/RU: See hybrid.h. / См. hybrid.h. */
extern "C" void fvm_styxrpc_hybrid_swap(FVM_Object *object, StyxRpcHybrid **replacement)
{
    StyxRpcHybrid *previous = static_cast<StyxRpcHybrid *>(object->resource);
    object->resource = *replacement;
    *replacement = previous;
}
