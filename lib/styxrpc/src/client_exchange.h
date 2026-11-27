/* Copyright (c) 2026 bhgv
 * EN: Internal common hosted function transport.
 * RU: Внутренний общий транспорт хостовых функций.
 */
#ifndef STYXRPC_CLIENT_EXCHANGE_H
#define STYXRPC_CLIENT_EXCHANGE_H
#include "styxrpc/managed_client.h"
#include "vfs_client_internal.h"


/* EN: Return an owned ORDWR function fid after matching profile and optional schema.
 * RU: Вернуть собственный ORDWR fid после проверки профиля и необязательной схемы. */
StyxFid *styxrpc_client_function(StyxFid *base, StyxClient *session, const char *module, const char *name,
    uint32_t signature, const StyxRpcSchema *schema, const char *graph_marker, int *scoped, StyxRpcVfsWait *wait);


/* EN: Open a verified scoped channel function and retain its exact module directory.
 * RU: Открыть проверенную функцию с каналами и удержать точный каталог модуля. */
StyxFid *styxrpc_client_channel_function(StyxFid *base, const StyxRpcManagedTarget *target,
    StyxFid **module, StyxRpcVfsWait *wait);


/* EN: Borrow the open fid and buffers; publish received only on a complete reply.
 * RU: Заимствовать fid и буферы; выдать received только при полном ответе. */
StyxRpcOutcome styxrpc_client_exchange(StyxFid *fid, const void *request, size_t size, unsigned wait_ms,
    void *reply, size_t capacity, size_t *received, StyxRpcCallStop stop, void *state);


/* EN: Match a managed offer, scope its request when required and exchange once.
 * RU: Сопоставить управляемое предложение, добавить контекст и выполнить один обмен. */
StyxRpcOutcome styxrpc_client_managed_exchange(StyxFid *base, const StyxRpcManagedTarget *target,
    const char *graph_marker, const void *request, size_t request_size, void *reply,
    size_t capacity, size_t *received, StyxRpcCallStop stop, void *state);
#endif
