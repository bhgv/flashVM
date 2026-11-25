/* Copyright (c) 2026 bhgv
 * EN: Queued access to the standard mixed-type UniBus schemas.
 * RU: Доступ через очередь к стандартным смешанным схемам UniBus.
 */
#ifndef STYXRPC_EXCHANGE_H
#define STYXRPC_EXCHANGE_H
#include "styxrpc/channel_messages.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_EXCHANGE_DEFAULT ".rpc" /* Standard application exchange on a paired player. */

typedef struct StyxRpcExchange StyxRpcExchange; /* Mailbox operated by one native owner. */
typedef enum StyxRpcExchangeKind
{
    STYXRPC_EXCHANGE_INT = 1, /* Managed Int32 child. */
    STYXRPC_EXCHANGE_REAL,    /* Managed binary64 child. */
    STYXRPC_EXCHANGE_BOOL,    /* One-byte Boolean. */
    STYXRPC_EXCHANGE_TEXT,    /* Counted WTF-8. */
    STYXRPC_EXCHANGE_NULL,    /* Empty payload. */
    STYXRPC_EXCHANGE_REF,     /* Int32 reference relative to the server. */
    STYXRPC_EXCHANGE_END      /* Upper bound, never a message kind. */
} StyxRpcExchangeKind;


/* EN: Create a disconnected mailbox. Release after the attached port/server is closed.
 * RU: Создать отключённую очередь. Освободить после закрытия порта или сервера. */
StyxRpcExchange *styxrpc_exchange_create(void);


/* EN: Release disconnected exchange and its queued payloads. NULL is valid.
 * RU: Освободить отключённый exchange и его очередь. NULL допустим. */
void styxrpc_exchange_destroy(StyxRpcExchange *exchange);


/* EN: Supply a spec and caller-owned types valid until copied by open/publish.
 * With hosted nonzero the hooks track the current server port (one active peer).
 * RU: Заполнить spec и types вызывающего до копирования в open/publish.
 * При ненулевом hosted хуки отслеживают текущий серверный порт (один активный peer). */
void styxrpc_exchange_spec(StyxRpcExchange *exchange, int hosted,
    StyxRpcMessageType types[STYXRPC_EXCHANGE_END - 1], StyxRpcMessageSpec *spec);


/* EN: Associate a client port, or NULL to detach and discard queued input. No I/O.
 * RU: Связать клиентский порт или NULL для разъединения и очистки входа. Без I/O. */
void styxrpc_exchange_attach(StyxRpcExchange *exchange, StyxRpcMessagePort *port);


/* EN: Queue one payload of a known kind. MORE means no active port or no queue room.
 * RU: Поставить сообщение известного вида в очередь. MORE при отсутствии порта или места. */
StyxRpcStatus styxrpc_exchange_send(StyxRpcExchange *exchange, StyxRpcExchangeKind kind,
    const void *bytes, size_t size);


/* EN: Return nonzero if a complete record is queued. No I/O.
 * RU: Вернуть ненулевое значение, если в очереди полная запись. Без I/O. */
int styxrpc_exchange_pending(const StyxRpcExchange *exchange);


/* EN: Pop one record into capacity bytes, returning OK or MORE when empty.
 * INVALID leaves the queue unchanged if arguments or capacity are invalid.
 * RU: Извлечь запись в буфер capacity байт, OK либо MORE для пустой очереди.
 * INVALID сохраняет очередь при ошибке параметров или ёмкости. */
StyxRpcStatus styxrpc_exchange_recv(StyxRpcExchange *exchange, StyxRpcExchangeKind *kind,
    void *bytes, size_t capacity, size_t *size);

#ifdef __cplusplus
}
#endif
#endif
