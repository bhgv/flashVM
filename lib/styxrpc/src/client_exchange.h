/* Copyright (c) 2026 bhgv
 * EN: Internal common hosted function transport.
 * RU: Внутренний общий транспорт хостовых функций.
 */
#ifndef STYXRPC_CLIENT_EXCHANGE_H
#define STYXRPC_CLIENT_EXCHANGE_H
#include "styxrpc/managed.h"
#include "client.h"


/* EN: Return an owned ORDWR function fid after matching profile and optional schema.
 * RU: Вернуть собственный ORDWR fid после проверки профиля и необязательной схемы. */
StyxFid *styxrpc_client_function(StyxClient *session, const char *module, const char *name,
    uint32_t signature, const StyxRpcSchema *schema, int *scoped);


/* EN: Borrow the open fid and buffers; publish received only on a complete reply.
 * RU: Заимствовать fid и буферы; выдать received только при полном ответе. */
int styxrpc_client_exchange(StyxFid *fid, const void *request, size_t size, unsigned wait_ms,
    void *reply, size_t capacity, size_t *received);
#endif
