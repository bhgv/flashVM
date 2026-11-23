/* Copyright (c) 2026 bhgv
 * EN: Dis-compatible UniBus frames for admitted typed channel operations.
 * RU: Кадры UniBus для допущенных типизированных операций каналов Dis.
 */
#ifndef STYXRPC_CHANNEL_H
#define STYXRPC_CHANNEL_H
#include "styxrpc/managed.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_CHANNEL_SLOTS 8U /* Existing maximum number of guards. */
#define STYXRPC_CHANNEL_ELEMENT_TAG 110UL /* Managed wrapper around a typed element. */


/* EN: Immutable local grant for one channel and its element frame schema.
 * RU: Неизменяемое локальное полномочие на канал со схемой кадра элемента. */
typedef struct StyxRpcChannelGrant
{
    uint32_t key;                     /* Positive key assigned by the session owner. */
    char shape[STYXRPC_MANAGED_SHAPE]; /* One type in parentheses. */
} StyxRpcChannelGrant;

typedef enum StyxRpcChannelMode
{
    STYXRPC_CHANNEL_ONE = 0,  /* A single send/receive with no ALT wrapper. */
    STYXRPC_CHANNEL_ALT = 28, /* Blocking selection. */
    STYXRPC_CHANNEL_TRY = 29, /* Immediate selection; zero means idle. */
    STYXRPC_CHANNEL_WAIT = 32 /* Authoritative bounded selection on the owning scheduler. */
} StyxRpcChannelMode;


/* EN: Borrow a checked guard with a send element or an empty receive.
 * RU: Заимствовать проверенную ветку с элементом отправки либо пустым приёмом. */
typedef struct StyxRpcGuard
{
    uint32_t key;        /* Key present in the local grant table. */
    int receive;         /* Exactly 1 for receive, 0 for send. */
    const void *value;   /* Complete element record borrowed through completion. */
    size_t size;         /* Bytes in the record, zero for receive. */
} StyxRpcGuard;


/* EN: Store parsed guards. Keys and borrowed value views belong to one request.
 * RU: Хранить разобранные ветки. Ключи и данные относятся к одному запросу. */
struct StyxRpcChannelPolicy; /* Optional delegated authority defined in channel_route.h. */

typedef struct StyxRpcSelection
{
    StyxRpcChannelMode mode;               /* Single operation or ALT mode. */
    size_t count;                          /* Valid guard count. */
    StyxRpcGuard guards[STYXRPC_CHANNEL_SLOTS]; /* In submitted preference order. */
    const struct StyxRpcChannelPolicy *policy; /* Optional immutable policy through completion. */
} StyxRpcSelection;


/* EN: Encode all admitted guards. Scratch is needed only for ALT variants.
 * RU: Кодировать допущенные ветки. Scratch нужен только для вариантов ALT. */
StyxRpcStatus styxrpc_channel_request(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, void *scratch, size_t scratch_size,
    void *bytes, size_t capacity, size_t *used);


/* EN: Decode one complete request into borrowed views after validating every guard.
 * RU: Разобрать полный запрос в заимствованные представления после проверки всех веток. */
StyxRpcStatus styxrpc_channel_read(const StyxRpcChannelGrant *grants, size_t grant_count,
    const void *bytes, size_t size, StyxRpcSelection *operation);


/* EN: Encode one outcome, selected is 1-based, 0 for idle, 255 for remote failure.
 * For typed ALT selecting an int receive, pass a managed frame around the scalar.
 * RU: Записать один исход. selected от 1, 0 при ожидании, 255 при ошибке.
 * Для типизированного ALT с приёмом int передайте скаляр в управляемом кадре. */
StyxRpcStatus styxrpc_channel_reply(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, unsigned selected, const void *value, size_t size,
    void *bytes, size_t capacity, size_t *used);

/* EN: Decode a complete exact reply. Selected is 1-based or zero when idle.
 * A typed received record borrows reply storage; int-only replies return integer.
 * RU: Декодировать полный ответ. Selected от 1 либо ноль при отсутствии готовых веток.
 * Типизированный элемент заимствует буфер ответа; для варианта int заполнен integer. */
typedef struct StyxRpcChannelResult
{
    unsigned selected; /* One-based guard ordinal, zero for idle. */
    const void *value; /* Borrowed typed receive record, otherwise NULL. */
    size_t size;       /* Typed record byte length. */
    int32_t integer;  /* Received int for a legacy int-only operation. */
} StyxRpcChannelResult;

/* EN: Validate a complete reply and publish the outcome only on success.
 * RU: Проверить полный ответ и выдать результат только при успехе. */
StyxRpcStatus styxrpc_channel_result(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, const void *bytes, size_t size, StyxRpcChannelResult *result);

#ifdef __cplusplus
}
#endif
#endif
