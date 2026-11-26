/* Copyright (c) 2026 bhgv
 * EN: Install a callable file publisher for an active movie context.
 * RU: Публикация файлов с колбэками активного фильма.
 */
#ifndef FVM_STYXRPC_SESSION_NODES_H
#define FVM_STYXRPC_SESSION_NODES_H
#include "flashvm/value.h"
#include "styxrpc/module_slot.h"

#ifdef __cplusplus
extern "C" {
#endif

/* [GC:] EN: Publish a counted path and mode using an AS function owned by context.
 * RU: Опубликовать путь и права, используя функцию AS во владении context. */
FVM_Result fvm_styxrpc_publish_node(FVM_Context *context, StyxRpcModuleSlot *slot,
    unsigned argc, const FVM_Value *args, FVM_Value *out);

#ifdef __cplusplus
}
#endif
#endif
