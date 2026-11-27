/* Copyright (c) 2026 bhgv
 * EN: Hosted positional graph calls using the existing managed target.
 * RU: Позиционные графовые вызовы через действующую управляемую цель.
 */
#ifndef STYXRPC_GRAPH_CLIENT_H
#define STYXRPC_GRAPH_CLIENT_H
#include "styxrpc/graph.h"
#include "styxrpc/managed_client.h"

#ifdef __cplusplus
extern "C" {
#endif

/* EN: Verify the compiled offer and graph marker, then exchange one graph frame.
 * RU: Проверить компилированное предложение и формат графа, затем выполнить один обмен. */
StyxRpcOutcome styxrpc_graph_invoke(const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcGraphReply *result);


/* EN: Use a borrowed unopened module directory instead of target's path when supplied.
 * RU: Использовать переданный неоткрытый каталог вместо пути target. */
StyxRpcOutcome styxrpc_graph_invoke_at(struct StyxFid *directory, const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcGraphReply *result);

#ifdef __cplusplus
}
#endif
#endif
