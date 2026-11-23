/* Copyright (c) 2026 bhgv
 * EN: Validate Dis graph calls and return/update pairs without a VM heap.
 * RU: Проверка графовых вызовов Dis и пары результат/изменения без кучи VM.
 */
#ifndef STYXRPC_GRAPH_H
#define STYXRPC_GRAPH_H
#include "styxrpc/managed.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_GRAPH_REQUEST_TAG 112UL /* Dis positional sparse-ref request. */
#define STYXRPC_GRAPH_UPDATES_TAG 19UL  /* Positional sparse-ref update list. */
#define STYXRPC_GRAPH_ARGUMENT_TAG 18UL /* Direct argument position, field mask, values. */
#define STYXRPC_POSITIONAL_GRAPH "v00009\n" /* .ref-fields contents and advertised length. */


/* EN: Borrow the encoded children of each validated reply field.
 * RU: Заимствовать закодированных потомков обоих проверенных полей ответа. */
typedef struct StyxRpcGraphReply
{
    FVM_SRecord result;  /* One result wrapper, including the nil tag for void. */
    FVM_SRecord updates; /* Sparse definitions or field edits validated against the request. */
} StyxRpcGraphReply;


/* EN: Encode one compatible .interface entry for a direct value-graph export.
 * RU: Закодировать одну совместимую запись .interface прямого графового экспорта. */
StyxRpcStatus styxrpc_graph_profile(const char *name, uint32_t signature, const StyxRpcSchema *schema,
    void *bytes, size_t capacity, size_t *used);


/* EN: Encode checked graph argument children in the positional Dis frame.
 * RU: Закодировать проверенных потомков аргументов в позиционном кадре Dis. */
StyxRpcStatus styxrpc_graph_request(const StyxRpcSchema *schema, const void *payload,
    size_t size, void *bytes, size_t capacity, size_t *used);


/* EN: Borrow the arguments of one complete verified positional request.
 * RU: Заимствовать аргументы полного проверенного позиционного запроса. */
StyxRpcStatus styxrpc_graph_arguments(const StyxRpcSchema *schema, const void *request, size_t request_size,
    FVM_SRecord *arguments);


/* EN: Validate and seal a result record plus update-list record against the original request.
 * RU: Проверить и упаковать запись результата и запись изменений по исходному запросу. */
StyxRpcStatus styxrpc_graph_reply(const StyxRpcSchema *schema, const void *request, size_t request_size,
    const void *result, size_t result_size, const void *updates, size_t update_size,
    void *bytes, size_t capacity, size_t *used);


/* EN: Validate both complete frames and borrow views only after all checks.
 * RU: Проверить оба полных кадра и выдать представления после всех проверок. */
StyxRpcStatus styxrpc_graph_read(const StyxRpcSchema *schema, const void *request, size_t request_size,
    const void *reply, size_t reply_size, StyxRpcGraphReply *decoded);

#ifdef __cplusplus
}
#endif
#endif
