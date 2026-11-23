/* Copyright (c) 2026 bhgv
 * EN: Internal graph delta encoding and staged copy-back for FlashVM.
 * RU: Внутренние дельты графов и подготовленная обратная запись FlashVM.
 */
#ifndef FVM_STYXRPC_GRAPH_REFS_H
#define FVM_STYXRPC_GRAPH_REFS_H
#include "managed_refs.h"


/* [GC:] EN: Encode sparse updates to original objects, using the result's identity table.
 * RU: Кодировать изменения исходных объектов с таблицей идентификаторов результата. */
int fvm_styxrpc_graph_updates(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    const StyxRpcSchema *schema, const FVM_SRecord *arguments, unsigned originals,
    unsigned char *bytes, size_t capacity, size_t *used);


/* [GC:] EN: Decode validated graph updates and stage every affected destination.
 * RU: Декодировать проверенные обновления и подготовить все слоты назначения. */
int fvm_styxrpc_graph_stage(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    const StyxRpcSchema *schema, const FVM_SRecord *arguments, unsigned originals,
    const FVM_SRecord *updates);
#endif
