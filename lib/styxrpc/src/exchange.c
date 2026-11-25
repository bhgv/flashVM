/* Copyright (c) 2026 bhgv
 * EN: Bound the copied mixed-value mailbox shared by hosted and attached endpoints.
 * RU: Ограниченная очередь копий смешанных значений для сервера и клиента.
 */
#include "styxrpc/exchange.h"
#include "message_internal.h"
#include <stdlib.h>
#include <string.h>

#define EXCHANGE_BUFFER_LIMIT (2U * STYXRPC_MESSAGE_PAYLOAD_MAX) /* Total pending payload bytes. */
#define EXCHANGE_RECORD_LIMIT 1024U /* Bound the number of empty or tiny pending records. */

typedef struct ExchangeRoute
{
    StyxRpcExchange *exchange; /* Owner of the incoming record queue. */
    StyxRpcExchangeKind kind;   /* One wire schema identifier. */
} ExchangeRoute;

typedef struct ExchangeRecord
{
    struct ExchangeRecord *next; /* Following allocated record. */
    StyxRpcExchangeKind kind;    /* Wire schema of this record. */
    size_t size;                /* Appended payload byte count. */
} ExchangeRecord;

struct StyxRpcExchange
{
    StyxRpcMessagePort *port;                  /* Borrowed active client or server port. */
    ExchangeRoute types[STYXRPC_EXCHANGE_END]; /* Stable receive callback handles. */
    ExchangeRecord *head;                    /* Oldest pending record. */
    ExchangeRecord **tail;                   /* Link to append the next record to. */
    size_t pending_bytes;                    /* Bytes held in currently queued messages. */
    size_t pending_records;                  /* Number of currently queued messages. */
};

static const char *const names[STYXRPC_EXCHANGE_END] = {NULL,
    STYXRPC_CHANNEL_MESSAGE_INT, STYXRPC_CHANNEL_MESSAGE_REAL, STYXRPC_CHANNEL_MESSAGE_BOOL,
    STYXRPC_CHANNEL_MESSAGE_TEXT, STYXRPC_CHANNEL_MESSAGE_NULL, STYXRPC_CHANNEL_MESSAGE_REF
}; /* Schema identities indexed by their mailbox kind. */


/* EN: Free the queue head after delivery or teardown. Requires a nonempty exchange.
 * RU: Освободить голову очереди после доставки или закрытия. Очередь должна быть непустой. */
static void drop_record(StyxRpcExchange *exchange)
{
    ExchangeRecord *record = exchange->head;
    exchange->head = record->next;
    exchange->pending_bytes -= record->size;
    --exchange->pending_records;
    free(record);
    if (!exchange->head)
    {
        exchange->tail = &exchange->head;
    }
}


/* EN/RU: See exchange.h. / См. exchange.h. */
void styxrpc_exchange_attach(StyxRpcExchange *exchange, StyxRpcMessagePort *port)
{
    while (exchange->head)
    {
        drop_record(exchange);
    }
    exchange->port = port;
}


/* EN/RU: See exchange.h. / См. exchange.h. */
StyxRpcExchange *styxrpc_exchange_create(void)
{
    StyxRpcExchange *exchange = calloc(1, sizeof(*exchange));
    if (exchange)
    {
        exchange->tail = &exchange->head;
    }
    return exchange;
}


/* EN/RU: See exchange.h. / См. exchange.h. */
void styxrpc_exchange_destroy(StyxRpcExchange *exchange)
{
    if (exchange)
    {
        styxrpc_exchange_attach(exchange, NULL);
        free(exchange);
    }
}


/* EN: Copy one accepted record from the active port. On exhaustion fail that port.
 * RU: Скопировать запись активного порта. При исчерпании ресурсов закрыть порт с ошибкой. */
