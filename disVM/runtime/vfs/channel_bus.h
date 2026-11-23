/* Copyright (c) 2026 bhgv
 * EN: Call-scoped UniBus transport for typed Limbo channel operations.
 * RU: Транспорт UniBus для типизированных операций каналов Limbo в пределах вызова.
 */
#ifndef DVM_CHANNEL_BUS_H
#define DVM_CHANNEL_BUS_H
#include "provider.h"
#include "../../native/terminal/call_record.h"
#include <stdint.h>

#define DVM_CHANNEL_BUS_SLOTS 8U /* Maximum distinct channels in one call. */
#define DVM_CHANNEL_BUS_SEND 25UL /* Capability and an element record. */
#define DVM_CHANNEL_BUS_RECV 26UL /* Capability, with no value payload. */
#define DVM_CHANNEL_BUS_REPLY 27UL /* Accepted operation and optional received value. */
#define DVM_CHANNEL_BUS_ALT 28UL /* One selection among wrapped send/receive guards. */
#define DVM_CHANNEL_BUS_TRY_ALT 29UL /* Selection, or zero if no guard was ready. */
#define DVM_CHANNEL_BUS_TYPED_REPLY 30UL /* Selected guard byte and optional element. */
#define DVM_CHANNEL_BUS_WIRE STYX_MANAGED_LIST_LIMIT /* Complete message bound including its S-record. */

typedef struct DVM_ChannelBus DVM_ChannelBus; /* Borrowed while its provider is retained. */


/* EN: Immutable wire identity and full element schema within a call.
 * RU: Неизменяемые сетевой ключ и полная схема элемента в пределах вызова. */
typedef struct DVM_ChannelSchema
{
    uint32_t key; /* Positive wire key. */
    char shape[STYX_SHAPE_MAX + 1]; /* Element schema wrapped in a frame. */
} DVM_ChannelSchema;


/* EN: One decoded guard carrying only native values and a scoped slot.
 * RU: Одно разобранное условие с нативными значениями и номером слота. */
typedef struct DVM_ChannelChoice
{
    unsigned slot; /* Zero-based capability slot. */
    int receive; /* True if the remote process is receiving. */
    size_t offset; /* Offset of a send payload within the native request. */
    size_t size; /* Wire extent of a send element. */
    int32_t integer; /* Legacy scalar int send, if this is an int channel. */
} DVM_ChannelChoice;


/* EN: Copy the pending operation out of the native endpoint.
 * RU: Скопировать ожидающую операцию из нативного конца. */
typedef struct DVM_ChannelOperation
{
    uint64_t sequence; /* Exact request on one opened fid. */
    unsigned count; /* Number of guards to check as one operation. */
    int alt; /* Reply includes the selected guard number. */
    int immediate; /* Reply with no selected guard if none is ready. */
    int typed; /* Use a reply with a nested value instead of the legacy int. */
    size_t extent; /* Number of valid bytes in the request buffer. */
    DVM_ChannelChoice choices[DVM_CHANNEL_BUS_SLOTS]; /* In priority order for this attempt. */
    unsigned char bytes[DVM_CHANNEL_BUS_WIRE]; /* Only copied through the highest referenced offset. */
} DVM_ChannelOperation;


/* EN: Copy count capability schemas; return a provider owning *bus, or NULL.
 * RU: Скопировать count схем полномочий; вернуть провайдер, владеющий *bus, либо NULL. */
DVM_Provider *dvm_channel_bus_create(const DVM_ChannelSchema *schemas, size_t count, DVM_ChannelBus **bus);


/* EN: Publish caller-owned capabilities after successfully encoding one message.
 * RU: Опубликовать полномочия клиента после успешного кодирования сообщения. */
int dvm_channel_bus_extend(DVM_ChannelBus *bus, const DVM_ChannelSchema *schemas, size_t count);

/* EN: Copy a pending operation after sequence, or report none. No guest access.
 * RU: Скопировать операцию после sequence либо вернуть 0. Куча VM не затрагивается. */
int dvm_channel_bus_next(DVM_ChannelBus *bus, uint64_t sequence, DVM_ChannelOperation *operation);


/* EN: Check whether the exact pending sequence is still admitted before a match.
 * RU: Перед сопоставлением проверить, что допущена точная ожидающая операция. */
int dvm_channel_bus_pending(DVM_ChannelBus *bus, uint64_t sequence);


/* EN: Complete a still-open operation on its exact fid. Ignore a retired one.
 * RU: Завершить открытую операцию на её fid. Закрытая операция пропускается. */
void dvm_channel_bus_finish(DVM_ChannelBus *bus, const DVM_ChannelOperation *operation,
    int success, int64_t value, unsigned selected, const void *element, size_t size);


/* EN: After the owner's local probe round, pair pending fids up to probed_through
 * on unbuffered_mask slots. Publish both replies under the bus lock, then settle
 * any unmatched immediate selectors. Caller owns the bus and drives its matches.
 * RU: После обхода локальных очередей сопоставить ожидающие fid до probed_through
 * для слотов unbuffered_mask. Опубликовать оба ответа под замком шины, затем
 * завершить оставшиеся пробы. Вызывающий удерживает bus и выполняет сопоставления. */
void dvm_channel_bus_settle(DVM_ChannelBus *bus, uint64_t probed_through, unsigned unbuffered_mask);


/* EN: Stop admission and invalidate pending operations before retiring the caller.
 * RU: Запретить новые операции и отозвать ожидающие до выхода клиента. */
void dvm_channel_bus_seal(DVM_ChannelBus *bus);


/* EN: Encode a request for key, copying the borrowed complete element for a send.
 * RU: Кодировать запрос для key, копируя заимствованную запись элемента при отправке. */
int dvm_channel_bus_write(uint32_t key, int receive, const void *element, size_t extent,
    unsigned char *bytes, size_t capacity, size_t *size);

#endif
