/* Copyright (c) 2026 bhgv
 * EN: Internal graph delta encoding and staged copy-back for FlashVM.
 * RU: Внутренние дельты графов и подготовленная обратная запись FlashVM.
 */
#ifndef FVM_STYXRPC_GRAPH_REFS_H
#define FVM_STYXRPC_GRAPH_REFS_H
#include "managed_refs.h"
#include "styxrpc/graph.h"


/* EN: Allocate a call-local graph ledger. On refusal the count stays zero.
 * RU: Выделить журнал графа вызова. При отказе число записей остаётся нулевым. */
int fvm_styxrpc_graph_init(FVM_StyxRefs *refs);


/* EN: Release staged resources and the owned ledger after success or refusal.
 * RU: Освободить подготовленные ресурсы и журнал после успеха или отказа. */
void fvm_styxrpc_graph_clear(FVM_StyxRefs *refs);


/* [GC:] EN: Decode the result and validate and commit all graph changes under roots.
 * RU: Декодировать результат, проверить и применить все изменения графа под корнями. */
int fvm_styxrpc_graph_result(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    const StyxRpcSchema *schema, const FVM_SRecord *arguments, const StyxRpcGraphReply *reply, FVM_Value *out);


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
