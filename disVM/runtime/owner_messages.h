/* Copyright (c) 2026 bhgv
 * EN: Native request and reply queues with caller-owned message storage.
 * RU: Нативные очереди запросов и ответов с памятью вызывающей стороны.
 */
#ifndef DVM_OWNER_MESSAGES_H
#define DVM_OWNER_MESSAGES_H

#include <stdint.h>

typedef struct DVM_MessageRouter DVM_MessageRouter;
typedef struct DVM_MessageEndpoint DVM_MessageEndpoint;


/* EN: A scalar message node transferred to the recipient on successful send.
 * RU: Узел сообщения со скалярами. После отправки принадлежит получателю. */
typedef struct DVM_OwnerMessage
{
    struct DVM_OwnerMessage *next; /* Queue link, cleared when taken. */
    uintptr_t sender;             /* Registered sender ID. */
    uint64_t request;             /* Correlation ID scoped to the requester. */
    unsigned kind;                /* Operation or reply kind. */
    intptr_t value;               /* Scalar argument or result. */
} DVM_OwnerMessage;

/* EN: Allocate a router. Destroy only after unregistering all endpoints.
 * RU: Создать маршрутизатор. Удалять после регистрации выхода всех получателей. */
DVM_MessageRouter *dvm_messages_create(void);
void dvm_messages_destroy(DVM_MessageRouter *router);

/* EN: Register a nonzero ID. The endpoint has one consumer. Close after
 * joining that consumer and draining both queues; returns zero if not empty.
 * RU: Зарегистрировать ненулевой ID с одним потребителем. Закрывать после
 * завершения потребителя и очистки очередей. Вернуть ноль, если есть записи. */
DVM_MessageEndpoint *dvm_messages_register(DVM_MessageRouter *router, uintptr_t id);
int dvm_messages_unregister(DVM_MessageRouter *router, DVM_MessageEndpoint *endpoint);

/* EN: Transfer node to destination's request FIFO or reply FIFO. Failure
 * leaves ownership with the caller. Senders must exclusively own node.
 * RU: Передать узел в FIFO запросов или ответов получателя. При отказе
 * узел остаётся у отправителя, который должен владеть им единолично. */
int dvm_messages_send(DVM_MessageRouter *router, uintptr_t destination,
    int reply, DVM_OwnerMessage *node);

/* EN: Post a stable embedded request. Return 2 if already pending.
 * RU: Поставить встроенный запрос. Вернуть 2, если он уже в очереди. */
int dvm_messages_send_once(DVM_MessageRouter *router, uintptr_t destination,
    DVM_OwnerMessage *node);

/* EN: Test for kind, or any request when kind is zero.
 * RU: Проверить наличие kind, либо любого запроса при нуле. */
int dvm_messages_pending(DVM_MessageEndpoint *endpoint, int reply, unsigned kind);

/* EN: Take one node from the selected FIFO. With wait set, sleep until a
 * node arrives or stop is requested. Existing nodes remain drainable.
 * RU: Извлечь узел из выбранного FIFO. С wait ждать узла или остановки.
 * После остановки накопленные узлы можно извлечь. */
DVM_OwnerMessage *dvm_messages_take(DVM_MessageEndpoint *endpoint, int reply, int wait);

/* EN: Reject future sends to endpoint and wake its consumer.
 * RU: Закрыть приём новых сообщений и разбудить потребителя endpoint. */
void dvm_messages_stop(DVM_MessageEndpoint *endpoint);

#endif
