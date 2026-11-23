/* Copyright (c) 2026 bhgv
 * EN: Call-scoped UniBus transport for one-word Limbo channel operations.
 * RU: Транспорт UniBus для однословных операций каналов Limbo в пределах вызова.
 */
#ifndef DVM_CHANNEL_BUS_H
#define DVM_CHANNEL_BUS_H
#include "provider.h"
#include <stdint.h>

#define DVM_CHANNEL_BUS_SLOTS 8U /* Maximum distinct channels in one call. */
#define DVM_CHANNEL_BUS_SEND 25UL /* Capability and a signed integer record. */
#define DVM_CHANNEL_BUS_RECV 26UL /* Capability, with no value payload. */
#define DVM_CHANNEL_BUS_REPLY 27UL /* Accepted operation and optional received value. */
#define DVM_CHANNEL_BUS_ALT 28UL /* One selection among wrapped send/receive guards. */
#define DVM_CHANNEL_BUS_TRY_ALT 29UL /* Selection, or zero if no guard was ready. */
#define DVM_CHANNEL_BUS_WIRE 256U /* Bound on a complete eight-guard request. */

typedef struct DVM_ChannelBus DVM_ChannelBus; /* Borrowed while its provider is retained. */


/* EN: One decoded guard carrying only native values and a scoped slot.
 * RU: Одно разобранное условие с нативными значениями и номером слота. */
typedef struct DVM_ChannelChoice
{
    unsigned slot; /* Zero-based capability slot. */
    int receive; /* True if the remote process is receiving. */
    int32_t value; /* Signed Limbo int for a remote send. */
} DVM_ChannelChoice;


/* EN: Copy the pending operation out of the native endpoint.
 * RU: Скопировать ожидающую операцию из нативного конца. */
typedef struct DVM_ChannelOperation
{
    uint64_t sequence; /* Exact request on one opened fid. */
    unsigned count; /* Number of guards to check as one operation. */
    int alt; /* Reply includes the selected guard number. */
    int immediate; /* Reply with no selected guard if none is ready. */
    DVM_ChannelChoice choices[DVM_CHANNEL_BUS_SLOTS]; /* In priority order for this attempt. */
} DVM_ChannelOperation;


/* EN: Copy count capability keys; return a provider owning *bus, or NULL.
 * RU: Скопировать count ключей; вернуть провайдер, владеющий *bus, либо NULL. */
DVM_Provider *dvm_channel_bus_create(const uint32_t *keys, size_t count, DVM_ChannelBus **bus);


/* EN: Copy a pending operation after sequence, or report none. No guest access.
 * RU: Скопировать операцию после sequence либо вернуть 0. Куча VM не затрагивается. */
int dvm_channel_bus_next(DVM_ChannelBus *bus, uint64_t sequence, DVM_ChannelOperation *operation);


/* EN: Check whether the exact pending sequence is still admitted before a match.
 * RU: Перед сопоставлением проверить, что допущена точная ожидающая операция. */
int dvm_channel_bus_pending(DVM_ChannelBus *bus, uint64_t sequence);


/* EN: Complete a still-open operation on its exact fid. Ignore a retired one.
 * RU: Завершить открытую операцию на её fid. Закрытая операция пропускается. */
void dvm_channel_bus_finish(DVM_ChannelBus *bus, const DVM_ChannelOperation *operation,
    int success, int64_t value);


/* EN: Stop admission and invalidate pending operations before retiring the caller.
 * RU: Запретить новые операции и отозвать ожидающие до выхода клиента. */
void dvm_channel_bus_seal(DVM_ChannelBus *bus);


/* EN: Encode a request for key, optionally transmitting value. Return 1 on success.
 * RU: Кодировать запрос для key, при отправке добавить value. Успех возвращает 1. */
int dvm_channel_bus_write(uint32_t key, int receive, int32_t value,
    unsigned char *bytes, size_t capacity, size_t *size);

#endif