static int receive(StyxRpcMessagePort *port, void *context, const void *bytes, size_t size)
{
    ExchangeRoute *route = context;
    StyxRpcExchange *exchange = route->exchange;
    if (port != exchange->port || exchange->pending_records == EXCHANGE_RECORD_LIMIT ||
        size > EXCHANGE_BUFFER_LIMIT - exchange->pending_bytes)
    {
        return 0;
    }
    ExchangeRecord *record = malloc(sizeof(*record) + size);
    if (!record)
    {
        return 0;
    }
    *record = (ExchangeRecord){NULL, route->kind, size};
    if (size)
    {
        memcpy(record + 1, bytes, size);
    }
    *exchange->tail = record;
    exchange->tail = &record->next;
    exchange->pending_bytes += size;
    ++exchange->pending_records;
    return 1;
}


/* EN: Claim one live hosted port, rejecting concurrent opens before publishing a fid.
 * RU: Занять один живой серверный порт, отклонив одновременные открытия до выдачи fid. */
static void opened(StyxRpcMessagePort *port, void *context)
{
    StyxRpcExchange *exchange = context;
    if (!exchange->port)
    {
        styxrpc_exchange_attach(exchange, port);
    }
    else
    {
        port->failed = 1;
    }
}


/* EN: Retire the matching port and its queued input. Other fids leave it intact.
 * RU: Отсоединить соответствующий порт и входную очередь. Прочие fid его не меняют. */
static void closed(StyxRpcMessagePort *port, void *context)
{
    StyxRpcExchange *exchange = context;
    if (exchange->port == port)
    {
        styxrpc_exchange_attach(exchange, NULL);
    }
}


/* EN/RU: See exchange.h. / См. exchange.h. */
void styxrpc_exchange_spec(StyxRpcExchange *exchange, int hosted,
    StyxRpcMessageType types[STYXRPC_EXCHANGE_END - 1], StyxRpcMessageSpec *spec)
{
    for (unsigned kind = 1; kind < STYXRPC_EXCHANGE_END; ++kind)
    {
        ExchangeRoute *route = &exchange->types[kind];
        *route = (ExchangeRoute){exchange, (StyxRpcExchangeKind)kind};
        types[kind - 1] = (StyxRpcMessageType){names[kind], STYXRPC_CHANNEL_MESSAGE_VERSION, kind, receive, route};
    }
    *spec = (StyxRpcMessageSpec){types, STYXRPC_EXCHANGE_END - 1,
        hosted ? opened : NULL, hosted ? closed : NULL, exchange};
}


/* EN/RU: See exchange.h. / См. exchange.h. */
StyxRpcStatus styxrpc_exchange_send(StyxRpcExchange *exchange, StyxRpcExchangeKind kind,
    const void *bytes, size_t size)
{
    if (kind <= 0 || kind >= STYXRPC_EXCHANGE_END || (!bytes && size) || size > STYXRPC_MESSAGE_PAYLOAD_MAX)
    {
        return STYXRPC_INVALID;
    }
    return exchange->port ? styxrpc_message_send(exchange->port, names[kind],
        STYXRPC_CHANNEL_MESSAGE_VERSION, bytes, size) : STYXRPC_MORE;
}


/* EN/RU: See exchange.h. / См. exchange.h. */
int styxrpc_exchange_pending(const StyxRpcExchange *exchange)
{
    return exchange && exchange->head != NULL;
}


/* EN/RU: See exchange.h. / См. exchange.h. */
StyxRpcStatus styxrpc_exchange_recv(StyxRpcExchange *exchange, StyxRpcExchangeKind *kind,
    void *bytes, size_t capacity, size_t *size)
{
    ExchangeRecord *record = exchange->head;
    if (!record)
    {
        return STYXRPC_MORE;
    }
    if (!kind || !size || record->size > capacity || (record->size && !bytes))
    {
        return STYXRPC_INVALID;
    }
    if (record->size)
    {
        memcpy(bytes, record + 1, record->size);
    }
    *kind = record->kind;
    *size = record->size;
    drop_record(exchange);
    return STYXRPC_OK;
}
